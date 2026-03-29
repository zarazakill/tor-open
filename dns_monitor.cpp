#include "dns_monitor.h"
#include <QRegularExpression>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>

// ─────────────────────────────────────────────────────────────────────────────
// DnsMonitor — перехват URL/DNS посещений VPN-клиентов
//
// Подход: три параллельных источника данных, без внешних зависимостей:
//
//  1. tcpdump на tun0 (DNS-запросы порт 53) — самый точный, если есть tcpdump
//  2. Tor ControlPort ADDRMAP — DNS через Tor TransparentProxy (порт 5353)
//  3. Таймер-поллинг /proc/net/nf_conntrack — все TCP/UDP соединения клиентов
//
// Источник 3 всегда работает без root если conntrack доступен.
// ─────────────────────────────────────────────────────────────────────────────

DnsMonitor::DnsMonitor(QObject *parent) : QObject(parent)
{
    torLogProcess = new QProcess(this);
    flushTimer    = new QTimer(this);
    flushTimer->setInterval(500);

    // Таймер поллинга conntrack
    conntrackTimer = new QTimer(this);
    conntrackTimer->setInterval(2000);  // каждые 2 сек

    connect(torLogProcess, &QProcess::readyReadStandardOutput,
            this, &DnsMonitor::onTorLogOutput);
    connect(torLogProcess, &QProcess::finished,
            this, &DnsMonitor::onTorLogFinished);
    connect(flushTimer, &QTimer::timeout, this, [this]() {
        for (const QString &line : buffer) parseLine(line);
        buffer.clear();
    });
    connect(conntrackTimer, &QTimer::timeout,
            this, &DnsMonitor::pollConntrack);
}

DnsMonitor::~DnsMonitor()
{
    stopMonitoring();
}

void DnsMonitor::startMonitoring(const QString &vpnNetwork)
{
    Q_UNUSED(vpnNetwork)

    // ── Источник 1: tcpdump DNS на tun-интерфейсе ──────────────────────
    // Определяем tun-интерфейс — если ещё не поднялся, пропускаем tcpdump
    QString tunIface = detectTunInterface();

    if (!tunIface.isEmpty()) {
        // Если tcpdump уже работает на нужном интерфейсе — не перезапускаем
        bool tcpdumpRunning = tcpdumpProcess &&
        tcpdumpProcess->state() == QProcess::Running;
        if (!tcpdumpRunning) {
            if (!tcpdumpProcess) {
                tcpdumpProcess = new QProcess(this);
                connect(tcpdumpProcess, &QProcess::readyReadStandardOutput,
                        this, &DnsMonitor::onTcpdumpOutput);
                connect(tcpdumpProcess,
                        QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
                        this, [this](int code, QProcess::ExitStatus) {
                            emit logMessage(
                                QString("tcpdump завершился (код %1)").arg(code), "warning");
                            if (tcpdumpProcess) {
                                tcpdumpProcess->deleteLater();
                                tcpdumpProcess = nullptr;
                            }
                        });
            }

            tcpdumpProcess->start("tcpdump", {
                "-i", tunIface,
                "-l",           // line-buffered
                "-n",           // no name resolution
                "-v",           // verbose — нужен для DNS ANSWER (извлечение IP из ответов)
                "udp port 53 or tcp port 53 or (tcp port 80 or tcp port 443) and tcp[tcpflags] & tcp-syn != 0"
            });

            if (tcpdumpProcess->waitForStarted(2000)) {
                emit logMessage("📡 DNS-сниффер запущен на " + tunIface, "info");
                if (!flushTimer->isActive()) flushTimer->start();
            } else {
                emit logMessage(
                    "⚠️ tcpdump недоступен — установите: sudo apt install tcpdump", "warning");
                emit logMessage(
                    "💡 Без sudo: sudo setcap cap_net_raw+eip $(which tcpdump)", "info");
            }
        }
    } else {
        emit logMessage("⚠️ tun-интерфейс ещё не появился, ожидаем...", "warning");
    }

    // ── Источник 2: Tor ControlPort ADDRMAP ────────────────────────────
    if (!torControl || torControl->state() == QAbstractSocket::UnconnectedState) {
        startTorController();
    }

    // ── Источник 3: /proc/net/nf_conntrack поллинг ─────────────────────
    if (!conntrackTimer->isActive()) {
        if (QFile::exists("/proc/net/nf_conntrack")) {
            conntrackTimer->start();
            emit logMessage("📡 Conntrack-мониторинг активен", "info");
        } else if (QFile::exists("/proc/net/ip_conntrack")) {
            useIpConntrack = true;
            conntrackTimer->start();
            emit logMessage("📡 ip_conntrack-мониторинг активен", "info");
        } else {
            emit logMessage("⚠️ /proc/net/nf_conntrack недоступен — "
            "установите: sudo apt install conntrack", "warning");
        }
    }

    if (!tunIface.isEmpty() || conntrackTimer->isActive()) {
        emit logMessage("📡 URL-мониторинг запущен", "info");
    }
}

