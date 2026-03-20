#include "dns_monitor.h"
#include <QRegularExpression>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QThread>
#include <QNetworkInterface>
#include <QDebug>
#include <QTextStream>
#include <algorithm>
#include <QRegularExpressionMatch>
#include <utility>
#include <QtConcurrent/QtConcurrent>


// ========== КОНСТАНТЫ ==========
#ifndef MAX_RESTART_DELAY
#define MAX_RESTART_DELAY 30000
#endif

#ifndef MIN_RESTART_DELAY
#define MIN_RESTART_DELAY 1000
#endif

#ifndef CONTROL_RECONNECT_DELAY
#define CONTROL_RECONNECT_DELAY 5000
#endif

#ifndef STATS_UPDATE_INTERVAL
#define STATS_UPDATE_INTERVAL 60000
#endif

#ifndef CONNTRACK_POLL_INTERVAL
#define CONNTRACK_POLL_INTERVAL 2000
#endif

#ifndef TCPDUMP_STABILITY_CHECK
#define TCPDUMP_STABILITY_CHECK 10000
#endif

DnsMonitor::DnsMonitor(QObject *parent) : QObject(parent)
{
    // Инициализация категорий доменов
    initDomainCategories();

    // Таймер для conntrack
    conntrackTimer = new QTimer(this);
    conntrackTimer->setInterval(CONNTRACK_POLL_INTERVAL);
    connect(conntrackTimer, &QTimer::timeout, this, &DnsMonitor::pollConntrack);

    // Таймер для сброса логов
    logFlushTimer = new QTimer(this);
    logFlushTimer->setInterval(500);
    connect(logFlushTimer, &QTimer::timeout, this, &DnsMonitor::flushLogBuffer);
    logFlushTimer->start();

    // Таймер для перезапуска tcpdump
    restartTimer = new QTimer(this);
    restartTimer->setSingleShot(true);
    connect(restartTimer, &QTimer::timeout, this, [this]() {
        if (!m_monitoringActive) return;  // остановлен намеренно

        restartAttempts++;

        // Экспоненциальный бэкофф: каждые 5 неудач задержка удваивается.
        // Задержка растёт ГЛОБАЛЬНО и сбрасывается только stabilityTimer
        // (когда tcpdump работает стабильно >= TCPDUMP_STABILITY_CHECK мс).
        if (restartAttempts <= 5) {
            emit logMessage(QString("Перезапуск tcpdump (попытка %1/5, задержка %2мс)...")
                            .arg(restartAttempts).arg(tcpdumpRestartDelay), "warning");
            startMonitoring("");
        } else {
            // Серия из 5 попыток провалилась — увеличиваем задержку для следующей серии
            restartAttempts = 0;
            tcpdumpRestartDelay = qMin(tcpdumpRestartDelay * 2, (int)MAX_RESTART_DELAY);

            if (tcpdumpRestartDelay >= (int)MAX_RESTART_DELAY) {
                emit logMessage(QString("⛔ tcpdump: все попытки исчерпаны. "
                    "Ожидание %1 сек перед следующей серией. "
                    "Проверьте: tcpdump установлен? права cap_net_raw? интерфейс tun0 активен?")
                    .arg(MAX_RESTART_DELAY / 1000), "error");
            } else {
                emit logMessage(QString("tcpdump: следующая серия через %1 сек...")
                    .arg(tcpdumpRestartDelay / 1000), "warning");
            }
            restartTimer->start(tcpdumpRestartDelay);
        }
    });

    // Таймер для проверки стабильности tcpdump
    stabilityTimer = new QTimer(this);
    stabilityTimer->setSingleShot(true);
    connect(stabilityTimer, &QTimer::timeout, this, [this]() {
        if (tcpdumpHasCaptured && tcpdumpProcess && tcpdumpProcess->state() == QProcess::Running) {
            tcpdumpRestartDelay = MIN_RESTART_DELAY;
            restartAttempts = 0;
            m_tcpdumpFastFailCount = 0;
            m_tcpdumpSimpleMode = false;
            emit logMessage("✅ tcpdump стабилен, все счётчики сброшены", "info");
        }
    });

    // Таймер для переподключения ControlPort
    controlReconnectTimer = new QTimer(this);
    controlReconnectTimer->setSingleShot(true);
    connect(controlReconnectTimer, &QTimer::timeout, this, [this]() {
        if (!torControl || torControl->state() == QAbstractSocket::UnconnectedState) {
            emit logMessage("Повторное подключение к Tor ControlPort...", "info");
            startTorController();
        }
    });

    // Таймер для обновления статистики
    statsUpdateTimer = new QTimer(this);
    statsUpdateTimer->setInterval(STATS_UPDATE_INTERVAL);
    connect(statsUpdateTimer, &QTimer::timeout, this, &DnsMonitor::updateStatsTimer);
    statsUpdateTimer->start();
}

DnsMonitor::~DnsMonitor()
{
    stopMonitoring();
}

