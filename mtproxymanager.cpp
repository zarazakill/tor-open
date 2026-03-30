// mtproxymanager.cpp
#include "mtproxymanager.h"
#include <QRegularExpression>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QHeaderView>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

QString MTProxyManager::getExternalIp()
{
    // Пробуем ifconfig.me
    QProcess proc;
    proc.start("curl", {"-s", "--max-time", "5", "ifconfig.me"});
    if (proc.waitForFinished(5000) && proc.exitCode() == 0) {
        QString ip = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (!ip.isEmpty() && !ip.startsWith("127.")) {
            return ip;
        }
    }

    // Пробуем icanhazip.com
    proc.start("curl", {"-s", "--max-time", "5", "icanhazip.com"});
    if (proc.waitForFinished(5000) && proc.exitCode() == 0) {
        QString ip = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (!ip.isEmpty() && !ip.startsWith("127.")) {
            return ip;
        }
    }

    // Пробуем через ip route
    proc.start("ip", {"route", "get", "1.1.1.1"});
    if (proc.waitForFinished(3000) && proc.exitCode() == 0) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput());
        QRegularExpression re("src (\\d+\\.\\d+\\.\\d+\\.\\d+)");
        auto match = re.match(output);
        if (match.hasMatch()) {
            QString ip = match.captured(1);
            if (!ip.startsWith("127.") && !ip.startsWith("10.") && !ip.startsWith("192.168.")) {
                return ip;
            }
        }
    }

    return QString();
}

QString MTProxyManager::getInternalIp()
{
    QProcess proc;
    proc.start("ip", {"-4", "addr", "show"});
    if (!proc.waitForFinished(3000)) {
        return QString();
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput());

    // Ищем не loopback интерфейс (eth, enp, ens, wlan, wlp)
    QRegularExpression re("(?:eth|enp|ens|wlan|wlp)(?:\\S+):.*?inet (\\d+\\.\\d+\\.\\d+\\.\\d+)/");
    auto match = re.match(output);
    if (match.hasMatch()) {
        return match.captured(1);
    }

    // Fallback: любой IP кроме 127.0.0.1 и 10.8.0.1 (VPN)
    QRegularExpression fallbackRe("inet (\\d+\\.\\d+\\.\\d+\\.\\d+)/");
    QRegularExpressionMatchIterator it = fallbackRe.globalMatch(output);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString ip = m.captured(1);
        if (!ip.startsWith("127.") && !ip.startsWith("10.8.")) {
            return ip;
        }
    }

    return QString();
}

void MTProxyManager::setupFirewallRules(int port)
{
    // Разрешаем входящие соединения на порт (INPUT chain)
    QProcess checkInput;
    checkInput.start("iptables", {"-C", "INPUT", "-p", "tcp", "--dport", QString::number(port), "-j", "ACCEPT"});
    checkInput.waitForFinished(2000);

    if (checkInput.exitCode() != 0) {
        QProcess addInput;
        addInput.start("iptables", {"-A", "INPUT", "-p", "tcp", "--dport", QString::number(port), "-j", "ACCEPT"});
        addInput.waitForFinished(2000);
        if (addInput.exitCode() == 0) {
            emit logMessage("✅ Правило INPUT добавлено для порта " + QString::number(port), "success");
        } else {
            QString err = QString::fromUtf8(addInput.readAllStandardError());
            emit logMessage("⚠️ Не удалось добавить INPUT правило: " + err, "warning");
        }
    }

    // UFW поддержка
    QProcess ufwCheck;
    ufwCheck.start("which", {"ufw"});
    ufwCheck.waitForFinished(2000);
    if (ufwCheck.exitCode() == 0) {
        QProcess ufwAllow;
        ufwAllow.start("ufw", {"allow", QString::number(port) + "/tcp"});
        ufwAllow.waitForFinished(5000);
        if (ufwAllow.exitCode() == 0) {
            emit logMessage("✅ UFW правило добавлено для порта " + QString::number(port), "success");
        }
    }
}

void MTProxyManager::removeFirewallRules(int port)
{
    // Удаляем правило INPUT
    QProcess delInput;
    delInput.start("iptables", {"-D", "INPUT", "-p", "tcp", "--dport", QString::number(port), "-j", "ACCEPT"});
    delInput.waitForFinished(2000);
    if (delInput.exitCode() == 0) {
        emit logMessage("✅ Правило INPUT удалено для порта " + QString::number(port), "info");
    }

    // Удаляем UFW правило
    QProcess ufwDelete;
    ufwDelete.start("ufw", {"delete", "allow", QString::number(port) + "/tcp"});
    ufwDelete.waitForFinished(5000);
}

// ==================== MTProxyManager Implementation ====================

MTProxyManager::MTProxyManager(QObject *parent)
: QObject(parent)
{
    mtprotoProcess = new QProcess(this);
    statsTimer = new QTimer(this);
    cleanupTimer = new QTimer(this);
    geoManager = new QNetworkAccessManager(this);

    // Настройка таймеров
    statsTimer->setInterval(STATS_UPDATE_INTERVAL);
    cleanupTimer->setInterval(CLEANUP_INTERVAL);

    // Подключение сигналов
    connect(mtprotoProcess, &QProcess::readyReadStandardOutput,
            this, &MTProxyManager::onProxyProcessOutput);
    connect(mtprotoProcess, &QProcess::readyReadStandardError,
            this, &MTProxyManager::onProxyProcessError);
    connect(mtprotoProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MTProxyManager::onProxyProcessFinished);
    connect(statsTimer, &QTimer::timeout, this, &MTProxyManager::updateStats);
    connect(cleanupTimer, &QTimer::timeout, this, &MTProxyManager::cleanupInactiveConnections);
    connect(geoManager, &QNetworkAccessManager::finished,
            this, &MTProxyManager::onGeoLookupFinished);

    // Инициализация БД
    initDatabase();

    // Пути по умолчанию
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    configPath = appData + "/mtproxy.conf";
    QDir().mkpath(appData);
}

MTProxyManager::~MTProxyManager()
{
    stopProxy();
    if (db.isOpen()) db.close();
}

bool MTProxyManager::initDatabase()
{
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
    + "/mtproxy_history.db";

    db = QSqlDatabase::addDatabase("QSQLITE", "mtproxy_connection");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        emit logMessage("Ошибка открытия БД: " + db.lastError().text(), "error");
        return false;
    }

    QSqlQuery query(db);

    // Таблица подключений
    QString createConnections =
    "CREATE TABLE IF NOT EXISTS connections ("
    "id TEXT PRIMARY KEY,"
    "client_ip TEXT NOT NULL,"
    "client_port INTEGER,"
    "country TEXT,"
    "city TEXT,"
    "country_code TEXT,"
    "latitude REAL,"
    "longitude REAL,"
    "bytes_received INTEGER DEFAULT 0,"
    "bytes_sent INTEGER DEFAULT 0,"
    "connected_at DATETIME NOT NULL,"
    "disconnected_at DATETIME,"
    "duration_seconds INTEGER DEFAULT 0,"
    "last_activity DATETIME,"
    "telegram_version TEXT,"
    "device_model TEXT,"
    "os_version TEXT,"
    "proxy_port INTEGER"
    ")";

    if (!query.exec(createConnections)) {
        emit logMessage("Ошибка создания таблицы: " + query.lastError().text(), "error");
        return false;
    }

    // Индексы для быстрого поиска
    query.exec("CREATE INDEX IF NOT EXISTS idx_ip ON connections(client_ip)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_date ON connections(connected_at)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_country ON connections(country)");

    dbInitialized = true;
    return true;
}