void DnsMonitor::stopMonitoring()
{
    if (tcpdumpProcess && tcpdumpProcess->state() == QProcess::Running) {
        tcpdumpProcess->terminate();
        tcpdumpProcess->waitForFinished(2000);
        tcpdumpProcess->deleteLater();
        tcpdumpProcess = nullptr;
    }
    if (torLogProcess && torLogProcess->state() == QProcess::Running) {
        torLogProcess->terminate();
        torLogProcess->waitForFinished(2000);
    }
    if (torControl) {
        torControl->disconnectFromHost();
        torControl->deleteLater();
        torControl = nullptr;
    }
    controlAuthenticated = false;
    if (flushTimer)      flushTimer->stop();
    if (conntrackTimer)  conntrackTimer->stop();
    seenConntrackKeys.clear();
}

// ─── Определение tun-интерфейса ──────────────────────────────────────────────

QString DnsMonitor::detectTunInterface()
{
    // Читаем /proc/net/dev — ищем tun0, tun1 и т.д.
    QFile f("/proc/net/dev");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!f.atEnd()) {
            QString line = QString::fromUtf8(f.readLine()).trimmed();
            if (line.startsWith("tun")) {
                QString iface = line.split(':').first().trimmed();
                f.close();
                return iface;
            }
        }
        f.close();
    }
    return QString();
}

// ─── Источник 1: tcpdump парсинг ─────────────────────────────────────────────

void DnsMonitor::onTcpdumpOutput()
{
    while (tcpdumpProcess && tcpdumpProcess->canReadLine()) {
        QString line = QString::fromUtf8(tcpdumpProcess->readLine()).trimmed();
        if (!line.isEmpty()) parseTcpdumpLine(line);
    }
}

