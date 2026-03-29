// mtproxymanager.h
#ifndef MTPROXY_MANAGER_H
#define MTPROXY_MANAGER_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QSet>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QMutex>
#include <QGeoPositionInfoSource>
#include <QGeoCoordinate>

/**
 * Структура для хранения информации о подключении клиента MTProxy
 */
struct MTProxyClient {
    QString connectionId;      // Уникальный ID соединения
    QString clientIp;          // IP адрес клиента
    int clientPort;            // Порт клиента
    QString country;           // Страна по IP
    QString city;              // Город
    QString countryCode;       // Код страны
    double latitude;           // Широта
    double longitude;          // Долгота
    qint64 bytesReceived;      // Получено байт
    qint64 bytesSent;          // Отправлено байт
    qint64 currentSpeed;       // Текущая скорость (байт/с)
    QDateTime connectedSince;  // Время подключения
    QDateTime lastActivity;     // Последняя активность
    QString userAgent;          // User-Agent если доступен
    QString telegramVersion;    // Версия Telegram
    QString deviceModel;        // Модель устройства
    QString osVersion;          // Версия ОС
    bool isActive;              // Активно ли соединение
    int proxyPort;              // Порт прокси (если несколько)
};

/**
 * Статистика по прокси-серверу
 */
struct MTProxyStats {
    int totalConnections;       // Всего подключений за всё время
    int activeConnections;      // Активных подключений сейчас
    qint64 totalBytesRx;        // Всего получено байт
    qint64 totalBytesTx;        // Всего отправлено байт
    qint64 peakConnections;     // Пик одновременных подключений
    QDateTime peakTime;         // Время пика
    double avgSpeed;            // Средняя скорость (байт/с)
    int uniqueUsers;            // Уникальных пользователей
};

/**
 * Главный класс управления MTProxy
 */
class MTProxyManager : public QObject
{
    Q_OBJECT

public:
    explicit MTProxyManager(QObject *parent = nullptr);
    ~MTProxyManager();

    // Управление прокси
    bool startProxy(int port, const QString &secret = QString());
    bool stopProxy();
    bool restartProxy();
    bool isRunning() const { return proxyRunning; }

    // Настройка
    void setExecutablePath(const QString &path) { mtprotoPath = path; }
    void setConfigPath(const QString &path) { configPath = path; }
    void setSecret(const QString &secret);
    QString getSecret() const { return currentSecret; }
    int getPort() const { return currentPort; }

    // Статистика
    MTProxyStats getStats() const;
    QList<MTProxyClient> getActiveClients() const;
    QList<MTProxyClient> getClientHistory(int limit = 100) const;

    // Генерация секрета
    static QString generateSecret();
    static QString generateSecureSecret();

    // Получение ссылки для Telegram
    QString getProxyLink() const;
    QString getProxyLinkForPlatform(const QString &platform = "generic") const;

signals:
    void clientConnected(const MTProxyClient &client);
    void clientDisconnected(const QString &connectionId);
    void clientActivity(const QString &connectionId, qint64 bytesRx, qint64 bytesTx);
    void statsUpdated(const MTProxyStats &stats);
    void logMessage(const QString &message, const QString &level = "info");
    void proxyStarted();
    void proxyStopped();
    void proxyError(const QString &error);

public slots:
    void updateStats();
    void refreshGeoLocation(const QString &ip);

private slots:
    void onProxyProcessOutput();
    void onProxyProcessError();
    void onProxyProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onGeoLookupFinished(QNetworkReply *reply);
    void cleanupInactiveConnections();

private:
    // Парсинг логов MTProxy
    void parseLogLine(const QString &line);
    void parseConnectionLine(const QString &line);
    void parseStatsLine(const QString &line);

    // Работа с БД
    bool initDatabase();
    void saveClientToDatabase(const MTProxyClient &client);
    void updateClientInDatabase(const MTProxyClient &client);
    void closeClientInDatabase(const QString &connectionId);