void DnsMonitor::initDomainCategories()
{
    // Социальные сети
    DomainCategory social;
    social.name = "Социальные сети";
    social.color = "#4267B2"; // Facebook blue
    social.domains = QStringList{
        "facebook.com", "instagram.com", "vk.com", "vk.ru", "ok.ru",
        "twitter.com", "x.com", "t.me", "telegram.org", "whatsapp.net",
        "whatsapp.com", "tiktok.com", "snapchat.com", "pinterest.com",
        "linkedin.com", "discord.com", "reddit.com", "tumblr.com"
    };
    for (const QString &d : social.domains) {
        domainCategories[d] = social;
    }

    // Поисковые системы
    DomainCategory search;
    search.name = "Поиск";
    search.color = "#4285F4"; // Google blue
    search.domains = QStringList{
        "google.com", "yandex.ru", "mail.ru", "rambler.ru", "bing.com",
        "duckduckgo.com", "yahoo.com", "baidu.com"
    };
    for (const QString &d : search.domains) {
        domainCategories[d] = search;
    }

    // Видео и стриминг
    DomainCategory video;
    video.name = "Видео";
    video.color = "#FF0000"; // YouTube red
    video.domains = QStringList{
        "youtube.com", "youtu.be", "twitch.tv", "netflix.com", "kinopoisk.ru",
        "ivi.ru", "rutube.ru", "vimeo.com", "dailymotion.com"
    };
    for (const QString &d : video.domains) {
        domainCategories[d] = video;
    }

    // Реклама и трекеры
    DomainCategory ads;
    ads.name = "Реклама/Трекеры";
    ads.color = "#FFA500"; // Orange
    ads.domains = QStringList{
        "doubleclick.net", "googleadservices.com", "googlesyndication.com",
        "adsrvr.org", "adnxs.com", "rubiconproject.com", "criteo.com",
        "taboola.com", "outbrain.com", "amazon-adsystem.com", "unity3d.com",
        "vungle.com", "applovin.com", "ironsrc.com", "supersonicads.com",
        "chartboost.com", "fyber.com", "heyzap.com", "tapjoy.com",
        "adcolony.com", "inmobi.com", "mopub.com"
    };
    for (const QString &d : ads.domains) {
        domainCategories[d] = ads;
    }

    // Аналитика
    DomainCategory analytics;
    analytics.name = "Аналитика";
    analytics.color = "#9C27B0"; // Purple
    analytics.domains = QStringList{
        "google-analytics.com", "yandex-metrika.ru", "metrica.yandex.net",
        "appmetrica.yandex.net", "firebase.googleapis.com", "crashlytics.com",
        "mixpanel.com", "amplitude.com", "segment.com", "hotjar.com",
        "newrelic.com", "datadoghq.com", "flurry.com", "adjust.com",
        "appsflyer.com", "kochava.com", "branch.io"
    };
    for (const QString &d : analytics.domains) {
        domainCategories[d] = analytics;
    }

    // Apple/iCloud
    DomainCategory apple;
    apple.name = "Apple";
    apple.color = "#555555"; // Gray
    apple.domains = QStringList{
        "apple.com", "icloud.com", "itunes.apple.com", "apps.apple.com",
        "mzstatic.com", "guzzoni.apple.com", "gs-loc.apple.com",
        "ocsp.apple.com", "push.apple.com", "time.apple.com"
    };
    for (const QString &d : apple.domains) {
        domainCategories[d] = apple;
    }

    // Microsoft
    DomainCategory microsoft;
    microsoft.name = "Microsoft";
    microsoft.color = "#00A4EF"; // Microsoft blue
    microsoft.domains = QStringList{
        "microsoft.com", "live.com", "outlook.com", "office.com",
        "office365.com", "skype.com", "linkedin.com", "github.com",
        "visualstudio.com", "azure.com", "xbox.com", "bing.com"
    };
    for (const QString &d : microsoft.domains) {
        domainCategories[d] = microsoft;
    }

    // Подозрительные домены (для предупреждений)
    suspiciousDomains = QStringList{
        "xztbxz.com", "imolive2.com", "buzocq.com", "maxyields.tech",
        "news-cdn.site", "ipof.in", "ipwhois.io", "ifconfig.co",
        "ipapi.co", "ipinfo.io", "ahpi.xztbxz.com", "netsocket.xztbxz.com",
        "logproxy.imoim.app", "gdl.news-cdn.site", "tv-ru.buzocq.com",
        "socks.imolive2.com", "medialbs.imolive2.com", "hw.maxyields.tech"
    };
}

QString DnsMonitor::categorizeDomain(const QString &domain)
{
    QString d = domain.toLower();

    // Проверяем точное совпадение или поддомен
    for (auto it = domainCategories.begin(); it != domainCategories.end(); ++it) {
        if (d.contains(it.key())) {
            return it.value().name;
        }
    }

    // Для обратных записей
    if (d.contains(".in-addr.arpa") || d.contains(".ip6.arpa")) {
        return "Reverse DNS";
    }

    // Для IP-адресов
    static QRegularExpression ipRe(R"(^\d+\.\d+\.\d+\.\d+$)");
    if (ipRe.match(domain).hasMatch()) {
        return "IP Address";
    }

    return "Другое";
}

QString DnsMonitor::getDomainCategoryColor(const QString &category)
{
    static QMap<QString, QString> categoryColors = {
        {"Социальные сети", "#4267B2"},
        {"Поиск", "#4285F4"},
        {"Видео", "#FF0000"},
        {"Реклама/Трекеры", "#FFA500"},
        {"Аналитика", "#9C27B0"},
        {"Apple", "#555555"},
        {"Microsoft", "#00A4EF"},
        {"Reverse DNS", "#7F8C8D"},
        {"IP Address", "#95A5A6"},
        {"Другое", "#34495E"}
    };

    return categoryColors.value(category, "#34495E");
}

bool DnsMonitor::isSuspiciousDomain(const QString &domain)
{
    QString d = domain.toLower();
    for (const QString &susp : suspiciousDomains) {
        if (d.contains(susp)) {
            return true;
        }
    }
    return false;
}

void DnsMonitor::checkSuspiciousActivity(const QString &clientName, const QString &domain)
{
    if (!isSuspiciousDomain(domain)) return;

    QString key = clientName + "|" + domain;
    if (!warnedDomains.contains(key)) {
        warnedDomains.insert(key);
        QString reason = "Подозрительный домен";
        emit suspiciousActivity(clientName, domain, reason);
        emit logMessage(QString("⚠️ ПОДОЗРИТЕЛЬНО: %1 → %2").arg(clientName, domain), "warning");
    }
}

void DnsMonitor::updateClientStats(const QString &clientName, const UrlAccess &access)
{
    QMutexLocker locker(&statsMutex);

    DnsClientStats &stats = clientStats[clientName];

    // Если клиент новый
    if (stats.firstSeen.isNull()) {
        stats.firstSeen = access.timestamp;
        stats.clientName = clientName;
    }

    // Обновляем статистику
    stats.totalRequests++;
    stats.lastRequest = access.timestamp;

    // По методам (HTTPS проверяем до HTTP — "HTTPS (IP)".contains("HTTP") тоже true)
    if (access.method.contains("DNS")) {
        stats.dnsRequests++;
    } else if (access.method.contains("HTTPS")) {
        stats.httpsRequests++;
    } else if (access.method.contains("HTTP")) {
        stats.httpRequests++;
    }

    // По категориям
    QString category = categorizeDomain(access.url);
    stats.categoryCount[category]++;

    // Топ доменов
    stats.topDomains[access.url]++;

    // Проверка на подозрительную активность
    if (isSuspiciousDomain(access.url)) {
        stats.suspiciousRequests++;
        checkSuspiciousActivity(clientName, access.url);
    }

    emit statsUpdated(clientName, stats);
}

void DnsMonitor::updateStatsTimer()
{
    QMutexLocker locker(&statsMutex);

    // Логируем сводку по клиентам каждую минуту
    QStringList summary;
    summary << "📊 Статистика клиентов:";

    for (auto it = clientStats.begin(); it != clientStats.end(); ++it) {
        const DnsClientStats &stats = it.value();
        summary << QString("  • %1: всего %2 запросов (DNS:%3, HTTP:%4, HTTPS:%5)%6")
        .arg(stats.clientName)
        .arg(stats.totalRequests)
        .arg(stats.dnsRequests)
        .arg(stats.httpRequests)
        .arg(stats.httpsRequests)
        .arg(stats.suspiciousRequests > 0 ?
        QString(" ⚠️ %1 подозрительных").arg(stats.suspiciousRequests) : "");
    }

    if (!summary.isEmpty()) {
        for (const QString &line : summary) {
            emit logMessage(line, "info");
        }
    }
}