void DnsMonitor::parseTcpdumpLine(const QString &line)
{
    // tcpdump -q UDP:  "10:28:38.123 IP 10.8.0.2.54321 > 8.8.8.8.53: UDP, length 35"
    // tcpdump -q DNS detail: не включён — нужен -v для имён
    // Используем tcpdump с -v для извлечения имён запросов:
    // "10:28:38.123 IP 10.8.0.2.54321 > 8.8.8.8.53: 12345+ A? google.com. (28)"

    // Паттерн DNS-запроса от клиента (A? AAAA? ANY?)
    static QRegularExpression dnsQueryRe(
        R"((\d+\.\d+\.\d+\.\d+)\.\d+\s+>\s+\S+\.53:.*?\?\s+(\S+?)\.\s+\()"
        );
    auto m = dnsQueryRe.match(line);
    if (m.hasMatch()) {
        QString srcIP  = m.captured(1);
        QString domain = m.captured(2);

        if (domain.isEmpty() || domain.endsWith(".local") ||
            domain.endsWith(".arpa"))  return;

        emitUrlAccess(srcIP, domain, "DNS");
        return;
    }

    // Паттерн DNS-ответа: сервер → клиент, извлекаем IP из ответа для кэша
    // "10:28:38.123 IP 8.8.8.8.53 > 10.8.0.2.12345: ... A 142.250.185.46"
    static QRegularExpression dnsAnswerRe(
        R"(\S+\.53\s+>\s+(\d+\.\d+\.\d+\.\d+)\.\d+:.*?\s+A\s+(\d+\.\d+\.\d+\.\d+))"
    );
    auto mAns = dnsAnswerRe.match(line);
    if (mAns.hasMatch()) {
        // Также пробуем извлечь доменное имя из запроса в этой же строке
        static QRegularExpression domInAnswerRe(R"(\?\s+(\S+?)\.)");
        auto mDom = domInAnswerRe.match(line);
        if (mDom.hasMatch()) {
            QString resolvedIP = mAns.captured(2);
            QString domain = mDom.captured(1);
            if (!resolvedIP.isEmpty() && !domain.isEmpty())
                ipToDomainCache[resolvedIP] = domain;
        }
        return;
    }

    // Паттерн HTTPS/HTTP соединения (SYN к порту 80/443)
    // "10:28:38.123 IP 10.8.0.2.54321 > 1.2.3.4.443: Flags [S]"
    static QRegularExpression tcpConnRe(
        R"((\d+\.\d+\.\d+\.\d+)\.\d+\s+>\s+(\d+\.\d+\.\d+\.\d+)\.(\d+):\s+Flags\s+\[S\])"
    );
    auto m2 = tcpConnRe.match(line);
    if (m2.hasMatch()) {
        QString srcIP  = m2.captured(1);
        QString dstIP  = m2.captured(2);
        QString port   = m2.captured(3);

        // Только 80/443 для HTTP/HTTPS
        if (port != "80" && port != "443") return;
        // Пропускаем если dst — сам VPN-сервер или локальные
        if (dstIP.startsWith("10.8.0.") || dstIP.startsWith("10.0.") ||
            dstIP.startsWith("192.168.") || dstIP.startsWith("127.")) return;

        QString method = (port == "443") ? "HTTPS" : "HTTP";
        // Подставляем домен из кэша если знаем, иначе используем IP
        QString displayUrl = ipToDomainCache.value(dstIP, dstIP);
        emitUrlAccess(srcIP, displayUrl, method);
    }
}

// ─── Источник 2: Tor ControlPort ─────────────────────────────────────────────

void DnsMonitor::startTorController()
{
    torControl = new QTcpSocket(this);
    connect(torControl, &QTcpSocket::connected, this, [this]() {
        // Cookie-аутентификация
        QStringList cookiePaths = {
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + "/tor_data/control_auth_cookie",
            "/var/lib/tor/control_auth_cookie",
            "/run/tor/control.authcookie"
        };
        QByteArray cookie;
        for (const QString &path : cookiePaths) {
            QFile f(path);
            if (f.open(QIODevice::ReadOnly)) {
                cookie = f.readAll(); f.close();
                break;
            }
        }
        if (!cookie.isEmpty())
            torControl->write("AUTHENTICATE " + cookie.toHex() + "\r\n");
        else
            torControl->write("AUTHENTICATE \"\"\r\n");
    });
    connect(torControl, &QTcpSocket::readyRead,
            this, &DnsMonitor::onControlData);
    connect(torControl, &QTcpSocket::errorOccurred,
            this, [this](QAbstractSocket::SocketError) {
                // Тихо — Tor может быть не запущен
            });
    torControl->connectToHost("127.0.0.1", 9051);
}