// В функции MTProxyManager::startProxy, замените блок с аргументами:
bool MTProxyManager::startProxy(int port, const QString &secret)
{
    if (proxyRunning) {
        emit logMessage("Прокси уже запущен", "warning");
        return false;
    }

    if (mtprotoPath.isEmpty() || !QFile::exists(mtprotoPath)) {
        emit logMessage("MTProto-Proxy бинарник не найден: " + mtprotoPath, "error");
        return false;
    }

    currentPort = port;
    if (!secret.isEmpty()) {
        currentSecret = secret;
    } else if (currentSecret.isEmpty()) {
        currentSecret = generateSecureSecret();
        currentSecret = "ee" + currentSecret;
    }

    if (!validateSecret(currentSecret)) {
        emit logMessage("Неверный формат секрета", "error");
        return false;
    }

    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString proxySecretPath = appData + "/proxy-secret";
    QString proxyConfPath   = appData + "/proxy-multi.conf";

    // Создаем правильный конфигурационный файл для MTProxy
    QFile confFile(proxyConfPath);
    if (confFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&confFile);
        out << "# MTProxy Multi-Config File\n";
        out << "# Generated by TorManager MTProxy\n";
        out << "# Date: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n\n";
        // Правильный формат для MTProxy - нужно указать proxy с IP и портом
        // Используем 0.0.0.0 чтобы слушать на всех интерфейсах
        QString listenIp = internalIp.isEmpty() ? "127.0.0.1" : internalIp;
        out << "proxy " << listenIp << ":" << port << ";\n";
        confFile.close();
        emit logMessage("✅ proxy-multi.conf создан", "info");
    }

    // Скачиваем proxy-secret если нет
    if (!QFile::exists(proxySecretPath)) {
        emit logMessage("Загрузка proxy-secret...", "info");
        QProcess dl;
        dl.start("curl", {"-s", "--max-time", "15", "-o", proxySecretPath,
            "https://core.telegram.org/getProxySecret"});
        if (!dl.waitForFinished(20000) || dl.exitCode() != 0) {
            emit logMessage("Не удалось загрузить proxy-secret", "error");
            return false;
        }
        emit logMessage("✅ proxy-secret загружен", "success");
    }

    // Определяем IP адреса
    QString externalIp = getExternalIp();
    QString internalIp = getInternalIp();

    if (externalIp.isEmpty()) {
        externalIp = "0.0.0.0";
        emit logMessage("⚠️ Не удалось определить внешний IP", "warning");
    }
    currentExternalIp = externalIp;

    if (internalIp.isEmpty()) {
        internalIp = "0.0.0.0";
        emit logMessage("⚠️ Не удалось определить внутренний IP", "warning");
    }

    // Формируем аргументы запуска
    QStringList args;

    // Порт статистики
    args << "-p" << "8888";

    // Порт для клиентов
    args << "-H" << QString::number(port);

    // Количество воркеров
    args << "-M" << "1";

    // Пользователь (запускаем от root)
    args << "-u" << "root";

    // Секрет (убираем префикс ee/dd)
    QString rawSecret = currentSecret;
    if (rawSecret.startsWith("ee", Qt::CaseInsensitive) ||
        rawSecret.startsWith("dd", Qt::CaseInsensitive)) {
        rawSecret = rawSecret.mid(2);
        }
        args << "-S" << rawSecret;

    // AES пароль
    args << "--aes-pwd" << proxySecretPath;

    // NAT info (если нужно)
    if (externalIp != internalIp && externalIp != "0.0.0.0" && internalIp != "0.0.0.0") {
        args << "--nat-info" << (externalIp + ":" + internalIp);
        emit logMessage("   NAT Info: " + externalIp + ":" + internalIp, "info");
    }

    // Конфигурационный файл
    args << proxyConfPath;

    emit logMessage("🚀 Запуск MTProxy на порту " + QString::number(port), "info");
    emit logMessage("   Внешний IP: " + externalIp, "info");
    emit logMessage("   Команда: " + mtprotoPath + " " + args.join(" "), "info");

    // Останавливаем старый процесс
    if (mtprotoProcess->state() != QProcess::NotRunning) {
        mtprotoProcess->terminate();
        mtprotoProcess->waitForFinished(2000);
    }

    // Запускаем
    mtprotoProcess->start(mtprotoPath, args);

    if (!mtprotoProcess->waitForStarted(5000)) {
        emit logMessage("❌ Не удалось запустить MTProxy: " + mtprotoProcess->errorString(), "error");
        return false;
    }

    // Даем время на инициализацию
    QThread::msleep(1000);

    // Проверяем, жив ли процесс
    if (mtprotoProcess->state() == QProcess::NotRunning) {
        QString error = QString::fromUtf8(mtprotoProcess->readAllStandardError());
        emit logMessage("❌ MTProxy завершился с кодом " + QString::number(mtprotoProcess->exitCode()), "error");
        if (!error.isEmpty()) {
            emit logMessage("   " + error.trimmed(), "error");
        }
        return false;
    }

    // Настраиваем фаервол
    setupFirewallRules(port);

    proxyRunning = true;
    statsTimer->start(STATS_UPDATE_INTERVAL);
    cleanupTimer->start(CLEANUP_INTERVAL);

    emit proxyStarted();
    emit logMessage("✅ MTProxy успешно запущен на порту " + QString::number(port), "success");
    emit logMessage("🔗 tg:// ссылка: " + getProxyLink(), "info");
    emit logMessage("🌐 Браузер/iOS ссылка: " + getProxyLinkForPlatform("ios"), "info");

    return true;
}

bool MTProxyManager::stopProxy()
{
    if (!proxyRunning) {
        return true;
    }

    emit logMessage("Остановка MTProxy...", "info");

    statsTimer->stop();
    cleanupTimer->stop();

    // Удаляем правила фаервола
    removeFirewallRules(currentPort);

    // Закрываем все активные соединения в БД
    {
        QMutexLocker locker(&clientsMutex);
        for (const QString &connId : activeClients.keys()) {
            closeClientInDatabase(connId);
        }
        activeClients.clear();
    }

    // Останавливаем процесс
    if (mtprotoProcess->state() == QProcess::Running) {
        mtprotoProcess->terminate();
        if (!mtprotoProcess->waitForFinished(5000)) {
            emit logMessage("Процесс не завершился по SIGTERM, отправляю SIGKILL", "warning");
            mtprotoProcess->kill();
            mtprotoProcess->waitForFinished(2000);
        }
    }

    proxyRunning = false;
    emit proxyStopped();
    emit logMessage("MTProxy остановлен", "info");

    return true;
}

bool MTProxyManager::restartProxy()
{
    int port = currentPort;
    QString secret = currentSecret;
    stopProxy();
    QThread::msleep(500);
    return startProxy(port, secret);
}

void MTProxyManager::setSecret(const QString &secret)
{
    if (validateSecret(secret)) {
        currentSecret = secret;
        if (proxyRunning) {
            restartProxy();
        }
    } else {
        emit logMessage("Неверный формат секрета", "error");
    }
}

void MTProxyManager::onProxyProcessOutput()
{
    while (mtprotoProcess->canReadLine()) {
        QString line = QString::fromUtf8(mtprotoProcess->readLine()).trimmed();
        if (!line.isEmpty()) {
            parseLogLine(line);
        }
    }
}