QMap<QString, DnsClientStats> DnsMonitor::getClientStats() const
{
    QMutexLocker locker(&statsMutex);
    return clientStats;
}

QStringList DnsMonitor::getSuspiciousDomains() const
{
    return suspiciousDomains;
}

void DnsMonitor::generateReport(const QString &clientName)
{
    QMutexLocker locker(&statsMutex);

    QString report;
    QTextStream out(&report);

    out << "📋 ОТЧЕТ ПО DNS МОНИТОРИНГУ\n";
    out << "================================\n";
    out << "Дата: " << QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss") << "\n\n";

    if (clientName.isEmpty()) {
        // Отчет по всем клиентам
        out << "ВСЕ КЛИЕНТЫ:\n";
        out << "------------\n";

        for (auto it = clientStats.begin(); it != clientStats.end(); ++it) {
            const DnsClientStats &stats = it.value();
            out << "\n👤 " << stats.clientName << "\n";
            out << "   Первое подключение: " << stats.firstSeen.toString("dd.MM.yyyy HH:mm") << "\n";
            out << "   Последний запрос: " << stats.lastRequest.toString("dd.MM.yyyy HH:mm") << "\n";
            out << "   Всего запросов: " << stats.totalRequests << "\n";
            out << "   DNS: " << stats.dnsRequests << ", HTTP: " << stats.httpRequests
            << ", HTTPS: " << stats.httpsRequests << "\n";

            if (stats.suspiciousRequests > 0) {
                out << "   ⚠️ Подозрительных запросов: " << stats.suspiciousRequests << "\n";
            }

            out << "\n   Категории:\n";
            for (auto catIt = stats.categoryCount.begin(); catIt != stats.categoryCount.end(); ++catIt) {
                out << QString("      • %1: %2\n").arg(catIt.key()).arg(catIt.value());
            }

            out << "\n   Топ-10 доменов:\n";
            QList<QPair<qint64, QString>> sorted;
            for (auto domIt = stats.topDomains.begin(); domIt != stats.topDomains.end(); ++domIt) {
                sorted.append({domIt.value(), domIt.key()});
            }
            std::sort(sorted.begin(), sorted.end(),
                      [](const auto &a, const auto &b) { return a.first > b.first; });

            int count = 0;
            for (const auto &item : sorted) {
                if (++count > 10) break;
                out << QString("      %1. %2 (%3)\n").arg(count).arg(item.second).arg(item.first);
            }
        }
    } else {
        // Отчет по конкретному клиенту
        if (!clientStats.contains(clientName)) {
            out << "Клиент " << clientName << " не найден в статистике.\n";
        } else {
            const DnsClientStats &stats = clientStats[clientName];
            out << "👤 КЛИЕНТ: " << clientName << "\n";
            out << "==================\n";
            out << "Первое подключение: " << stats.firstSeen.toString("dd.MM.yyyy HH:mm") << "\n";
            out << "Последний запрос: " << stats.lastRequest.toString("dd.MM.yyyy HH:mm") << "\n";
            out << "Всего запросов: " << stats.totalRequests << "\n";
            out << "DNS: " << stats.dnsRequests << ", HTTP: " << stats.httpRequests
            << ", HTTPS: " << stats.httpsRequests << "\n";

            if (stats.suspiciousRequests > 0) {
                out << "⚠️ Подозрительных запросов: " << stats.suspiciousRequests << "\n";
            }

            out << "\nКатегории:\n";
            for (auto catIt = stats.categoryCount.begin(); catIt != stats.categoryCount.end(); ++catIt) {
                out << QString("   • %1: %2\n").arg(catIt.key()).arg(catIt.value());
            }

            out << "\nТоп-20 доменов:\n";
            QList<QPair<qint64, QString>> sorted;
            for (auto domIt = stats.topDomains.begin(); domIt != stats.topDomains.end(); ++domIt) {
                sorted.append({domIt.value(), domIt.key()});
            }
            std::sort(sorted.begin(), sorted.end(),
                      [](const auto &a, const auto &b) { return a.first > b.first; });

            int count = 0;
            for (const auto &item : sorted) {
                if (++count > 20) break;
                QString category = categorizeDomain(item.second);
                QString color = getDomainCategoryColor(category);
                out << QString("   %1. %2 (%3) [%4]\n")
                .arg(count).arg(item.second).arg(item.first).arg(category);
            }
        }
    }

    out << "\n================================\n";
    out << "КОНЕЦ ОТЧЕТА\n";

    emit logMessage(report, "info");
}

// ========== Tor ControlPort методы ==========

void DnsMonitor::startTorController()
{
    if (torControl) {
        torControl->disconnectFromHost();
        torControl->deleteLater();
        torControl = nullptr;
    }

    controlConnecting = true;
    torControl = new QTcpSocket(this);

    connect(torControl, &QTcpSocket::connected, this, &DnsMonitor::onControlSocketConnected);
    connect(torControl, &QTcpSocket::readyRead, this, &DnsMonitor::onControlData);
    connect(torControl, &QTcpSocket::errorOccurred, this, &DnsMonitor::onControlSocketError);
    connect(torControl, &QTcpSocket::disconnected, this, [this]() {
        controlAuthenticated = false;
        controlConnecting = false;
        emit logMessage("ControlPort отключен", "warning");
        controlReconnectTimer->start(CONTROL_RECONNECT_DELAY);
    });

    QTimer::singleShot(10000, this, &DnsMonitor::onControlSocketTimeout);
    torControl->connectToHost("127.0.0.1", 9051);
}

void DnsMonitor::onControlSocketConnected()
{
    controlConnecting = false;
    emit logMessage("✅ Подключено к ControlPort Tor", "info");

    QStringList cookiePaths = {
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tor_data/control_auth_cookie",
        "/var/lib/tor/control_auth_cookie",
        "/run/tor/control.authcookie",
        "/tmp/control_auth_cookie"
    };

    QByteArray cookie;
    for (const QString &path : cookiePaths) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly)) {
            cookie = f.readAll();
            f.close();
            emit logMessage(QString("Найден cookie файл: %1 (%2 байт)").arg(path).arg(cookie.size()), "info");
            break;
        }
    }

    if (!cookie.isEmpty()) {
        sendTorCommand("AUTHENTICATE " + cookie.toHex());
    } else {
        emit logMessage("Cookie не найден, пробуем пустой пароль", "warning");
        sendTorCommand("AUTHENTICATE \"\"");
    }
}

void DnsMonitor::onControlSocketError()
{
    if (torControl) {
        emit logMessage("Ошибка ControlPort: " + torControl->errorString(), "warning");
        controlConnecting = false;
        torControl->deleteLater();
        torControl = nullptr;
        controlReconnectTimer->start(CONTROL_RECONNECT_DELAY);
    }
}

