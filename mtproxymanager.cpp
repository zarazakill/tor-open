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

bool MTProxyManager::startProxy(int port, const QString &secret)
{
    if (proxyRunning) {
        emit logMessage("Прокси уже запущен", "warning");
        return false;
    }

    if (mtprotoPath.isEmpty()) {
        emit logMessage("Путь к MTProto-Proxy не задан", "error");
        return false;
    }

    currentPort = port;
    if (!secret.isEmpty()) {
        currentSecret = secret;
    } else if (currentSecret.isEmpty()) {
        currentSecret = generateSecret();
    }

    if (!validateSecret(currentSecret)) {
        emit logMessage("Неверный формат секрета", "error");
        return false;
    }

    // Подготовка аргументов
    QStringList args;
    args << "--port" << QString::number(port);
    args << "--secret" << currentSecret;
    args << "--nat-info" << "0.0.0.0:0" << "0.0.0.0:0";
    args << "--allow-skip-dh" << "--no-dh";

    // Дополнительные настройки для лучшей производительности
    args << "--max-connections" << "10000";
    args << "--max-special-connections" << "1000";
    args << "--workers" << "4";

    emit logMessage("Запуск MTProxy на порту " + QString::number(port), "info");
    emit logMessage("Секрет: " + currentSecret, "info");

    mtprotoProcess->start(mtprotoPath, args);

    if (!mtprotoProcess->waitForStarted(5000)) {
        emit logMessage("Не удалось запустить MTProxy: " + mtprotoProcess->errorString(), "error");
        emit proxyError(mtprotoProcess->errorString());
        return false;
    }

    proxyRunning = true;
    statsTimer->start();
    cleanupTimer->start();

    // Сброс статистики при новом запуске
    {
        QMutexLocker locker(&statsMutex);
        currentStats = MTProxyStats();
        peakConnections = 0;
    }

    emit proxyStarted();
    emit logMessage("MTProxy успешно запущен", "success");

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

    // Закрываем все активные соединения в БД
    {
        QMutexLocker locker(&clientsMutex);
        for (const QString &connId : activeClients.keys()) {
            closeClientInDatabase(connId);
        }
        activeClients.clear();
    }

    if (mtprotoProcess->state() == QProcess::Running) {
        mtprotoProcess->terminate();
        if (!mtprotoProcess->waitForFinished(5000)) {
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
    query.prepare(
        "UPDATE connections SET bytes_received=?, bytes_sent=?, last_activity=? WHERE id=?"
    );
    query.addBindValue(client.bytesReceived);
    query.addBindValue(client.bytesSent);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(client.connectionId);
    query.exec();
}

void MTProxyManager::closeClientInDatabase(const QString &connectionId)
{
    if (!dbInitialized) return;

    QSqlQuery query(db);
    query.prepare(
        "UPDATE connections SET disconnected_at=?, "
        "duration_seconds=(JULIANDAY(disconnected_at)-JULIANDAY(connected_at))*86400 "
        "WHERE id=?"
    );
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(connectionId);
    query.exec();
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
    // Более безопасный секрет с дополнительной энтропией
    QByteArray secret(32, 0);
    for (int i = 0; i < 32; i++) {
        secret[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    return secret.toHex();
}

bool MTProxyManager::validateSecret(const QString &secret)
{
    // Секрет должен быть hex строкой чётной длины (16-64 символа)
    if (secret.isEmpty()) return false;
    if (secret.length() % 2 != 0) return false;
    if (secret.length() < 16 || secret.length() > 64) return false;

    static QRegularExpression hexRe("^[0-9a-fA-F]+$");
    return hexRe.match(secret).hasMatch();
}

QString MTProxyManager::getProxyLink() const
{
    // Формат: tg://proxy?server=DOMAIN&port=PORT&secret=SECRET
    return QString("tg://proxy?server=wwcat.duckdns.org&port=%1&secret=%2")
    .arg(currentPort).arg(currentSecret);
}

QString MTProxyManager::getProxyLinkForPlatform(const QString &platform) const
{
    if (platform == "ios") {
        // iOS формат
        return QString("https://t.me/socks?server=wwcat.duckdns.org&port=%1&secret=%2")
        .arg(currentPort).arg(currentSecret);
    } else if (platform == "android") {
        // Android формат
        return getProxyLink();
    } else if (platform == "desktop") {
        // Desktop формат
        return QString("socks5://wwcat.duckdns.org:%1").arg(currentPort);
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

    controlLayout->addWidget(new QLabel("Секрет:"), 0, 2);
    edtSecret = new QLineEdit();
    edtSecret->setPlaceholderText("Автоматически генерируется");
    edtSecret->setReadOnly(true);
    controlLayout->addWidget(edtSecret, 0, 3, 1, 2);

    btnGenerateSecret = new QPushButton("🎲 Генерация");
    controlLayout->addWidget(btnGenerateSecret, 0, 5);

    btnStartStop = new QPushButton("▶ Запустить");
    btnStartStop->setStyleSheet("QPushButton { background: #27ae60; color: white; font-weight: bold; }");
    controlLayout->addWidget(btnStartStop, 0, 6);

    btnCopyLink = new QPushButton("📋 Копировать ссылку");
    controlLayout->addWidget(btnCopyLink, 1, 0, 1, 2);

    lblStatus = new QLabel("Статус: ○ Остановлен");
    lblStatus->setStyleSheet("color: #7f8c8d; font-weight: bold;");
    controlLayout->addWidget(lblStatus, 1, 2, 1, 4);

    lblProxyLink = new QLabel();
    lblProxyLink->setStyleSheet("color: #2980b9; font-size: 10px; font-family: monospace;");
    lblProxyLink->setWordWrap(true);
    controlLayout->addWidget(lblProxyLink, 2, 0, 1, 7);

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
    connect(btnCopyLink, &QPushButton::clicked, this, &MTProxyWidget::onCopyLink);
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
        historyTable->setItem(i, 3, new QTableWidgetItem("—")); // Время отключения
        historyTable->setItem(i, 4, new QTableWidgetItem(fmtDuration(c.bytesReceived / 1024)));
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
        lblProxyLink->setText("🔗 " + proxyManager->getProxyLink());
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
            secret = MTProxyManager::generateSecret();
            edtSecret->setText(secret);
        }

        if (proxyManager->startProxy(port, secret)) {
            saveSettings();
        }
    }
}

void MTProxyWidget::onGenerateSecret()
{
    QString secret = MTProxyManager::generateSecureSecret();
    edtSecret->setText(secret);

    if (proxyManager->isRunning()) {
        // Перезапуск с новым секретом
        proxyManager->setSecret(secret);
    }
}

void MTProxyWidget::onCopyLink()
{
    if (!proxyManager->isRunning()) return;

    QString link = proxyManager->getProxyLink();
    QApplication::clipboard()->setText(link);

    QMessageBox::information(this, "Ссылка скопирована",
                             "Ссылка для подключения к прокси скопирована в буфер обмена.\n\n"
                             "В Telegram: Настройки → Данные и память → Прокси → Вставить ссылку");
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
    edtSecret->setText(settings.value("secret").toString());
}

void MTProxyWidget::saveSettings()
{
    if (!edtPort || !edtSecret) return;
    
    QSettings settings("TorManager", "MTProxy");
    settings.setValue("port", edtPort->text());
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