void MTProxyManager::onProxyProcessError()
{
    QString error = QString::fromUtf8(mtprotoProcess->readAllStandardError());
    if (!error.isEmpty()) {
        emit logMessage("MTProxy stderr: " + error.trimmed(), "error");
        parseLogLine(error);
    }
}

void MTProxyManager::onProxyProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    proxyRunning = false;
    emit logMessage(QString("MTProxy завершён (код %1)").arg(exitCode),
                    exitCode == 0 ? "info" : "error");
    emit proxyStopped();
}

void MTProxyManager::parseLogLine(const QString &line)
{
    // Парсинг подключений
    // Формат: "new connection from 192.168.1.1:54321"
    static QRegularExpression connRe(R"(new connection from (\d+\.\d+\.\d+\.\d+):(\d+))");
    auto connMatch = connRe.match(line);
    if (connMatch.hasMatch()) {
        parseConnectionLine(line);
        return;
    }

    // Парсинг статистики трафика
    // Формат: "connection 123: rx=1024 tx=2048"
    static QRegularExpression trafficRe(R"(connection \d+: rx=(\d+) tx=(\d+))");
    auto trafficMatch = trafficRe.match(line);
    if (trafficMatch.hasMatch()) {
        parseStatsLine(line);
        return;
    }

    // Информация о клиенте (Telegram версия и т.д.)
    static QRegularExpression clientInfoRe(R"(client: ([\d\.]+) ([\w\s]+) ([\w\.]+))");
    auto infoMatch = clientInfoRe.match(line);
    if (infoMatch.hasMatch()) {
        // Можно обновить информацию о клиенте
    }

    // Логируем важные события
    if (line.contains("error", Qt::CaseInsensitive)) {
        emit logMessage("MTProxy: " + line, "error");
    } else if (line.contains("warning", Qt::CaseInsensitive)) {
        emit logMessage("MTProxy: " + line, "warning");
    }
}

void MTProxyManager::parseConnectionLine(const QString &line)
{
    static QRegularExpression connRe(R"(new connection from (\d+\.\d+\.\d+\.\d+):(\d+))");
    auto match = connRe.match(line);

    if (match.hasMatch()) {
        QString ip = match.captured(1);
        int port = match.captured(2).toInt();
        QString connId = QString("%1:%2_%3").arg(ip).arg(port)
                         .arg(QDateTime::currentMSecsSinceEpoch());

        MTProxyClient client;
        client.connectionId = connId;
        client.clientIp = ip;
        client.clientPort = port;
        client.connectedSince = QDateTime::currentDateTime();
        client.lastActivity = client.connectedSince;
        client.isActive = true;
        client.proxyPort = currentPort;
        client.bytesReceived = 0;
        client.bytesSent = 0;
        client.currentSpeed = 0;

        // Геолокация
        QString country = getCountryFromCache(ip);
        if (!country.isEmpty()) {
            client.country = country;
        } else {
            lookupGeoIP(ip);
        }

        {
            QMutexLocker locker(&clientsMutex);
            activeClients[connId] = client;
        }

        {
            QMutexLocker locker(&statsMutex);
            currentStats.totalConnections++;
            currentStats.activeConnections = activeClients.size();
            currentStats.uniqueUsers = QSet<QString>(activeClients.keys().begin(),
                                                     activeClients.keys().end()).size();
                                                     updatePeakConnections();
        }

        saveClientToDatabase(client);
        emit clientConnected(client);
        emit logMessage(QString("Новое подключение: %1:%2 (%3)").arg(ip).arg(port)
        .arg(client.country.isEmpty() ? "определение..." : client.country),
                        "info");
    }
}

void MTProxyManager::parseStatsLine(const QString &line)
{
    static QRegularExpression statsRe(R"(rx=(\d+) tx=(\d+))");
    auto match = statsRe.match(line);

    if (match.hasMatch()) {
        qint64 rx = match.captured(1).toLongLong();
        qint64 tx = match.captured(2).toLongLong();

        // Обновляем общую статистику
        {
            QMutexLocker locker(&statsMutex);
            currentStats.totalBytesRx += rx;
            currentStats.totalBytesTx += tx;
        }

        // Обновляем информацию о клиенте (если можно определить)
        // В реальном MTProxy нужно парсить более детально
    }
}

void MTProxyManager::updateStats()
{
    if (!proxyRunning) return;

    // Обновление статистики активных соединений
    QMutexLocker locker(&clientsMutex);

    QDateTime now = QDateTime::currentDateTime();

    for (auto it = activeClients.begin(); it != activeClients.end(); ++it) {
        // Обновляем скорость (приблизительно)
        qint64 elapsed = it->lastActivity.msecsTo(now);
        if (elapsed > 0 && elapsed < 5000) {
            // Расчет скорости на основе изменения трафика
            // В реальной реализации нужно отслеживать изменения
        }
        it->lastActivity = now;
    }

    {
        QMutexLocker statsLocker(&statsMutex);
        currentStats.activeConnections = activeClients.size();
        updatePeakConnections();
    }

    emit statsUpdated(currentStats);
}

void MTProxyManager::cleanupInactiveConnections()
{
    QDateTime now = QDateTime::currentDateTime();
    QMutexLocker locker(&clientsMutex);

    QList<QString> toRemove;

    for (auto it = activeClients.begin(); it != activeClients.end(); ++it) {
        if (it->lastActivity.secsTo(now) > INACTIVE_TIMEOUT) {
            toRemove.append(it.key());
        }
    }

    for (const QString &connId : toRemove) {
        closeClientInDatabase(connId);
        activeClients.remove(connId);
        emit clientDisconnected(connId);
        emit logMessage(QString("Клиент отключён по таймауту: %1").arg(connId), "info");
    }
}

void MTProxyManager::lookupGeoIP(const QString &ip)
{
    if (pendingGeoLookups.contains(ip)) return;
    if (ip.startsWith("127.") || ip.startsWith("192.168.") ||
        ip.startsWith("10.") || ip.startsWith("172.")) {
        return; // Локальные IP
        }

        pendingGeoLookups.insert(ip);

    // Используем бесплатный API для геолокации
    QUrl url(QString("http://ip-api.com/json/%1?fields=status,country,countryCode,city,lat,lon")
    .arg(ip));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      "MTProxyManager/1.0");

    geoManager->get(request);
}

void MTProxyManager::onGeoLookupFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    if (obj["status"].toString() != "success") return;

    QString ip = reply->url().toString().split('/').last().split('?').first();
    pendingGeoLookups.remove(ip);

    QString country = obj["country"].toString();
    QString city = obj["city"].toString();
    QString countryCode = obj["countryCode"].toString();
    double lat = obj["lat"].toDouble();
    double lon = obj["lon"].toDouble();

    // Сохраняем в кэш
    geoCache[ip] = qMakePair(country, city);

    // Обновляем активные соединения
    QMutexLocker locker(&clientsMutex);
    for (auto &client : activeClients) {
        if (client.clientIp == ip) {
            client.country = country;
            client.city = city;
            client.countryCode = countryCode;
            client.latitude = lat;
            client.longitude = lon;

            // Обновляем в БД
            QSqlQuery query(db);
            query.prepare("UPDATE connections SET country=?, city=?, country_code=?, latitude=?, longitude=? WHERE id=?");
            query.addBindValue(country);
            query.addBindValue(city);
            query.addBindValue(countryCode);
            query.addBindValue(lat);
            query.addBindValue(lon);
            query.addBindValue(client.connectionId);
            query.exec();

            emit logMessage(QString("Геолокация определена для %1: %2, %3")
            .arg(ip, country, city), "info");
            break;
        }
    }
}