void DnsMonitor::onControlData()
{
    while (torControl->canReadLine()) {
        QString line = QString::fromUtf8(torControl->readLine()).trimmed();
        if (line.startsWith("250 OK") && !controlAuthenticated) {
            controlAuthenticated = true;
            torControl->write("SETEVENTS ADDRMAP\r\n");
            emit logMessage("✅ Tor ControlPort: ADDRMAP мониторинг активен", "info");
            continue;
        }
        if (line.contains("ADDRMAP")) parseAddrMap(line);
    }
}

void DnsMonitor::parseAddrMap(const QString &line)
{
    static QRegularExpression re(R"(ADDRMAP\s+(\S+)\s+(\S+)\s+)");
    auto m = re.match(line);
    if (!m.hasMatch()) return;

    QString hostname = m.captured(1);
    if (QRegularExpression(R"(^\d+\.\d+\.\d+\.\d+$)").match(hostname).hasMatch()) return;
    if (hostname.endsWith(".onion") || hostname == "<e>") return;

    // Tor ADDRMAP не содержит source IP — используем последнего активного клиента
    QString clientName = "unknown", clientIP;
    if (clientIPMap.size() == 1) {
        clientIP = clientIPMap.firstKey();
        clientName = clientIPMap.first();
    } else if (!lastActiveClient.isEmpty()) {
        clientName = lastActiveClient;
        clientIP   = lastActiveClientIP;
    }

    UrlAccess access;
    access.clientName = clientName;
    access.clientIP   = clientIP;
    access.url        = hostname;
    access.timestamp  = QDateTime::currentDateTime();
    access.method     = "DNS";
    emit urlAccessed(access);
}

// ─── Источник 3: /proc/net/nf_conntrack поллинг ──────────────────────────────

void DnsMonitor::pollConntrack()
{
    QString path = useIpConntrack ? "/proc/net/ip_conntrack" : "/proc/net/nf_conntrack";
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    // Извлекаем сеть VPN из clientIPMap (обычно 10.8.0.x)
    // Ищем все активные соединения от VPN-клиентов к внешним адресам
    // Формат nf_conntrack:
    // ipv4 2 tcp 6 86399 ESTABLISHED src=10.8.0.2 dst=142.250.185.46 sport=54321 dport=443 ...

    static QRegularExpression connRe(
        R"(src=(\d+\.\d+\.\d+\.\d+)\s+dst=(\d+\.\d+\.\d+\.\d+)\s+sport=\d+\s+dport=(\d+))"
    );

    QSet<QString> currentKeys;

    while (!f.atEnd()) {
        QString line = QString::fromUtf8(f.readLine());

        // Только ESTABLISHED соединения к порту 80/443/53
        if (!line.contains("ESTABLISHED") && !line.contains("TIME_WAIT")) continue;

        auto m = connRe.match(line);
        if (!m.hasMatch()) continue;

        QString src   = m.captured(1);
        QString dst   = m.captured(2);
        QString dport = m.captured(3);

        // Только VPN-клиенты (10.8.x.x или настроенная сеть)
        if (!isVpnClient(src)) continue;

        // Пропускаем локальные адреса назначения
        if (dst.startsWith("10.") || dst.startsWith("192.168.") ||
            dst.startsWith("172.16.") || dst.startsWith("127.")) continue;

        if (dport != "53" && dport != "80" && dport != "443") continue;

        // Дедупликация — показываем новые соединения
        QString key = src + "|" + dst + "|" + dport;
        currentKeys.insert(key);

        if (!seenConntrackKeys.contains(key)) {
            seenConntrackKeys.insert(key);

            QString method;
            if      (dport == "53")  method = "DNS";
            else if (dport == "443") method = "HTTPS";
            else                     method = "HTTP";

            // Подставляем домен из кэша вместо голого IP
            QString displayUrl = ipToDomainCache.value(dst, dst);
            emitUrlAccess(src, displayUrl, method);
        }
    }
    f.close();

    // Чистим устаревшие ключи (соединения закрылись)
    seenConntrackKeys.intersect(currentKeys);
}