    // GeoIP
    void lookupGeoIP(const QString &ip);
    QString getCountryFromCache(const QString &ip);

    // Вспомогательные
    bool validateSecret(const QString &secret);
    QString formatBytes(qint64 bytes) const;
    QString formatDuration(qint64 seconds) const;
    void updatePeakConnections();

    // Компоненты
    QProcess *mtprotoProcess = nullptr;
    QTimer *statsTimer = nullptr;
    QTimer *cleanupTimer = nullptr;
    QNetworkAccessManager *geoManager = nullptr;

    // Состояние
    bool proxyRunning = false;
    int currentPort = 443;
    QString currentSecret;
    QString mtprotoPath = "/usr/bin/mtproto-proxy";
    QString configPath;

    // Данные клиентов
    QMap<QString, MTProxyClient> activeClients;
    mutable QMutex clientsMutex;

    // Кэш GeoIP
    QMap<QString, QPair<QString, QString>> geoCache; // IP -> (страна, город)
    QSet<QString> pendingGeoLookups;

    // Статистика
    MTProxyStats currentStats;
    mutable QMutex statsMutex;
    int peakConnections = 0;
    QDateTime peakTime;

    // База данных
    QSqlDatabase db;
    bool dbInitialized = false;

    // Константы
    static const int STATS_UPDATE_INTERVAL = 2000;  // 2 секунды
    static const int CLEANUP_INTERVAL = 5000;       // 5 секунд
    static const int INACTIVE_TIMEOUT = 300;        // 5 минут без активности
    static const int MAX_HISTORY_RECORDS = 10000;
};

/**
 * Виджет для отображения статистики MTProxy
 */
class MTProxyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MTProxyWidget(MTProxyManager *manager, QWidget *parent = nullptr);
    ~MTProxyWidget();

private slots:
    void refreshData();
    void onClientConnected(const MTProxyClient &client);
    void onClientDisconnected(const QString &connectionId);
    void onStatsUpdated(const MTProxyStats &stats);
    void onStartStopClicked();
    void onGenerateSecret();
    void onCopyLink();
    void onExportCSV();
    void onClientTableContextMenu(const QPoint &pos);
    void showClientDetails();
    void banClientIP();

private:
    void setupUI();
    void updateClientsTable();
    void updateStatsDisplay();
    void updateProxyStatus();
    void loadSettings();
    void saveSettings();

    // UI Компоненты
    QGroupBox *controlGroup = nullptr;
    QLineEdit *edtPort = nullptr;
    QLineEdit *edtSecret = nullptr;
    QPushButton *btnStartStop = nullptr;
    QPushButton *btnGenerateSecret = nullptr;
    QPushButton *btnCopyLink = nullptr;
    QLabel *lblStatus = nullptr;
    QLabel *lblProxyLink = nullptr;

    QGroupBox *statsGroup = nullptr;
    QLabel *lblActiveConns = nullptr;
    QLabel *lblTotalConns = nullptr;
    QLabel *lblPeakConns = nullptr;
    QLabel *lblTraffic = nullptr;
    QLabel *lblAvgSpeed = nullptr;
    QLabel *lblUniqueUsers = nullptr;

    QGroupBox *clientsGroup = nullptr;
    QTableWidget *clientsTable = nullptr;
    QPushButton *btnRefresh = nullptr;
    QPushButton *btnExport = nullptr;
    QLineEdit *edtSearch = nullptr;
    QComboBox *cboCountryFilter = nullptr;

    QGroupBox *historyGroup = nullptr;
    QTableWidget *historyTable = nullptr;
    QPushButton *btnClearHistory = nullptr;

    QTimer *refreshTimer = nullptr;

    MTProxyManager *proxyManager = nullptr;
    QMap<QString, int> countryStats;
};

#endif // MTPROXY_MANAGER_H