QString MTProxyManager::getCountryFromCache(const QString &ip)
{
    if (geoCache.contains(ip)) {
        return geoCache[ip].first;
    }
    return QString();
}

void MTProxyManager::saveClientToDatabase(const MTProxyClient &client)
{
    if (!dbInitialized) return;

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO connections (id, client_ip, client_port, country, city, country_code, "
        "latitude, longitude, bytes_received, bytes_sent, connected_at, proxy_port) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
    );

    query.addBindValue(client.connectionId);
    query.addBindValue(client.clientIp);
    query.addBindValue(client.clientPort);
    query.addBindValue(client.country);
    query.addBindValue(client.city);
    query.addBindValue(client.countryCode);
    query.addBindValue(client.latitude);
    query.addBindValue(client.longitude);
    query.addBindValue(client.bytesReceived);
    query.addBindValue(client.bytesSent);
    query.addBindValue(client.connectedSince.toString(Qt::ISODate));
    query.addBindValue(client.proxyPort);

    if (!query.exec()) {
        emit logMessage("Ошибка сохранения в БД: " + query.lastError().text(), "error");
    }
}

void MTProxyManager::updateClientInDatabase(const MTProxyClient &client)
{
    if (!dbInitialized) return;

    QSqlQuery query(db);
    // FIX #5: last_activity добавлена в схему таблицы
    query.prepare(
        "UPDATE connections SET bytes_received=?, bytes_sent=?, last_activity=? WHERE id=?"
    );
    query.addBindValue(client.bytesReceived);
    query.addBindValue(client.bytesSent);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(client.connectionId);
    if (!query.exec()) {
        emit logMessage("updateClientInDatabase error: " + query.lastError().text(), "error");
    }
}

void MTProxyManager::closeClientInDatabase(const QString &connectionId)
{
    if (!dbInitialized) return;

    // FIX #4: JULIANDAY(disconnected_at) был NULL т.к. поле ещё не записано —
    // duration_seconds всегда равнялся 0. Теперь вычисляем в C++.
    QSqlQuery sel(db);
    sel.prepare("SELECT connected_at FROM connections WHERE id=?");
    sel.addBindValue(connectionId);
    sel.exec();

    qint64 durationSecs = 0;
    if (sel.next()) {
        QDateTime connectedAt = QDateTime::fromString(sel.value(0).toString(), Qt::ISODate);
        if (connectedAt.isValid())
            durationSecs = connectedAt.secsTo(QDateTime::currentDateTime());
    }

    QSqlQuery query(db);
    query.prepare(
        "UPDATE connections SET disconnected_at=?, duration_seconds=? WHERE id=?"
    );
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(durationSecs);
    query.addBindValue(connectionId);
    if (!query.exec()) {
        emit logMessage("closeClientInDatabase error: " + query.lastError().text(), "error");
    }
}

QList<MTProxyClient> MTProxyManager::getActiveClients() const
{
    QMutexLocker locker(&clientsMutex);
    return activeClients.values();
}

QList<MTProxyClient> MTProxyManager::getClientHistory(int limit) const
{
    QList<MTProxyClient> history;

    if (!dbInitialized) return history;

    QSqlQuery query(db);
    query.prepare(
        "SELECT id, client_ip, client_port, country, city, country_code, "
        "latitude, longitude, bytes_received, bytes_sent, connected_at, "
        "disconnected_at, duration_seconds, proxy_port "
        "FROM connections ORDER BY connected_at DESC LIMIT ?"
    );
    query.addBindValue(limit);

    if (!query.exec()) {
        return history;
    }

    while (query.next()) {
        MTProxyClient client;
        client.connectionId = query.value(0).toString();
        client.clientIp = query.value(1).toString();
        client.clientPort = query.value(2).toInt();
        client.country = query.value(3).toString();
        client.city = query.value(4).toString();
        client.countryCode = query.value(5).toString();
        client.latitude = query.value(6).toDouble();
        client.longitude = query.value(7).toDouble();
        client.bytesReceived = query.value(8).toLongLong();
        client.bytesSent = query.value(9).toLongLong();
        client.connectedSince = QDateTime::fromString(query.value(10).toString(), Qt::ISODate);
        // FIX #6: читаем disconnected_at (индекс 11) и duration_seconds (индекс 12)
        client.disconnectedAt = QDateTime::fromString(query.value(11).toString(), Qt::ISODate);
        client.durationSeconds = query.value(12).toLongLong();
        client.isActive = false;
        client.proxyPort = query.value(13).toInt();
        history.append(client);
    }

    return history;
}

MTProxyStats MTProxyManager::getStats() const
{
    QMutexLocker locker(&statsMutex);
    return currentStats;
}

void MTProxyManager::updatePeakConnections()
{
    int current = currentStats.activeConnections;
    if (current > peakConnections) {
        peakConnections = current;
        peakTime = QDateTime::currentDateTime();
        currentStats.peakConnections = peakConnections;
        currentStats.peakTime = peakTime;
    }
}

QString MTProxyManager::generateSecret()
{
    // Простой секрет (32 символа hex)
    QByteArray secret;
    for (int i = 0; i < 16; i++) {
        secret.append(static_cast<char>(QRandomGenerator::global()->bounded(256)));
    }
    return secret.toHex();
}