bool DnsMonitor::isVpnClient(const QString &ip) const
{
    // Проверяем по clientIPMap
    if (clientIPMap.contains(ip)) return true;
    // Fallback: 10.8.0.x — стандартная сеть OpenVPN
    if (ip.startsWith("10.8.0.") && ip != "10.8.0.1") return true;
    return false;
}

// ─── Общая функция эмита UrlAccess ───────────────────────────────────────────

void DnsMonitor::emitUrlAccess(const QString &srcIP, const QString &url,
                               const QString &method)
{
    // Определяем клиента по VPN IP
    QString clientName = clientIPMap.value(srcIP, "unknown");
    QString clientIP   = srcIP;

    // Подставляем домен из кэша если url — это IP-адрес
    static QRegularExpression ipRe(R"(^\d+\.\d+\.\d+\.\d+$)");
    QString displayUrl = url;
    if (ipRe.match(url).hasMatch()) {
        displayUrl = ipToDomainCache.value(url, url);
    }

    // Дедупликация: не показываем один и тот же url одного клиента чаще раза в 30 сек
    QString dedupeKey = srcIP + "|" + displayUrl + "|" + method;
    QDateTime now = QDateTime::currentDateTime();
    if (seenUrlTimestamps.contains(dedupeKey)) {
        if (seenUrlTimestamps[dedupeKey].secsTo(now) < 30)
            return;
    }
    seenUrlTimestamps[dedupeKey] = now;

    // Периодически чистим старые записи (старше 5 минут)
    if (seenUrlTimestamps.size() > 2000) {
        for (auto it = seenUrlTimestamps.begin(); it != seenUrlTimestamps.end(); ) {
            if (it.value().secsTo(now) > 300)
                it = seenUrlTimestamps.erase(it);
            else
                ++it;
        }
    }

    UrlAccess access;
    access.clientName = clientName;
    access.clientIP   = clientIP;
    access.url        = displayUrl;
    access.timestamp  = now;
    access.method     = method;
    emit urlAccessed(access);
}

// ─── Файловый мониторинг лога Tor (вспомогательный) ─────────────────────────

void DnsMonitor::onTorLogOutput()
{
    while (torLogProcess->canReadLine()) {
        QString line = QString::fromUtf8(torLogProcess->readLine()).trimmed();
        if (!line.isEmpty()) buffer.append(line);
    }
}

void DnsMonitor::onTorLogFinished()
{
    emit logMessage("📡 Tor лог-мониторинг остановлен", "warning");
    QTimer::singleShot(5000, this, [this]() { startMonitoring(""); });
}

void DnsMonitor::parseLine(const QString &line)
{
    // Парсинг строк Tor лога: "Resolved address 'google.com' to 142.250.185.46"
    static QRegularExpression torDnsRe(R"(Resolved address '([^']+)' to )");
    auto m = torDnsRe.match(line);
    if (m.hasMatch()) {
        QString hostname = m.captured(1);
        if (hostname.endsWith(".onion")) return;
        QString clientName = (clientIPMap.size() == 1) ? clientIPMap.first() : "unknown";
        QString clientIP   = (clientIPMap.size() == 1) ? clientIPMap.firstKey() : "";
        UrlAccess access;
        access.clientName = clientName;
        access.clientIP   = clientIP;
        access.url        = hostname;
        access.timestamp  = QDateTime::currentDateTime();
        access.method     = "DNS";
        emit urlAccessed(access);
    }
}

// ─── Прочие методы ───────────────────────────────────────────────────────────

void DnsMonitor::setClientMap(const QMap<QString, QString> &ipToClient)
{
    clientIPMap = ipToClient;
}

void DnsMonitor::setLastActiveClient(const QString &name, const QString &ip)
{
    lastActiveClient   = name;
    lastActiveClientIP = ip;
}