void DnsMonitor::sendTorCommand(const QString &cmd)
{
    if (!torControl || torControl->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    torControl->write((cmd + "\r\n").toUtf8());
}

void DnsMonitor::onControlSocketTimeout()
{
    if (controlConnecting && torControl) {
        emit logMessage("Таймаут ControlPort", "warning");
        torControl->abort();
        controlConnecting = false;
        controlReconnectTimer->start(CONTROL_RECONNECT_DELAY);
    }
}

void DnsMonitor::onControlData()
{
    while (torControl && torControl->canReadLine()) {
        QString line = QString::fromUtf8(torControl->readLine()).trimmed();

        if (line.startsWith("250 OK") && !controlAuthenticated) {
            controlAuthenticated = true;
            sendTorCommand("SETEVENTS ADDRMAP");
            emit logMessage("✅ Tor ControlPort: ADDRMAP мониторинг активен", "info");
            continue;
        }

        if (line.startsWith("515 ") || line.startsWith("551 ")) {
            emit logMessage("Tor control: ошибка аутентификации", "error");
            controlAuthenticated = false;
            continue;
        }

        if (line.contains("ADDRMAP")) {
            parseAddrMap(line);
        }
    }
}

void DnsMonitor::parseAddrMap(const QString &line)
{
    RecursiveMutexLocker locker(&cacheMutex);
    static QRegularExpression re(R"(ADDRMAP\s+(\S+)\s+(\S+)\s+)");
    auto m = re.match(line);
    if (!m.hasMatch()) return;

    QString hostname = m.captured(1);
    QString address = m.captured(2);

    if (address == "<error>" || address == "<unknown>" || hostname.endsWith(".onion")) {
        return;
    }

    static QRegularExpression ipRe(R"(^\d+\.\d+\.\d+\.\d+$)");
    if (!ipRe.match(address).hasMatch()) return;

    CacheEntry entry;
    entry.domain = hostname;
    entry.timestamp = QDateTime::currentDateTime();
    entry.category = categorizeDomain(hostname);
    ipToDomainCache[address] = entry;

    QString clientName = "unknown";
    QString clientIP;

    if (clientIPMap.size() == 1) {
        clientIP = clientIPMap.firstKey();
        clientName = clientIPMap.first();
    } else if (!lastActiveClient.isEmpty()) {
        clientName = lastActiveClient;
        clientIP = lastActiveClientIP;
    }

    if (!clientIP.isEmpty()) {
        UrlAccess access;
        access.clientName = clientName;
        access.clientIP = clientIP;
        access.url = hostname;
        access.timestamp = QDateTime::currentDateTime();
        access.method = "DNS (Tor)";
        access.category = entry.category;
        emit urlAccessed(access);
        updateClientStats(clientName, access);
    }
}

// ========== Основные методы ==========

void DnsMonitor::updateClientMap(const QMap<QString, QString> &ipToClient)
{
    QMutexLocker locker(&clientMapMutex);
    clientIPMap = ipToClient;
}

QString DnsMonitor::detectTunInterface()
{
    QStringList candidates;

    QProcess process;
    process.start("ip", {"-o", "link", "show", "type", "tun"});
    if (process.waitForFinished(3000)) {
        QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        if (!output.isEmpty()) {
            QStringList parts = output.split(':');
            if (parts.size() >= 2) {
                QString iface = parts[1].trimmed();
                candidates << iface;
            }
        }
    }

    process.start("ip", {"-o", "link"});
    if (process.waitForFinished(3000)) {
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        QStringList lines = output.split('\n');
        for (const QString &line : lines) {
            if (line.contains(": tun") && line.contains("UP")) {
                QStringList parts = line.split(':');
                if (parts.size() >= 2) {
                    QString iface = parts[1].trimmed();
                    if (iface.startsWith("tun")) {
                        candidates << iface;
                    }
                }
            }
        }
    }

    QFile f("/proc/net/dev");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!f.atEnd()) {
            QString line = QString::fromUtf8(f.readLine()).trimmed();
            if (line.startsWith("tun") && line.contains(':')) {
                QString iface = line.split(':').first().trimmed();
                if (iface.startsWith("tun")) {
                    candidates << iface;
                }
            }
        }
        f.close();
    }

    for (const QString &iface : candidates) {
        QProcess checkProc;
        checkProc.start("ip", {"link", "show", iface});
        if (checkProc.waitForFinished(2000)) {
            QString out = QString::fromUtf8(checkProc.readAllStandardOutput());
            if (out.contains("UP") || out.contains("UNKNOWN")) {
                emit logMessage(QString("Найден tun интерфейс: %1").arg(iface), "info");
                return iface;
            }
        }
    }

    return QString();
}