QString MTProxyManager::generateSecureSecret()
{
    // Генерирует 32 hex символа (16 байт) — передаётся в -S аргументе mtproto-proxy
    QByteArray secret(16, 0);
    for (int i = 0; i < 16; i++) {
        secret[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    return secret.toHex();
}

QString MTProxyManager::generateFakeTLSSecret(const QString &domain)
{
    // fake-TLS секрет: "ee" + 32 hex (16 случайных байт)
    // Опционально: + hex-кодировка домена (для SNI маскировки)
    // Формат который принимает mtproto-proxy через -S: без префикса ee (он только в URL)
    // В URL ссылке: "ee" + те же 32 hex
    QByteArray secret(16, 0);
    for (int i = 0; i < 16; i++) {
        secret[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    // Возвращаем уже с префиксом ee — для хранения в currentSecret
    // При передаче в -S аргумент префикс будет отрезан в startProxy()
    QString result = "ee" + secret.toHex();
    if (!domain.isEmpty()) {
        // Добавляем домен в hex для fake-TLS SNI (расширенный формат)
        result += domain.toUtf8().toHex();
    }
    return result;
}

bool MTProxyManager::validateSecret(const QString &secret)
{
    if (secret.isEmpty()) return false;

    QString s = secret;
    // Убираем известные префиксы
    if (s.startsWith("ee", Qt::CaseInsensitive) ||
        s.startsWith("dd", Qt::CaseInsensitive)) {
        s = s.mid(2);
    }

    // Базовая часть должна быть ровно 32 hex символа
    // (расширенный fake-TLS с доменом может быть длиннее — допускаем >= 32)
    if (s.length() < 32) return false;

    static QRegularExpression hexRe("^[0-9a-fA-F]+$");
    return hexRe.match(s).hasMatch();
}

QString MTProxyManager::getProxyLink() const
{
    // Используем реальный внешний IP, определённый при запуске.
    // Если serverAddress задан вручную — используем его (для доменного имени).
    QString server = !serverAddress.isEmpty() ? serverAddress
                   : (!currentExternalIp.isEmpty() ? currentExternalIp : "YOUR_SERVER_IP");

    // В ссылке секрет должен быть с префиксом "ee" для поддержки fake-TLS
    // (без него iOS Telegram 7.x+ отказывается подключаться)
    QString linkSecret = currentSecret;
    if (!linkSecret.startsWith("ee", Qt::CaseInsensitive) &&
        !linkSecret.startsWith("dd", Qt::CaseInsensitive)) {
        linkSecret = "ee" + linkSecret;
    }

    // Формат tg://proxy работает на всех платформах включая iOS
    return QString("tg://proxy?server=%1&port=%2&secret=%3")
           .arg(server).arg(currentPort).arg(linkSecret);
}

QString MTProxyManager::getProxyLinkForPlatform(const QString &platform) const
{
    QString server = !serverAddress.isEmpty() ? serverAddress
                   : (!currentExternalIp.isEmpty() ? currentExternalIp : "YOUR_SERVER_IP");

    QString linkSecret = currentSecret;
    if (!linkSecret.startsWith("ee", Qt::CaseInsensitive) &&
        !linkSecret.startsWith("dd", Qt::CaseInsensitive)) {
        linkSecret = "ee" + linkSecret;
    }

    if (platform == "ios" || platform == "android" || platform == "desktop") {
        // https://t.me/proxy — универсальный формат, открывается в браузере
        // и перенаправляет в Telegram на всех платформах
        return QString("https://t.me/proxy?server=%1&port=%2&secret=%3")
               .arg(server).arg(currentPort).arg(linkSecret);
    }
    return getProxyLink();
}

QString MTProxyManager::formatBytes(qint64 bytes) const
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024LL * 1024 * 1024)
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

QString MTProxyManager::formatDuration(qint64 seconds) const
{
    if (seconds < 60) return QString("%1 сек").arg(seconds);
    if (seconds < 3600) return QString("%1 мин %2 сек").arg(seconds / 60).arg(seconds % 60);
    if (seconds < 86400)
        return QString("%1 ч %2 мин").arg(seconds / 3600).arg((seconds % 3600) / 60);
    return QString("%1 дн %2 ч").arg(seconds / 86400).arg((seconds % 86400) / 3600);
}

// ==================== MTProxyWidget Implementation ====================

MTProxyWidget::MTProxyWidget(MTProxyManager *manager, QWidget *parent)
: QWidget(parent)
, proxyManager(manager)
{
    setupUI();
    loadSettings();

    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(3000);
    connect(refreshTimer, &QTimer::timeout, this, &MTProxyWidget::refreshData);
    refreshTimer->start();

    // Подключение сигналов
    connect(proxyManager, &MTProxyManager::clientConnected,
            this, &MTProxyWidget::onClientConnected);
    connect(proxyManager, &MTProxyManager::clientDisconnected,
            this, &MTProxyWidget::onClientDisconnected);
    connect(proxyManager, &MTProxyManager::statsUpdated,
            this, &MTProxyWidget::onStatsUpdated);
    connect(proxyManager, &MTProxyManager::logMessage,
            [this](const QString &msg, const QString &level) {
                // Можно отображать в общем логе
                Q_UNUSED(level)
                Q_UNUSED(msg)
            });

    refreshData();
}

MTProxyWidget::~MTProxyWidget()
{
    saveSettings();
}

void MTProxyWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(6);

    // ==================== Панель управления ====================
    controlGroup = new QGroupBox("🎮 Управление MTProxy");
    QGridLayout *controlLayout = new QGridLayout(controlGroup);

    controlLayout->addWidget(new QLabel("Порт:"), 0, 0);
    edtPort = new QLineEdit("443");
    edtPort->setValidator(new QIntValidator(1, 65535, this));
    controlLayout->addWidget(edtPort, 0, 1);

    controlLayout->addWidget(new QLabel("Домен/IP:"), 1, 0);
    edtDomain = new QLineEdit();
    edtDomain->setPlaceholderText("wwcat.duckdns.org  (или оставьте пустым — будет использован внешний IP)");
    edtDomain->setToolTip("Домен или IP для ссылки подключения. Используйте DDNS-домен если IP меняется.");
    controlLayout->addWidget(edtDomain, 1, 1, 1, 5);

    controlLayout->addWidget(new QLabel("Секрет:"), 2, 0);
    edtSecret = new QLineEdit();
    edtSecret->setPlaceholderText("Автоматически генерируется (ee-префикс = fake-TLS)");
    edtSecret->setReadOnly(true);
    controlLayout->addWidget(edtSecret, 2, 1, 1, 4);

    btnGenerateSecret = new QPushButton("🎲 Генерация");
    controlLayout->addWidget(btnGenerateSecret, 2, 5);

    // FIX #2: кнопка установки бинарника mtproto-proxy
    btnInstallMTProxy = new QPushButton("⚙️ Установить MTProxy");
    btnInstallMTProxy->setToolTip("Скачать и собрать mtproto-proxy из исходников GitHub");
    controlLayout->addWidget(btnInstallMTProxy, 3, 5, 1, 2);

    btnStartStop = new QPushButton("▶ Запустить");
    btnStartStop->setStyleSheet("QPushButton { background: #27ae60; color: white; font-weight: bold; }");
    controlLayout->addWidget(btnStartStop, 2, 6);

    btnCopyLink = new QPushButton("📋 Копировать ссылку (tg://)");
    controlLayout->addWidget(btnCopyLink, 3, 0, 1, 2);

    QPushButton *btnCopyWebLink = new QPushButton("📱 Копировать iOS/Web ссылку");
    btnCopyWebLink->setToolTip("https://t.me/proxy — открывается в браузере и перенаправляет в Telegram");
    controlLayout->addWidget(btnCopyWebLink, 4, 0, 1, 2);

    lblStatus = new QLabel("Статус: ○ Остановлен");
    lblStatus->setStyleSheet("color: #7f8c8d; font-weight: bold;");
    controlLayout->addWidget(lblStatus, 3, 2, 1, 4);

    lblProxyLink = new QLabel();
    lblProxyLink->setStyleSheet("color: #2980b9; font-size: 10px; font-family: monospace;");
    lblProxyLink->setWordWrap(true);
    controlLayout->addWidget(lblProxyLink, 5, 0, 1, 7);

    mainLayout->addWidget(controlGroup);

    // ==================== Статистика ====================
    statsGroup = new QGroupBox("📊 Статистика прокси");
    QGridLayout *statsLayout = new QGridLayout(statsGroup);

    lblActiveConns = new QLabel("Активных: 0");
    lblTotalConns = new QLabel("Всего: 0");
    lblPeakConns = new QLabel("Пик: 0");
    lblTraffic = new QLabel("Трафик: ↓0 ↑0");
    lblAvgSpeed = new QLabel("Ср. скорость: 0");
    lblUniqueUsers = new QLabel("Уникальных: 0");

    statsLayout->addWidget(lblActiveConns, 0, 0);
    statsLayout->addWidget(lblTotalConns, 0, 1);
    statsLayout->addWidget(lblPeakConns, 0, 2);
    statsLayout->addWidget(lblTraffic, 1, 0);
    statsLayout->addWidget(lblAvgSpeed, 1, 1);
    statsLayout->addWidget(lblUniqueUsers, 1, 2);

    mainLayout->addWidget(statsGroup);

    // ==================== Активные клиенты ====================
    clientsGroup = new QGroupBox("🌐 Активные клиенты");
    QVBoxLayout *clientsLayout = new QVBoxLayout(clientsGroup);

    QHBoxLayout *filterLayout = new QHBoxLayout();
    edtSearch = new QLineEdit();
    edtSearch->setPlaceholderText("🔍 Поиск по IP или стране...");
    edtSearch->setClearButtonEnabled(true);
    filterLayout->addWidget(edtSearch, 1);

    cboCountryFilter = new QComboBox();
    cboCountryFilter->addItem("Все страны");
    filterLayout->addWidget(cboCountryFilter);

    btnRefresh = new QPushButton("🔄 Обновить");
    filterLayout->addWidget(btnRefresh);

    btnExport = new QPushButton("💾 Экспорт CSV");
    filterLayout->addWidget(btnExport);

    clientsLayout->addLayout(filterLayout);

    clientsTable = new QTableWidget();
    clientsTable->setColumnCount(9);
    clientsTable->setHorizontalHeaderLabels({
        "IP Адрес", "Страна", "Город", "Время подключения",
        "Активность", "↓ Получено", "↑ Отправлено", "Скорость", "Порт"
    });
    clientsTable->horizontalHeader()->setStretchLastSection(true);
    clientsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    clientsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    clientsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    clientsLayout->addWidget(clientsTable);

    mainLayout->addWidget(clientsGroup, 2);

    // ==================== История подключений ====================
    historyGroup = new QGroupBox("📜 История подключений");
    QVBoxLayout *historyLayout = new QVBoxLayout(historyGroup);

    QHBoxLayout *historyBtnLayout = new QHBoxLayout();
    btnClearHistory = new QPushButton("🗑 Очистить историю");
    btnClearHistory->setStyleSheet("color: #e74c3c;");
    historyBtnLayout->addStretch();
    historyBtnLayout->addWidget(btnClearHistory);
    historyLayout->addLayout(historyBtnLayout);

    historyTable = new QTableWidget();
    historyTable->setColumnCount(7);
    historyTable->setHorizontalHeaderLabels({
        "IP", "Страна", "Время подключения", "Время отключения",
        "Длительность", "Трафик ↓/↑", "Порт"
    });
    historyTable->horizontalHeader()->setStretchLastSection(true);
    historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    historyLayout->addWidget(historyTable);

    mainLayout->addWidget(historyGroup, 1);

    // ==================== Подключение сигналов ====================
    connect(btnStartStop, &QPushButton::clicked, this, &MTProxyWidget::onStartStopClicked);
    connect(btnGenerateSecret, &QPushButton::clicked, this, &MTProxyWidget::onGenerateSecret);
    connect(btnInstallMTProxy, &QPushButton::clicked, this, &MTProxyWidget::onInstallMTProxy);
    connect(btnCopyLink, &QPushButton::clicked, this, &MTProxyWidget::onCopyLink);
    connect(btnCopyWebLink, &QPushButton::clicked, this, [this]() {
        if (!proxyManager->isRunning()) {
            QMessageBox::warning(this, "Прокси не запущен", "Сначала запустите прокси.");
            return;
        }
        QString link = proxyManager->getProxyLinkForPlatform("ios");
        QApplication::clipboard()->setText(link);
        QMessageBox::information(this, "iOS/Web ссылка скопирована",
            "Ссылка для iOS/Android/браузера скопирована:\n\n" + link + "\n\n"
            "Отправьте её пользователю — при открытии в Safari/Chrome "
            "автоматически откроется Telegram с настройками прокси.");
    });
    connect(btnRefresh, &QPushButton::clicked, this, &MTProxyWidget::refreshData);
    connect(btnExport, &QPushButton::clicked, this, &MTProxyWidget::onExportCSV);
    connect(btnClearHistory, &QPushButton::clicked, this, [this]() {
        // Очистка истории
    });
    connect(edtSearch, &QLineEdit::textChanged, this, &MTProxyWidget::updateClientsTable);
    connect(cboCountryFilter, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MTProxyWidget::updateClientsTable);
    connect(clientsTable, &QTableWidget::customContextMenuRequested,
            this, &MTProxyWidget::onClientTableContextMenu);
}

void MTProxyWidget::refreshData()
{
    updateClientsTable();
    updateStatsDisplay();
    updateProxyStatus();

    // Обновление истории
    auto history = proxyManager->getClientHistory(100);
    historyTable->setSortingEnabled(false);
    historyTable->setRowCount(history.size());

    auto fmtBytes = [](qint64 b) -> QString {
        if (b < 1024) return QString::number(b) + " B";
        if (b < 1024*1024) return QString::number(b/1024) + " KB";
        return QString::number(b/(1024.0*1024.0), 'f', 1) + " MB";
    };

    auto fmtDuration = [](qint64 s) -> QString {
        if (s <= 0) return "—";
        if (s < 60) return QString("%1 сек").arg(s);
        if (s < 3600) return QString("%1 мин %2 сек").arg(s/60).arg(s%60);
        return QString("%1 ч %2 мин").arg(s/3600).arg((s%3600)/60);
    };

    for (int i = 0; i < history.size(); ++i) {
        const auto &c = history[i];

        historyTable->setItem(i, 0, new QTableWidgetItem(c.clientIp));
        historyTable->setItem(i, 1, new QTableWidgetItem(c.country.isEmpty() ? "—" : c.country));
        historyTable->setItem(i, 2, new QTableWidgetItem(
            c.connectedSince.toString("dd.MM HH:mm:ss")));
        // FIX #6: было "—" — теперь показываем реальное время отключения
        historyTable->setItem(i, 3, new QTableWidgetItem(
            c.disconnectedAt.isValid() ? c.disconnectedAt.toString("dd.MM HH:mm:ss") : "—"));
        // FIX #6: было fmtDuration(c.bytesReceived/1024) — трафик вместо времени!
        historyTable->setItem(i, 4, new QTableWidgetItem(fmtDuration(c.durationSeconds)));
        historyTable->setItem(i, 5, new QTableWidgetItem(
            QString("%1 / %2").arg(fmtBytes(c.bytesReceived), fmtBytes(c.bytesSent))));
        historyTable->setItem(i, 6, new QTableWidgetItem(QString::number(c.proxyPort)));
    }
    historyTable->setSortingEnabled(true);
}

void MTProxyWidget::updateClientsTable()
{
    auto clients = proxyManager->getActiveClients();
    QString search = edtSearch->text().trimmed().toLower();
    QString countryFilter = cboCountryFilter->currentText();

    // Обновление списка стран в фильтре
    static QSet<QString> knownCountries;
    for (const auto &c : clients) {
        if (!c.country.isEmpty() && !knownCountries.contains(c.country)) {
            knownCountries.insert(c.country);
            cboCountryFilter->addItem(c.country);
        }
    }

    // Фильтрация
    QList<MTProxyClient> filtered;
    for (const auto &c : clients) {
        if (!search.isEmpty() && !c.clientIp.contains(search) &&
            !c.country.toLower().contains(search)) {
            continue;
            }
            if (countryFilter != "Все страны" && c.country != countryFilter) {
                continue;
            }
            filtered.append(c);
    }

    auto fmtBytes = [](qint64 b) -> QString {
        if (b < 1024) return QString::number(b) + " B";
        if (b < 1024*1024) return QString::number(b/1024) + " KB";
        return QString::number(b/(1024.0*1024.0), 'f', 1) + " MB";
    };

    auto fmtSpeed = [](qint64 bps) -> QString {
        if (bps <= 0) return "—";
        if (bps < 1024) return QString::number(bps) + " Б/с";
        if (bps < 1024*1024) return QString::number(bps/1024.0, 'f', 1) + " КБ/с";
        return QString::number(bps/(1024.0*1024.0), 'f', 1) + " МБ/с";
    };

    clientsTable->setSortingEnabled(false);
    clientsTable->setRowCount(filtered.size());

    QDateTime now = QDateTime::currentDateTime();

    for (int i = 0; i < filtered.size(); ++i) {
        const auto &c = filtered[i];
        qint64 activeSecs = c.connectedSince.secsTo(now);

        clientsTable->setItem(i, 0, new QTableWidgetItem(c.clientIp));

        QTableWidgetItem *countryItem = new QTableWidgetItem(c.country.isEmpty() ? "—" : c.country);
        if (!c.country.isEmpty()) {
            countryItem->setToolTip(QString("%1, %2").arg(c.country, c.city));
        }
        clientsTable->setItem(i, 1, countryItem);

        clientsTable->setItem(i, 2, new QTableWidgetItem(c.city.isEmpty() ? "—" : c.city));
        clientsTable->setItem(i, 3, new QTableWidgetItem(
            c.connectedSince.toString("dd.MM HH:mm:ss")));
        clientsTable->setItem(i, 4, new QTableWidgetItem(
            QString("%1 (%2)").arg(c.lastActivity.toString("HH:mm:ss"))
            .arg(proxyManager->formatDuration(activeSecs))));
        clientsTable->setItem(i, 5, new QTableWidgetItem(fmtBytes(c.bytesReceived)));
        clientsTable->setItem(i, 6, new QTableWidgetItem(fmtBytes(c.bytesSent)));
        clientsTable->setItem(i, 7, new QTableWidgetItem(fmtSpeed(c.currentSpeed)));
        clientsTable->setItem(i, 8, new QTableWidgetItem(QString::number(c.proxyPort)));

        // Подсветка давно неактивных
        if (c.lastActivity.secsTo(now) > 60) {
            for (int col = 0; col < 9; ++col) {
                if (auto *item = clientsTable->item(i, col)) {
                    item->setForeground(QColor(150, 150, 150));
                }
            }
        }
    }

    clientsTable->setSortingEnabled(true);
}

void MTProxyWidget::updateStatsDisplay()
{
    MTProxyStats stats = proxyManager->getStats();

    lblActiveConns->setText(QString("Активных: <b>%1</b>").arg(stats.activeConnections));
    lblTotalConns->setText(QString("Всего: %1").arg(stats.totalConnections));
    lblPeakConns->setText(QString("Пик: %1").arg(stats.peakConnections));

    auto fmtBytes = [](qint64 b) -> QString {
        if (b < 1024*1024) return QString::number(b/1024) + " KB";
        return QString::number(b/(1024.0*1024.0), 'f', 1) + " MB";
    };

    lblTraffic->setText(QString("Трафик: ↓%1 ↑%2")
    .arg(fmtBytes(stats.totalBytesRx), fmtBytes(stats.totalBytesTx)));
    lblAvgSpeed->setText(QString("Ср. скорость: %1").arg(fmtBytes(stats.avgSpeed) + "/с"));
    lblUniqueUsers->setText(QString("Уникальных: %1").arg(stats.uniqueUsers));
}

void MTProxyWidget::updateProxyStatus()
{
    bool running = proxyManager->isRunning();

    if (running) {
        lblStatus->setText("Статус: ● Активен");
        lblStatus->setStyleSheet("color: #27ae60; font-weight: bold;");
        btnStartStop->setText("⏹ Остановить");
        btnStartStop->setStyleSheet("QPushButton { background: #e74c3c; color: white; font-weight: bold; }");
        QString tgLink = proxyManager->getProxyLink();
        QString webLink = proxyManager->getProxyLinkForPlatform("ios");
        lblProxyLink->setText("🔗 " + tgLink + "\n🌐 " + webLink);
    } else {
        lblStatus->setText("Статус: ○ Остановлен");
        lblStatus->setStyleSheet("color: #7f8c8d; font-weight: bold;");
        btnStartStop->setText("▶ Запустить");
        btnStartStop->setStyleSheet("QPushButton { background: #27ae60; color: white; font-weight: bold; }");
        lblProxyLink->clear();
    }
}

void MTProxyWidget::onStartStopClicked()
{
    if (proxyManager->isRunning()) {
        proxyManager->stopProxy();
    } else {
        int port = edtPort->text().toInt();
        QString secret = edtSecret->text().trimmed();

        if (secret.isEmpty()) {
            secret = MTProxyManager::generateFakeTLSSecret();
            edtSecret->setText(secret);
        }

        // Передаём домен/хост — используется в ссылках вместо IP
        QString domain = edtDomain ? edtDomain->text().trimmed() : QString();
        proxyManager->setServerAddress(domain);

        if (proxyManager->startProxy(port, secret)) {
            saveSettings();
        }
    }
}

void MTProxyWidget::onGenerateSecret()
{
    // Генерируем fake-TLS секрет с префиксом "ee" — работает на iOS, Android, Desktop
    QString secret = MTProxyManager::generateFakeTLSSecret();
    edtSecret->setText(secret);

    if (proxyManager->isRunning()) {
        proxyManager->setSecret(secret);
    }
}

void MTProxyWidget::onCopyLink()
{
    if (!proxyManager->isRunning()) return;

    QString tgLink      = proxyManager->getProxyLink();
    QString browserLink = proxyManager->getProxyLinkForPlatform("ios");

    // Копируем в буфер ссылку tg:// (открывается напрямую в Telegram)
    QApplication::clipboard()->setText(tgLink);

    QMessageBox::information(this, "Ссылка скопирована",
        "Ссылка для подключения скопирована в буфер обмена.\n\n"
        "🔗 Ссылка для Telegram (все платформы):\n" + tgLink + "\n\n"
        "🌐 Ссылка через браузер (iOS / Android):\n" + browserLink + "\n\n"
        "iOS: откройте ссылку в Safari — Telegram откроется автоматически.\n"
        "Desktop: Настройки → Данные и память → Прокси → Вставить ссылку.");
}

void MTProxyWidget::onExportCSV()
{
    QString filename = QFileDialog::getSaveFileName(this, "Экспорт статистики",
                                                    QDir::homePath() + "/mtproxy_stats_" +
                                                    QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".csv",
                                                    "CSV Files (*.csv)");

    if (filename.isEmpty()) return;

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось создать файл");
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "IP;Страна;Город;Время подключения;Длительность;Получено;Отправлено;Порт\n";

    auto history = proxyManager->getClientHistory(10000);
    for (const auto &c : history) {
        qint64 duration = c.connectedSince.secsTo(QDateTime::currentDateTime());
        out << c.clientIp << ";"
        << c.country << ";"
        << c.city << ";"
        << c.connectedSince.toString("dd.MM.yyyy HH:mm:ss") << ";"
        << proxyManager->formatDuration(duration) << ";"
        << c.bytesReceived << ";"
        << c.bytesSent << ";"
        << c.proxyPort << "\n";
    }

    file.close();
    QMessageBox::information(this, "Экспорт завершён",
                             "Статистика экспортирована в:\n" + filename);
}

void MTProxyWidget::onClientTableContextMenu(const QPoint &pos)
{
    auto item = clientsTable->itemAt(pos);
    if (!item) return;

    int row = item->row();
    QString ip = clientsTable->item(row, 0)->text();
    QString country = clientsTable->item(row, 1)->text();

    QMenu menu(this);
    menu.addAction("📋 Копировать IP", [ip]() {
        QApplication::clipboard()->setText(ip);
    });
    menu.addAction("📋 Копировать страну", [country, ip]() {
        QApplication::clipboard()->setText(country);
    });
    menu.addSeparator();
    menu.addAction("🚫 Заблокировать IP", [this, ip]() {
        // Блокировка IP через iptables
        QProcess::execute("iptables", {"-A", "INPUT", "-s", ip, "-j", "DROP"});
        QMessageBox::information(this, "IP заблокирован",
                                 "IP " + ip + " добавлен в чёрный список");
    });
    menu.addAction("🔍 WHOIS", [ip]() {
        QProcess::startDetached("whois", {ip});
    });

    menu.exec(clientsTable->mapToGlobal(pos));
}

void MTProxyWidget::showClientDetails()
{
    auto selected = clientsTable->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    QString ip = clientsTable->item(row, 0)->text();
    QString country = clientsTable->item(row, 1)->text();
    QString city = clientsTable->item(row, 2)->text();

    QMessageBox::information(this, "Детали клиента",
                             QString("<b>IP адрес:</b> %1<br>"
                             "<b>Страна:</b> %2<br>"
                             "<b>Город:</b> %3<br>"
                             "<b>Прокси порт:</b> %4")
                             .arg(ip, country, city)
                             .arg(clientsTable->item(row, 8)->text()));
}

void MTProxyWidget::banClientIP()
{
    auto selected = clientsTable->selectedItems();
    if (selected.isEmpty()) return;

    QString ip = clientsTable->item(selected.first()->row(), 0)->text();

    if (QMessageBox::question(this, "Блокировка IP",
        "Заблокировать IP " + ip + "?\n\n"
        "Это добавит правило в iptables и клиент больше не сможет подключиться.",
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

        QProcess::execute("iptables", {"-A", "INPUT", "-s", ip, "-j", "DROP"});
        QMessageBox::information(this, "IP заблокирован",
                                 "IP " + ip + " добавлен в чёрный список");
    }
}

void MTProxyWidget::loadSettings()
{
    if (!edtPort || !edtSecret) return;

    QSettings settings("TorManager", "MTProxy");
    edtPort->setText(settings.value("port", "443").toString());

    // Загружаем домен (по умолчанию пустой — будет использован внешний IP)
    if (edtDomain) {
        edtDomain->setText(settings.value("domain", "").toString());
    }

    QString secret = settings.value("secret").toString();
    // Проверяем валидность секрета — если неверный, генерируем новый fake-TLS
    if (!secret.isEmpty() && !MTProxyManager::validateSecret(secret)) {
        secret = MTProxyManager::generateFakeTLSSecret();
        settings.setValue("secret", secret);
        settings.sync();
    }
    // Если секрет пустой — не генерируем здесь, пользователь сам нажмёт "Генерация"
    edtSecret->setText(secret);
}

void MTProxyWidget::saveSettings()
{
    if (!edtPort || !edtSecret) return;

    QSettings settings("TorManager", "MTProxy");
    settings.setValue("port", edtPort->text());
    if (edtDomain) {
        settings.setValue("domain", edtDomain->text().trimmed());
    }
    if (!edtSecret->text().isEmpty()) {
        settings.setValue("secret", edtSecret->text());
    }
    settings.sync();
}

void MTProxyWidget::onClientConnected(const MTProxyClient &client)
{
    refreshData();

    // Уведомление в трее - ищем MainWindow как родительский виджет
    QWidget *mainWindow = window();
    QSystemTrayIcon *tray = mainWindow ? mainWindow->findChild<QSystemTrayIcon*>() : nullptr;
    if (tray) {
        tray->showMessage("Новое подключение MTProxy",
                          QString("%1 (%2)").arg(client.clientIp, client.country),
                          QSystemTrayIcon::Information, 3000);
    }
}

void MTProxyWidget::onClientDisconnected(const QString &connectionId)
{
    Q_UNUSED(connectionId)
    refreshData();
}

void MTProxyWidget::onStatsUpdated(const MTProxyStats &stats)
{
    Q_UNUSED(stats)
    updateStatsDisplay();
}

// FIX #2: установка mtproto-proxy из исходников
void MTProxyWidget::onInstallMTProxy()
{
    int ret = QMessageBox::question(this, "Установить MTProxy",
        "Будет выполнена сборка mtproto-proxy из исходников GitHub.\n"
        "Потребуется: git, make, gcc, libssl-dev, zlib1g-dev.\n\n"
        "Продолжить?", QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    btnInstallMTProxy->setEnabled(false);
    btnInstallMTProxy->setText("⏳ Сборка...");

    // Если уже собран в ~/Загрузки/MTProxy — просто копируем, не клонируем заново
    QString existingBin;
    QStringList knownBuilt = {
        QDir::homePath() + "/Загрузки/MTProxy/objs/bin/mtproto-proxy",
        QDir::homePath() + "/Downloads/MTProxy/objs/bin/mtproto-proxy",
        "/tmp/MTProxy_build/objs/bin/mtproto-proxy",
        "/tmp/MTProxy/objs/bin/mtproto-proxy"
    };
    for (const QString &p : knownBuilt) {
        if (QFile::exists(p)) { existingBin = p; break; }
    }

    QString script;
    if (!existingBin.isEmpty()) {
        // Бинарник уже есть — просто копируем
        script = QString(
            "set -e\n"
            "cp \'%1\' /usr/local/bin/mtproto-proxy\n"
            "chmod +x /usr/local/bin/mtproto-proxy\n"
        ).arg(existingBin);
        emit proxyManager->logMessage("Найден готовый бинарник: " + existingBin + " — копирую...", "info");
    } else {
        // Собираем из исходников
        script =
            "set -e\n"
            "apt-get install -y git build-essential libssl-dev zlib1g-dev 2>/dev/null || true\n"
            "rm -rf /tmp/MTProxy_build\n"
            "git clone --depth 1 https://github.com/TelegramMessenger/MTProxy /tmp/MTProxy_build\n"
            "cd /tmp/MTProxy_build && make -j$(nproc)\n"
            "cp /tmp/MTProxy_build/objs/bin/mtproto-proxy /usr/local/bin/mtproto-proxy\n"
            "chmod +x /usr/local/bin/mtproto-proxy\n";
    }

    QProcess *proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    proc->start("pkexec", {"bash", "-c", script});

    connect(proc, &QProcess::readyReadStandardOutput, this, [proc, this]() {
        QString out = QString::fromUtf8(proc->readAllStandardOutput());
        // forward to main log via proxyManager signal
        emit proxyManager->logMessage(out.trimmed(), "info");
    });

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int code, QProcess::ExitStatus) {
        btnInstallMTProxy->setEnabled(true);
        btnInstallMTProxy->setText("⚙️ Установить MTProxy");
        if (code == 0) {
            proxyManager->setExecutablePath("/usr/local/bin/mtproto-proxy");
            QMessageBox::information(this, "Готово",
                "mtproto-proxy успешно установлен в /usr/local/bin/\n"
                "Теперь можно запустить прокси.");
            emit proxyManager->logMessage("mtproto-proxy установлен: /usr/local/bin/mtproto-proxy", "success");
        } else {
            QString err = QString::fromUtf8(proc->readAllStandardError());
            QMessageBox::warning(this, "Ошибка сборки",
                "Сборка завершилась с ошибкой (код " + QString::number(code) + ").\n\n" + err.left(500));
            emit proxyManager->logMessage("Ошибка установки mtproto-proxy (код " + QString::number(code) + ")", "error");
        }
        proc->deleteLater();
    });
}