void DnsMonitor::startMonitoring(const QString &vpnNetwork)
{
    Q_UNUSED(vpnNetwork)
    m_monitoringActive = true;  // разрешаем авторестарт

    // Если tcpdump уже запущен и работает — не перезапускаем.
    // Это предотвращает гонку когда MainWindow и restartTimer вызывают
    // startMonitoring одновременно и убивают только что запущенный процесс.
    if (tcpdumpProcess && tcpdumpProcess->state() == QProcess::Running) {
        return;  // уже работает — ничего не делаем
    }

    QString tunIface = detectTunInterface();

    if (!tunIface.isEmpty()) {
        if (tcpdumpProcess) {
            if (tcpdumpProcess->state() == QProcess::Running) {
                tcpdumpProcess->terminate();
                tcpdumpProcess->waitForFinished(2000);
            }
            tcpdumpProcess->deleteLater();
            tcpdumpProcess = nullptr;
        }

        tcpdumpProcess = new QProcess(this);
        tcpdumpProcess->setWorkingDirectory("/");
        tcpdumpProcess->setProcessChannelMode(QProcess::MergedChannels);

        connect(tcpdumpProcess, &QProcess::readyReadStandardOutput,
                this, &DnsMonitor::onTcpdumpOutput);
        connect(tcpdumpProcess, &QProcess::readyReadStandardError,
                this, &DnsMonitor::onTcpdumpError);
        connect(tcpdumpProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &DnsMonitor::onTcpdumpFinished);

        // Базовый фильтр — только трафик клиентов VPN
        // Исключаем трафик между VPN-сервером и Tor (10.8.0.1) чтобы
        // не захватывать служебные соединения
        QString bpfFilter = QString(
            "(udp port 53 or tcp port 80 or tcp port 443)"
            " and not host 10.8.0.1"
        );

        QStringList args;
        args << "-i" << tunIface
             << "-l"          // line-buffered: вывод без буферизации
             << "-n"          // без reverse DNS внутри tcpdump
             << "-tt";        // unix timestamp вместо читаемого формата

        if (!m_tcpdumpSimpleMode) {
            // Полный режим: захватываем payload для HTTP Host и TLS SNI
            args << "-s" << "512"   // достаточно для DNS + HTTP Host + TLS ClientHello
                 << "-A";           // ASCII для HTTP Host header
        } else {
            // Упрощённый режим: только заголовки, без payload
            // Используется после 3 быстрых сбоев в полном режиме
            emit logMessage("ℹ️ tcpdump: упрощённый режим (только заголовки)", "info");
            args << "-s" << "96";   // только IP+TCP заголовки
        }

        args << bpfFilter;

        emit logMessage(QString("Запуск tcpdump на интерфейсе: %1%2")
                        .arg(tunIface)
                        .arg(m_tcpdumpSimpleMode ? " [упрощённый]" : ""), "info");

        m_tcpdumpStartTime = QDateTime::currentMSecsSinceEpoch();
        tcpdumpProcess->start("tcpdump", args);

        if (tcpdumpProcess->waitForStarted(3000)) {
            emit logMessage("📡 DNS/HTTPS сниффер запущен", "info");
            stabilityTimer->start(TCPDUMP_STABILITY_CHECK);
        } else {
            QString error = tcpdumpProcess->errorString();
            emit logMessage("⚠️ tcpdump не удалось запустить: " + error + 
                            " — убедитесь что tcpdump установлен и имеет права cap_net_raw", "warning");
            // Проверяем существование бинарника
            QProcess which;
            which.start("which", {"tcpdump"});
            which.waitForFinished(1000);
            if (which.exitCode() != 0) {
                emit logMessage("❌ tcpdump не найден! Установите: apt install tcpdump", "error");
            } else {
                QString tcpdumpBin = QString::fromUtf8(which.readAllStandardOutput()).trimmed();
                emit logMessage("tcpdump найден: " + tcpdumpBin + 
                                " — проверьте права: setcap cap_net_raw,cap_net_admin=eip " + tcpdumpBin,
                                "warning");
            }
            restartTimer->start(tcpdumpRestartDelay);
        }
    } else {
        emit logMessage("⚠️ tun-интерфейс не найден, мониторинг URL недоступен", "warning");
        restartTimer->start(tcpdumpRestartDelay);
        tcpdumpRestartDelay = qMin(tcpdumpRestartDelay * 2, MAX_RESTART_DELAY);
    }

    if (!torControl || torControl->state() == QAbstractSocket::UnconnectedState) {
        startTorController();
    }

    if (!conntrackTimer->isActive()) {
        if (QFile::exists("/proc/net/nf_conntrack")) {
            conntrackTimer->start();
            emit logMessage("📡 Conntrack-мониторинг активен", "info");
        } else if (QFile::exists("/proc/net/ip_conntrack")) {
            useIpConntrack = true;
            conntrackTimer->start();
            emit logMessage("📡 ip_conntrack-мониторинг активен", "info");
        }
    }
}

void DnsMonitor::stopMonitoring()
{
    m_monitoringActive = false;  // запрещаем авторестарт

    if (tcpdumpProcess) {
        if (tcpdumpProcess->state() == QProcess::Running) {
            tcpdumpProcess->terminate();
            if (!tcpdumpProcess->waitForFinished(3000)) {
                tcpdumpProcess->kill();
            }
        }
        tcpdumpProcess->deleteLater();
        tcpdumpProcess = nullptr;
    }

    restartTimer->stop();
    stabilityTimer->stop();

    tcpdumpHasCaptured = false;
    tcpdumpRestartDelay = MIN_RESTART_DELAY;
    restartAttempts = 0;

    if (torControl) {
        torControl->disconnectFromHost();
        torControl->deleteLater();
        torControl = nullptr;
    }
    controlAuthenticated = false;
    controlConnecting = false;
    controlReconnectTimer->stop();

    if (conntrackTimer) {
        conntrackTimer->stop();
    }

    seenConntrackKeys.clear();
    {
        RecursiveMutexLocker locker(&cacheMutex);
        ipToDomainCache.clear();
        seenUrlTimestamps.clear();
    }

    for (auto *timer : pendingReverseLookups) {
        timer->stop();
        timer->deleteLater();
    }
    pendingReverseLookups.clear();

    cleanupOldEntries();
}

void DnsMonitor::onTcpdumpError()
{
    if (tcpdumpProcess) {
        QString error = QString::fromUtf8(tcpdumpProcess->readAllStandardError()).trimmed();
        if (!error.isEmpty()) {
            QMutexLocker locker(&logMutex);

            if (error.contains("No such device")) {
                logBuffer.append("⚠️ tcpdump: интерфейс не найден");
                QString tunIface = detectTunInterface();
                if (!tunIface.isEmpty()) {
                    logBuffer.append("✅ Найден интерфейс: " + tunIface);
                }
            } else if (error.contains("Permission denied")) {
                logBuffer.append("⚠️ tcpdump: нет прав. Нужны cap_net_raw,cap_net_admin");
            } else if (!error.contains("packets captured") &&
                !error.contains("packets received") &&
                !error.contains("dropped by kernel")) {
                logBuffer.append("tcpdump: " + error);
                }
        }
    }
}

void DnsMonitor::onTcpdumpOutput()
{
    // Обрабатываем строки прямо в слоте — без QtConcurrent.
    // tcpdump без -XX даёт ~100 байт/строку; парсинг регулярками быстр.
    // Ограничение партии защищает от временных зависаний UI при burst-трафике.
    const int MAX_LINES_PER_BATCH = 200;
    int processed = 0;

    while (tcpdumpProcess && tcpdumpProcess->canReadLine() && processed < MAX_LINES_PER_BATCH) {
        QString line = QString::fromUtf8(tcpdumpProcess->readLine()).trimmed();
        if (!line.isEmpty()) {
            // Отмечаем что tcpdump жив и получает пакеты
            tcpdumpHasCaptured = true;
            lastPacketTime = QDateTime::currentMSecsSinceEpoch();
            parseTcpdumpLine(line);
        }
        processed++;
    }

    // Если в буфере ещё есть данные — отдаём управление event loop и продолжим
    if (tcpdumpProcess && tcpdumpProcess->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, &DnsMonitor::onTcpdumpOutput, Qt::QueuedConnection);
    }
}

void DnsMonitor::onTcpdumpFinished()
{
    if (!tcpdumpProcess) return;

    int exitCode = tcpdumpProcess->exitCode();
    QProcess::ExitStatus exitStatus = tcpdumpProcess->exitStatus();
    qint64 uptime = QDateTime::currentMSecsSinceEpoch() - m_tcpdumpStartTime;

    // Читаем весь накопленный вывод (stderr смерджен в stdout)
    QString output = QString::fromUtf8(tcpdumpProcess->readAllStandardOutput()).trimmed();

    // Всегда логируем детали завершения для диагностики
    if (exitStatus == QProcess::CrashExit) {
        emit logMessage(QString("❌ tcpdump упал (сигнал), uptime=%1мс").arg(uptime), "warning");
    } else if (exitCode != 0) {
        emit logMessage(QString("❌ tcpdump завершился с ошибкой (код %1), uptime=%2мс")
                        .arg(exitCode).arg(uptime), "warning");
    }

    // Логируем вывод tcpdump если есть — там причина ошибки
    if (!output.isEmpty()) {
        // Разбиваем на строки, логируем каждую
        for (const QString &line : output.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
            QString t = line.trimmed();
            if (!t.isEmpty())
                emit logMessage(QString("tcpdump: %1").arg(t), "warning");
        }
    }

    if (!m_monitoringActive) {
        return; // остановлен намеренно — не перезапускаем
    }

    // Если tcpdump жил меньше 2 секунд — это системная проблема (права, нет интерфейса, etc.)
    // Пробуем диагностику и альтернативные параметры запуска
    if (uptime < 2000 && exitCode != 0) {
        m_tcpdumpFastFailCount++;
        emit logMessage(QString("⚠️ tcpdump быстро завершился (%1 мс), быстрых сбоев: %2")
                        .arg(uptime).arg(m_tcpdumpFastFailCount), "warning");

        // После 3 быстрых сбоев подряд — переключаемся на упрощённый режим
        if (m_tcpdumpFastFailCount >= 3) {
            emit logMessage("🔄 Переключение на упрощённый режим tcpdump (без -A и -s)", "info");
            m_tcpdumpSimpleMode = true;
            m_tcpdumpFastFailCount = 0;
        }
    } else if (uptime >= 2000) {
        // Работал нормально — сбрасываем счётчик быстрых сбоев
        m_tcpdumpFastFailCount = 0;
        m_tcpdumpSimpleMode = false;
    }

    if (tcpdumpHasCaptured) {
        tcpdumpRestartDelay = MIN_RESTART_DELAY;
    } else {
        tcpdumpRestartDelay = qMin(tcpdumpRestartDelay * 2, (int)MAX_RESTART_DELAY);
    }

    tcpdumpHasCaptured = false;

    emit logMessage(QString("tcpdump завершился, перезапуск через %1 мс...")
                    .arg(tcpdumpRestartDelay), "warning");
    restartTimer->start(tcpdumpRestartDelay);
}

void DnsMonitor::flushLogBuffer()
{
    QMutexLocker locker(&logMutex);
    for (const QString &msg : std::as_const(logBuffer)) {
        emit logMessage(msg, "info");
    }
    logBuffer.clear();
}

bool DnsMonitor::isIgnoredDomain(const QString &domain)
{
    static const QSet<QString> ignored = {
        "localhost", "localdomain", "local",
        "ip6.arpa", "in-addr.arpa",
        "8.8.8.8.in-addr.arpa", "8.8.4.4.in-addr.arpa"
    };

    QString d = domain.toLower();
    for (const QString &ignore : ignored) {
        if (d.contains(ignore)) return true;
    }

    return false;
}

QString DnsMonitor::extractDomainFromReverse(const QString &ptr)
{
    if (!ptr.contains(".in-addr.arpa") && !ptr.contains(".ip6.arpa")) {
        return ptr;
    }
    return "REVERSE[" + ptr + "]";
}

void DnsMonitor::reverseLookup(const QString &ip, const QString &srcIP, const QString &method)
{
    // Не делаем PTR-запросы для VPN-адресов (10.8.0.x) и приватных сетей —
    // они уходят через Tor DNS и порождают "ill-formed reverse lookup"
    if (ip.startsWith("10.") || ip.startsWith("192.168.") ||
        ip.startsWith("172.16.") || ip.startsWith("127.") ||
        ip.startsWith("::1") || ip.isEmpty()) {
        return;
    }

    // Не дублируем запросы: если уже ожидаем ответ для этого IP — выходим
    if (pendingReverseLookups.contains(ip)) return;

    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->start(10000); // Таймаут 10 сек

    connect(timer, &QTimer::timeout, this, [this, ip]() {
        if (pendingReverseLookups.contains(ip)) {
            pendingReverseLookups[ip]->deleteLater();
            pendingReverseLookups.remove(ip);
        }
    });

    pendingReverseLookups[ip] = timer;

    QHostInfo::lookupHost(ip, this, [this, ip, srcIP, method, timer](const QHostInfo &hostInfo) {
        timer->stop();
        timer->deleteLater();
        pendingReverseLookups.remove(ip);
        if (hostInfo.error() == QHostInfo::NoError) {
            QString hostname = hostInfo.hostName();
            if (!hostname.isEmpty() && !isIgnoredDomain(hostname)) {
                CacheEntry entry;
                entry.domain = hostname;
                entry.timestamp = QDateTime::currentDateTime();
                entry.category = categorizeDomain(hostname);
                ipToDomainCache[ip] = entry;

                emitUrlAccess(srcIP, hostname, method + " (PTR)");
            }
        }
    });
}

void DnsMonitor::parseTcpdumpLine(const QString &line)
{
    // 1. DNS запросы (A, AAAA, ANY и т.д.)
    static QRegularExpression dnsQueryRe(
        R"(IP\s+(\d+\.\d+\.\d+\.\d+)\.\d+\s+>\s+\S+\.53:\s+(\d+)\+\s+([A-Z]+)\?\s+([^\s]+)\.?\s*\((\d+)\))"
    );
    auto m = dnsQueryRe.match(line);
    if (m.hasMatch()) {
        QString srcIP = m.captured(1);
        QString domain = m.captured(4);
        QString qtype = m.captured(3);

        if (domain.endsWith('.')) {
            domain.chop(1);
        }

        if (!domain.isEmpty() && !isIgnoredDomain(domain)) {
            QMutexLocker locker(&logMutex);
            logBuffer.append(QString("DNS запрос: %1 -> %2 (%3)").arg(srcIP, domain, qtype));

            // Определяем метод (DNS-over-HTTPS или обычный DNS)
            QString method = (qtype == "HTTPS") ? "DNS-over-HTTPS" : "DNS";
            emitUrlAccess(srcIP, domain, method);
        }
        return;
    }

    // 2. DNS ответы (извлекаем IP -> домен для кэша)
    static QRegularExpression dnsAnswerRe(
        R"(IP\s+\S+\.53\s+>\s+(\d+\.\d+\.\d+\.\d+)\.\d+:\s+\d+\s+\d+/\d+/\d+\s+[A-Z]+\s+(\d+\.\d+\.\d+\.\d+))"
    );
    auto mAns = dnsAnswerRe.match(line);
    if (mAns.hasMatch()) {
        QString clientIP = mAns.captured(1);
        QString resolvedIP = mAns.captured(2);

        // Ищем домен в предыдущих запросах (будет добавлен позже через ADDRMAP)
        // Пока просто запоминаем, что этот IP был в ответе
        return;
    }

    // 3. TLS SNI — получается через ADDRMAP из Tor ControlPort (надёжнее hex-парсинга)
    // Hex-парсинг удалён: требовал флаг -XX у tcpdump, который переполнял pipe и
    // вызывал постоянные перезапуски. Домены для HTTPS резолвятся через Tor ADDRMAP.

        // 4. HTTP запросы (порт 80)
        // tcpdump -A выводит пакет многострочно: сначала IP/Flags, потом payload.
        // Поэтому отслеживаем srcIP по TCP SYN/PSH строкам, затем ловим "Host:" на следующих.
        static QRegularExpression httpPktRe(
            R"(IP\s+(\d+\.\d+\.\d+\.\d+)\.\d+\s+>\s+(\d+\.\d+\.\d+\.\d+)\.80:\s+Flags\s+\[)"
        );
        auto mHttpPkt = httpPktRe.match(line);
        if (mHttpPkt.hasMatch()) {
            m_lastTcpSrcIP = mHttpPkt.captured(1);
            m_lastTcpDstIP = mHttpPkt.captured(2);
            m_inHttpPayload = line.contains("[P.");
            return;
        }
        // Host: header строка (приходит в payload после IP-строки)
        if (m_inHttpPayload && !m_lastTcpSrcIP.isEmpty()) {
            static QRegularExpression hostRe(R"(^Host:\s*([^
\s]+))");
            auto mHost = hostRe.match(line);
            if (mHost.hasMatch()) {
                QString host = mHost.captured(1).trimmed();
                if (!host.isEmpty() && !isIgnoredDomain(host)) {
                    emitUrlAccess(m_lastTcpSrcIP, host, "HTTP");
                }
                m_inHttpPayload = false;
                m_lastTcpSrcIP.clear();
                return;
            }
            // Если встретили пустую строку или конец заголовков — сбрасываем
            if (line.isEmpty() || line.startsWith("IP ")) {
                m_inHttpPayload = false;
            }
            return;
        }

    // 5. HTTPS соединения (SYN to 443)
    static QRegularExpression tcpConnRe(
        R"(IP\s+(\d+\.\d+\.\d+\.\d+)\.\d+\s+>\s+(\d+\.\d+\.\d+\.\d+)\.443:\s+Flags\s+\[S\])"
    );
    auto mTcp = tcpConnRe.match(line);
    if (mTcp.hasMatch()) {
        QString srcIP = mTcp.captured(1);
        QString dstIP = mTcp.captured(2);

        // Игнорируем локальные адреса
        if (dstIP.startsWith("10.") || dstIP.startsWith("192.168.") ||
            dstIP.startsWith("127.") || dstIP.startsWith("172.16.")) {
            return;
            }

        QString displayUrl = dstIP;
        {
            RecursiveMutexLocker locker(&cacheMutex);
            if (ipToDomainCache.contains(dstIP)) {
                displayUrl = ipToDomainCache[dstIP].domain;
            }
        }
        if (displayUrl != dstIP) {
            emitUrlAccess(srcIP, displayUrl, "HTTPS");
        } else {
            // Если домена нет в кэше, показываем IP и запускаем reverse lookup
            emitUrlAccess(srcIP, displayUrl, "HTTPS (IP)");
            // reverseLookup создаёт QTimer и QHostInfo — только из главного потока
            QMetaObject::invokeMethod(this, [this, dstIP, srcIP]() {
                reverseLookup(dstIP, srcIP, "HTTPS");
            }, Qt::QueuedConnection);
        }
        return;
    }
}

void DnsMonitor::parseTlsSni(const QByteArray &packet, const QString &srcIP)
{
    // Структура TLS Record (RFC 5246 / RFC 8446):
    // [0]      Content Type:    0x16 = Handshake
    // [1..2]   Version:         0x0301 (TLS 1.0) .. 0x0303 (TLS 1.2)
    // [3..4]   Record Length
    // [5]      Handshake Type:  0x01 = ClientHello
    // [6..8]   Handshake Length (3 bytes, big-endian)
    // [9..10]  Client Version
    // [11..42] Random (32 bytes)
    // [43]     Session ID Length
    //          Session ID (0..32 bytes)
    //          Cipher Suites Length (2 bytes)
    //          Cipher Suites
    //          Compression Methods Length (1 byte)
    //          Compression Methods
    //          Extensions Length (2 bytes)
    //          Extensions: type(2) + len(2) + data  (repeated)

    if (packet.size() < 43) return;

    const auto byte_ = [&](int p) -> int {
        return static_cast<unsigned char>(packet[p]);
    };
    const auto u16_ = [&](int p) -> int {
        return (byte_(p) << 8) | byte_(p + 1);
    };

    // --- TLS Record Layer ---
    if (byte_(0) != 0x16) return;    // Content Type: Handshake
    if (byte_(1) != 0x03) return;    // Major version must be 3 (SSLv3/TLS)

    int recordLen = u16_(3);
    if (packet.size() < 5 + recordLen) return;

    // --- Handshake Layer ---
    if (byte_(5) != 0x01) return;    // HandshakeType: ClientHello

    int hsLen = (byte_(6) << 16) | (byte_(7) << 8) | byte_(8);
    if (hsLen < 34 || packet.size() < 9 + hsLen) return;

    // --- ClientHello Body, starts at offset 9 ---
    int pos = 9;
    pos += 2;  // skip ClientVersion

    if (pos + 32 > packet.size()) return;
    pos += 32; // skip Random

    // Session ID
    if (pos + 1 > packet.size()) return;
    int sessionIdLen = byte_(pos++);
    if (sessionIdLen > 32 || pos + sessionIdLen > packet.size()) return;
    pos += sessionIdLen;

    // Cipher Suites
    if (pos + 2 > packet.size()) return;
    int cipherSuitesLen = u16_(pos);
    pos += 2;
    if (pos + cipherSuitesLen > packet.size()) return;
    pos += cipherSuitesLen;

    // Compression Methods
    if (pos + 1 > packet.size()) return;
    int compressionLen = byte_(pos++);
    if (pos + compressionLen > packet.size()) return;
    pos += compressionLen;

    // --- Extensions ---
    if (pos + 2 > packet.size()) return;
    int extensionsLen = u16_(pos);
    pos += 2;

    int extensionsEnd = pos + extensionsLen;
    if (extensionsEnd > packet.size()) return; // truncated — не парсим неполные данные

    while (pos + 4 <= extensionsEnd) {
        int extType = u16_(pos);
        int extLen  = u16_(pos + 2);
        pos += 4;

        if (pos + extLen > extensionsEnd) break;

        // Extension Type 0x0000 = server_name (RFC 6066)
        if (extType == 0x0000) {
            int p = pos;

            // ServerNameList length
            if (p + 2 > pos + extLen) break;
            int listLen = u16_(p);
            p += 2;

            int listEnd = p + listLen;
            if (listEnd > pos + extLen) break;

            while (p + 3 <= listEnd) {
                int nameType = byte_(p);     // 0x00 = host_name
                int nameLen  = u16_(p + 1);
                p += 3;

                if (p + nameLen > listEnd) break;

                if (nameType == 0x00 && nameLen > 0 && nameLen <= 255) {
                    QString domain = QString::fromLatin1(packet.mid(p, nameLen));

                    if (!domain.isEmpty() && !isIgnoredDomain(domain)) {
                        {
                            RecursiveMutexLocker locker(&cacheMutex);
                            CacheEntry entry;
                            entry.domain    = domain;
                            entry.timestamp = QDateTime::currentDateTime();
                            entry.category  = categorizeDomain(domain);
                            ipToDomainCache[srcIP] = entry;
                        }
                        emitUrlAccess(srcIP, domain, "HTTPS (SNI)");
                    }
                    return; // host_name найден — выходим
                }
                p += nameLen;
            }
            break; // server_name extension обработан
        }

        pos += extLen; // перейти к следующему расширению
    }
}

void DnsMonitor::pollConntrack()
{
    QString path = useIpConntrack ? "/proc/net/ip_conntrack" : "/proc/net/nf_conntrack";
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    static QRegularExpression connRe(
        R"(src=(\d+\.\d+\.\d+\.\d+)\s+dst=(\d+\.\d+\.\d+\.\d+)\s+sport=\d+\s+dport=(\d+))"
    );

    QSet<QString> currentKeys;
    QDateTime now = QDateTime::currentDateTime();

    while (!f.atEnd()) {
        QString line = QString::fromUtf8(f.readLine());

        if (!line.contains("ESTABLISHED") && !line.contains("TIME_WAIT")) continue;

        auto m = connRe.match(line);
        if (!m.hasMatch()) continue;

        QString src = m.captured(1);
        QString dst = m.captured(2);
        QString dport = m.captured(3);

        if (!isVpnClient(src)) continue;

        if (dst.startsWith("10.") || dst.startsWith("192.168.") ||
            dst.startsWith("172.16.") || dst.startsWith("127.")) continue;

        if (dport != "53" && dport != "80" && dport != "443") continue;

        QString key = src + "|" + dst + "|" + dport;
        currentKeys.insert(key);

        if (!seenConntrackKeys.contains(key)) {
            seenConntrackKeys.insert(key);

            QString method;
            if (dport == "53") method = "DNS";
            else if (dport == "443") method = "HTTPS";
            else method = "HTTP";

            QString displayUrl = dst;
            {
                RecursiveMutexLocker locker(&cacheMutex);
                if (ipToDomainCache.contains(dst)) {
                    displayUrl = ipToDomainCache[dst].domain;
                }
            }
            if (displayUrl != dst) {
                emitUrlAccess(src, displayUrl, method);
            } else {
                emitUrlAccess(src, displayUrl, method + " (IP)");
                reverseLookup(dst, src, method);
            }
        }
    }
    f.close();

    seenConntrackKeys.intersect(currentKeys);
}

bool DnsMonitor::isVpnClient(const QString &ip) const
{
    if (clientIPMap.contains(ip)) return true;
    if (ip.startsWith("10.8.0.") && ip != "10.8.0.1") return true;
    return false;
}

void DnsMonitor::emitUrlAccess(const QString &srcIP, const QString &url, const QString &method)
{
    static QRegularExpression ipRe(R"(^\d+\.\d+\.\d+\.\d+$)");
    QDateTime now = QDateTime::currentDateTime();
    QString displayUrl = url;
    QString clientName;

    // --- критическая секция: читаем/пишем кэши ---
    {
        RecursiveMutexLocker cacheLocker(&cacheMutex);
        QMutexLocker mapLocker(&clientMapMutex);

        clientName = clientIPMap.value(srcIP, "unknown");

        // Подставляем домен по IP если известен
        if (ipRe.match(url).hasMatch() && ipToDomainCache.contains(url)) {
            displayUrl = ipToDomainCache[url].domain;
        }

        // Дедупликация
        QString dedupeKey = srcIP + "|" + displayUrl + "|" + method;
        if (seenUrlTimestamps.contains(dedupeKey)) {
            if (seenUrlTimestamps[dedupeKey].secsTo(now) < DEDUP_SECONDS) {
                return;
            }
        }
        seenUrlTimestamps[dedupeKey] = now;

        // Обратное кэширование IP->домен
        if (ipRe.match(url).hasMatch() && displayUrl != url) {
            CacheEntry entry;
            entry.domain    = displayUrl;
            entry.timestamp = now;
            entry.category  = categorizeDomain(displayUrl);
            ipToDomainCache[url] = entry;
        }

        cleanupOldEntries(); // cacheMutex рекурсивный — не дедлок
    }
    // --- блокировки сняты: emit и обновление статистики без холдинга кэш-мьютексов ---

    UrlAccess access;
    access.clientName = clientName;
    access.clientIP   = srcIP;
    access.url        = displayUrl;
    access.timestamp  = now;
    access.method     = method;
    access.category   = categorizeDomain(displayUrl);

    emit urlAccessed(access);
    updateClientStats(clientName, access);
}

void DnsMonitor::cleanupOldEntries()
{
    RecursiveMutexLocker locker(&cacheMutex);
    QDateTime now = QDateTime::currentDateTime();
    QDateTime cutoff = now.addSecs(-300); // 5 минут назад

    // Очищаем URL timestamp кэш по времени
    auto it = seenUrlTimestamps.begin();
    while (it != seenUrlTimestamps.end()) {
        if (it.value() < cutoff) {
            it = seenUrlTimestamps.erase(it);
        } else {
            ++it;
        }
    }

    // Очищаем conntrack кэш (удаляем 10% за раз если превышен лимит)
    if (seenConntrackKeys.size() > 5000) {
        int toRemove = seenConntrackKeys.size() / 10;
        auto cit = seenConntrackKeys.begin();
        while (cit != seenConntrackKeys.end() && toRemove > 0) {
            cit = seenConntrackKeys.erase(cit);
            toRemove--;
        }
    }

    // Очищаем IP->Domain кэш по timestamp
    auto dit = ipToDomainCache.begin();
    while (dit != ipToDomainCache.end()) {
        if (dit.value().timestamp < cutoff) {
            dit = ipToDomainCache.erase(dit);
        } else {
            ++dit;
        }
    }
}

void DnsMonitor::setClientMap(const QMap<QString, QString> &ipToClient)
{
    QMutexLocker locker(&clientMapMutex);
    clientIPMap = ipToClient;
}

void DnsMonitor::setLastActiveClient(const QString &name, const QString &ip)
{
    lastActiveClient = name;
    lastActiveClientIP = ip;
}
