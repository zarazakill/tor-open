#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QTcpSocket>
#include <QSettings>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QLineEdit>
#include <QMenu>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMap>
#include <QSet>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QTableWidget>
#include <QScrollBar>
#include <QDateTime>
#include <QCalendarWidget>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QClipboard>
#include <QGuiApplication>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QMutex>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

// Условное включение QTextCodec для разных версий Qt
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringDecoder>
#else
#include <QTextCodec>
#endif

#ifdef Q_OS_LINUX
#include <unistd.h>
#include <sys/types.h>
#endif

#include "tgbot_manager.h"
#include "dns_monitor.h"
#include "client_stats.h"

// Структура для хранения информации о клиенте
struct ClientInfo {
    QString commonName;
    QString realAddress;
    QString virtualAddress;
    QString virtualIPv6;
    qint64 bytesReceived;
    qint64 bytesSent;
    qint64 speedRxBps = 0;
    qint64 speedTxBps = 0;
    QDateTime connectedSince;
    qint64 connectedSinceEpoch;
    qint64 pid;
    bool isActive;
};

// Запись одной сессии
struct SessionRecord {
    QDateTime connectedAt;
    QDateTime disconnectedAt;
    QString sourceIP;
    qint64 durationSeconds;
    qint64 bytesReceived;
    qint64 bytesSent;
    QString vpnIP;
};

// Структура для реестра клиентов (расширенная для Telegram бота)
struct ClientRecord {
    QString displayName;
    QDate expiryDate;
    QDateTime firstSeen;
    QDateTime lastSeen;
    qint64 totalBytesRx = 0;
    qint64 totalBytesTx = 0;
    int sessionCount = 0;
    qint64 totalOnlineSeconds = 0;
    bool isBanned = false;
    int speedLimitKbps = 0;
    QList<SessionRecord> sessions;
    QSet<QString> knownIPs;
    qint64 telegramId = 0;
    QDate registeredAt;
};

/**
 * Класс для асинхронной генерации сертификатов
 */
class CertificateGenerator : public QObject
{
    Q_OBJECT

public:
    explicit CertificateGenerator(QObject *parent = nullptr);
    ~CertificateGenerator();

    void generateCertificates(const QString &certsDir,
                              const QString &openVPNPath,
                              bool useEasyRSA);
signals:
    void logMessage(const QString &message, const QString &type);
    void finished(bool success);
    void progress(int percent);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onProcessOutput();
    void onProcessErrorOutput();

private:
    QProcess *currentProcess;
    QStringList commandQueue;
    int currentCommandIndex;
    QString certsDirectory;
    QString openVPNPath;
    bool useEasyRSAFlag;
    void runNextCommand();
    void runOpenSSLCommand(const QStringList &args, const QString &description);
    void runEasyRSACommand(const QString &cmd, const QString &description);
};

/**
 * Главный класс приложения Tor Manager
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Публичные члены для доступа из main.cpp
    QSystemTrayIcon *trayIcon = nullptr;
    bool torRunning = false;
    bool serverMode = false;

    // Публичные слоты
    DnsMonitor* getDnsMonitor() const { return dnsMonitor; }


public slots:
    void startTor();
    void startOpenVPNServer();
    void updateClientsTable();

    // Методы для доступа к реестру
    const QMap<QString, ClientRecord>& getClientRegistry() const { return clientRegistry; }
    void updateRegistryFromTelegram(const QString &cn, const ClientRecord &record);

    // НОВЫЕ МЕТОДЫ ДЛЯ СТАТИСТИКИ
    void showStatsReport();
    void showClientStats(const QString &clientName);
    void exportStatsToCSV();
    void checkSuspiciousDomains();

    void updateClientStatsAsync();

protected:
    void closeEvent(QCloseEvent *event) override;

signals:
    void serverStarted();
    void clientsUpdated();
    void torStateChanged(bool running);
    void registryUpdated(const QMap<QString, ClientRecord> &registry);
    void clientRegistryChanged();

private slots:
    // Управление Tor
    void stopTor();
    void restartTor();
    void onTorStarted();
    void onTorFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onTorError(QProcess::ProcessError error);
    void onTorReadyRead();
    void checkTorStatus();
    void parseTorCircuitLine(const QString &line);
    void sendTorCommand(const QString &command);
    void onControlSocketConnected();
    void onControlSocketReadyRead();
    void onControlSocketError();
    void requestNewCircuit();

    // Управление сервером
    void stopOpenVPNServer();
    void onServerStarted();
    void onServerFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onServerError(QProcess::ProcessError error);
    void onServerReadyRead();

    // Генерация сертификатов
    void generateCertificates();
    void generateCertificatesAsync();
    void onCertGenerationFinished(bool success);
    void checkCertificates();

    // Генерация клиентских конфигураций
    void generateClientConfig();
    void generateTestAndroidConfig();
    void generateClientCertificate(const QString &clientName);

    // Проверка сети
    void checkIPLeak();
    void onIPCheckFinished();

    // Интерфейс
    void updateStatus();
    void showSettings();
    void showAbout();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void applySettings();
    void updateTrafficStats();
    void addLogMessage(const QString &message, const QString &type = "info");
    void flushLogs();

    // Управление мостами
    void addBridge();
    void removeBridge();
    void importBridgesFromText();
    void validateBridgeFormat();
    void testBridgeConnection(const QString &bridge);
    void updateBridgeConfig();

    // Kill switch
    void enableKillSwitch();
    void disableKillSwitch();
    void setupFirewallRules(bool enable);

    // Диагностика
    void diagnoseConnection();
    void testServerConfig();

    // Маршрутизация
    QString getExternalInterface();
    bool setupIPTablesRules(bool enable);
    void applyRoutingManually();
    void verifyRouting();
    void enableIPForwarding();
    bool checkIPForwarding();

    // Управление клиентами
    void createClientsTab();
    void disconnectSelectedClient();
    void disconnectAllClients();
    void showClientDetails();
    void banClient();
    void applySpeedLimit(const QString &cn, int kbps);
    void exportClientsLog();
    void clearClientsLog();
    void onClientTableContextMenu(const QPoint &pos);
    void refreshClientsNow();
    void generateNamedClientConfig();
    void showClientAnalytics();
    void showClientSessionHistory();
    void loadClientRegistry();
    void saveClientRegistry();
    void updateRegistryTable();
    void applyRegistryFilter();
    void updateRealtimeDurations();
    void checkMultipleConnections();
    void deleteClientPermanently(const QString &cn);

    // Telegram бот
    void createTelegramTab();
    void onBotClientCreated(const QString &cn);
    void onBotClientRevoked(const QString &cn);
    void syncBotWithSettings();

    // URL мониторинг
    void createUrlHistoryTab();
    void addUrlAccess(const UrlAccess &access);
    void updateUrlTable();

    void diagnoseBotConnection();

    // НОВЫЕ СЛОТЫ ДЛЯ СТАТИСТИКИ
    void onSuspiciousActivity(const QString &clientName, const QString &domain, const QString &reason);
    void onStatsUpdated(const QString &clientName, const DnsClientStats &stats);
    void updateStatsDisplay();
    void addSuspiciousToList(const QString &client, const QString &domain);

    void onServerStartedHandler();
    void updateClientMapForDnsMonitor();
    void verifyDnsMonitorRunning();
    void logDnsMonitorDiagnostics();
    void updateClientsTableUI(const QMap<QString, ClientInfo> &newClients, const QDateTime &now);
    void scheduleClientsUpdate(int delayMs = 2000);

private:
    // Инициализация
    void setupUI();
    void setupTrayIcon();
    void setupConnections();
    void createMenuBar();
    void createTabWidget();
    void createTorTab();
    void createServerTab();
    void createSettingsTab();
    void createLogsTab();

    // Создание вкладки статистики
    void setupStatsUI();

    // Конфигурация Tor
    void createTorConfig();
    QString getTorConfigPath() { return torrcPath; }
    QString getTorDataPath() { return torDataDir; }
    bool checkTorInstalled();

    // Конфигурация сервера
    void createServerConfig();
    void createTorRoutingScripts();
    bool checkOpenVPNInstalled();
    bool validateServerConfig();
    QString findEasyRSA();
    QString getLocalIP();
    void updateClientStats();

    // Вспомогательные функции
    bool copyPath(const QString &src, const QString &dst);
    QString decodeClientName(const QByteArray &data);
    QString sanitizeClientName(const QString &name);
    QString resolveTaKeyPath();
    QString resolveClientCertPath(const QString &cn) const;
    QString resolveClientKeyPath(const QString &cn) const;
    QString extractTaKeyFromServerConf() const;

    // Управление мостами
    QString normalizeBridgeLine(const QString &bridge);
    bool validateWebtunnelBridge(const QString &bridge);
    bool validateObfs4Bridge(const QString &bridge);
    QStringList parseBridgeFile(const QString &filePath);
    void saveBridgesToSettings();
    void loadBridgesFromSettings();
    QString detectBridgeType(const QString &bridgeLine);
    QString getTransportPluginPath(const QString &transport);
    bool checkTransportPluginInstalled(const QString &transport);
    void updateBridgeStats();
    QString findLyrebirdPath();

    // Вспомогательные
    bool isProcessRunning(const QString &processName);
    QString executeCommand(const QString &command);
    QString runSafeCommand(const QString &program, const QStringList &args);
    void setConnectionState(const QString &state);
    bool verifyTorConnection();
    void requestExternalIP();
    void loadSettings();
    void saveSettings();

    // Компоненты интерфейса
    QTabWidget *tabWidget = nullptr;

    // Вкладка Tor
    QWidget *torTab = nullptr;
    QPushButton *btnStartTor = nullptr;
    QPushButton *btnStopTor = nullptr;
    QPushButton *btnRestartTor = nullptr;
    QPushButton *btnNewCircuit = nullptr;
    QLabel *lblTorStatus = nullptr;
    QLabel *lblTorIP = nullptr;
    QLabel *lblCircuitInfo = nullptr;
    QTextEdit *txtTorLog = nullptr;
    QComboBox *cboBridgeType = nullptr;
    QListWidget *lstBridges = nullptr;
    QPushButton *btnAddBridge = nullptr;
    QPushButton *btnRemoveBridge = nullptr;
    QLabel *lblTrafficStats = nullptr;
    QPushButton *btnImportBridges = nullptr;
    QPushButton *btnTestBridge = nullptr;
    QLabel *lblBridgeStats = nullptr;

    // Вкладка сервера
    QWidget *serverTab = nullptr;
    QGroupBox *serverGroup = nullptr;
    QSpinBox *spinServerPort = nullptr;
    QLineEdit *txtServerNetwork = nullptr;
    QCheckBox *chkRouteThroughTor = nullptr;
    QCheckBox *chkExternalSocks = nullptr;
    QCheckBox *chkExternalTrans = nullptr;
    QCheckBox *chkExternalDns = nullptr;
    QSpinBox  *spinTransPort = nullptr;
    QSpinBox  *spinDnsPort = nullptr;
    QPushButton *btnGenerateCerts = nullptr;
    QPushButton *btnCheckCerts = nullptr;
    QPushButton *btnStartServer = nullptr;
    QPushButton *btnStopServer = nullptr;
    QLabel *lblServerStatus = nullptr;
    QLabel *lblConnectedClients = nullptr;
    QTextEdit *txtServerLog = nullptr;
    QComboBox *cboServerProto = nullptr;
    QComboBox *cboDataCipher = nullptr;
    QComboBox *cboTlsCipher = nullptr;
    QComboBox *cboHmacAuth = nullptr;
    QComboBox *cboTlsMinVer = nullptr;
    QComboBox *cboTlsMode = nullptr;
    QCheckBox *chkPfs = nullptr;
    QCheckBox *chkCompression = nullptr;
    QCheckBox *chkDuplicateCN = nullptr;
    QCheckBox *chkClientToClient = nullptr;
    QSpinBox  *spinMaxClients = nullptr;
    QSpinBox  *spinMtu = nullptr;
    QLabel *lblCurrentIP = nullptr;
    QPushButton *btnCheckIP = nullptr;
    QPushButton *btnGenerateClientConfig = nullptr;
    QPushButton *btnDiagnose = nullptr;
    QPushButton *btnTestConfig = nullptr;

    // Вкладка клиентов
    QWidget *clientsTab = nullptr;
    QTableWidget *clientsTable = nullptr;
    QTableWidget *registryTable = nullptr;
    QLineEdit   *regSearchEdit = nullptr;
    QComboBox   *regFilterCombo = nullptr;
    QTextEdit *txtClientsLog = nullptr;
    QPushButton *btnDisconnectClient = nullptr;
    QPushButton *btnDisconnectAll = nullptr;
    QPushButton *btnRefreshClients = nullptr;
    QPushButton *btnClientDetails = nullptr;
    QPushButton *btnClientAnalytics = nullptr;
    QPushButton *btnClientHistory = nullptr;
    QPushButton *btnBanClient = nullptr;
    QPushButton *btnExportClientsLog = nullptr;
    QPushButton *btnClearClientsLog = nullptr;
    QPushButton *btnAddNamedClient = nullptr;
    QPushButton *btnDeleteClientPermanent = nullptr;
    QLabel *lblTotalClients = nullptr;
    QLabel *lblActiveClients = nullptr;
    QLabel *lblOnlineClients = nullptr;
    QTimer *clientsRefreshTimer = nullptr;

    // НОВЫЕ UI ЭЛЕМЕНТЫ ДЛЯ СТАТИСТИКИ
    QWidget *statsTab = nullptr;              // Новая вкладка статистики
    QTableWidget *statsTable = nullptr;        // Таблица статистики по клиентам
    QTextEdit *suspiciousLog = nullptr;        // Лог подозрительной активности
    QPushButton *btnGenerateReport = nullptr;  // Кнопка генерации отчета
    QPushButton *btnExportStats = nullptr;     // Кнопка экспорта статистики
    QComboBox *cmbClientSelect = nullptr;      // Выбор клиента для отчета
    QLabel *lblTotalRequests = nullptr;        // Общая статистика
    QLabel *lblSuspiciousCount = nullptr;      // Счетчик подозрительных

    // Кэш клиентов
    QMap<QString, ClientInfo> clientsCache;
    qint64 lastClientRefreshMs = 0;
    QMap<QString, ClientRecord> clientRegistry;
    QMap<QString, QStringList> activeIPsPerClient;

    // Мьютекс для потокобезопасного доступа к реестру
    QMutex registryMutex;

    // Вкладка настроек
    QWidget *settingsTab = nullptr;
    QSpinBox *spinTorSocksPort = nullptr;
    QSpinBox *spinTorControlPort = nullptr;
    QCheckBox *chkAutoStart = nullptr;
    QCheckBox *chkKillSwitch = nullptr;
    QCheckBox *chkBlockIPv6 = nullptr;
    QCheckBox *chkDNSLeakProtection = nullptr;
    QCheckBox *chkStartMinimized = nullptr;
    QLineEdit *txtTorPath = nullptr;
    QLineEdit *txtOpenVPNPath = nullptr;
    QPushButton *btnApplySettings = nullptr;
    QPushButton *btnBrowseTor = nullptr;
    QPushButton *btnBrowseOpenVPN = nullptr;
    QLineEdit  *txtServerAddress = nullptr;
    QComboBox  *cboLogLevelSetting = nullptr;
    QSpinBox   *spinRefreshInterval = nullptr;
    QCheckBox  *chkWriteAllLogs = nullptr;
    QCheckBox  *chkShowTrayNotifications = nullptr;

    // Вкладка журналов
    QWidget *logsTab = nullptr;
    QTextEdit *txtAllLogs = nullptr;
    QComboBox *cboLogLevel = nullptr;
    QPushButton *btnClearLogs = nullptr;
    QPushButton *btnSaveLogs = nullptr;

    // Вкладка URL истории
    QWidget *urlHistoryTab = nullptr;
    QTableWidget *urlTable = nullptr;
    QComboBox *urlClientFilter = nullptr;
    QComboBox *urlMethodFilter = nullptr;
    QLabel *urlStatsLabel = nullptr;
    QList<UrlAccess> urlHistory;

    // Системный трей
    QMenu *trayMenu = nullptr;

    // Процессы
    QProcess *torProcess = nullptr;
    QProcess *openVPNServerProcess = nullptr;
    CertificateGenerator *certGenerator = nullptr;
    DnsMonitor *dnsMonitor = nullptr;

    // Telegram бот
    TelegramBotManager *tgBotManager = nullptr;

    // Сеть
    QTcpSocket *controlSocket = nullptr;
    QNetworkAccessManager *ipCheckManager = nullptr;

    // Таймеры
    QTimer *statusTimer = nullptr;
    QTimer *trafficTimer = nullptr;
    QTimer *clientStatsTimer = nullptr;
    QTimer *realtimeTimer = nullptr;

    // Клиентская статистика
    ClientStats *clientStats = nullptr;
    QStringList logBuffer;       // буфер UI-сообщений
    QStringList fileLogBuffer;    // буфер записи в файл (thread-safe)
    QMutex logMutex;
    QTimer *logFlushTimer = nullptr;
    QTimer *m_clientsUpdateDebounce = nullptr;  // дебаунс обновления таблицы клиентов

    // Настройки
    QSettings *settings = nullptr;

    // Переменные состояния
    bool killSwitchEnabled = false;
    bool controlSocketConnected = false;
    bool torCircuitSubscribed = false;
    QString controlSocketBuffer;
    bool serverStopPending = false;
    int serverTorWaitRetries = 0;
    QString currentConnectionState;
    QString currentIP;
    QString torIP;
    quint64 bytesReceived = 0;
    quint64 bytesSent = 0;
    int connectedClients = 0;
    QString tempLinkPath;

    // Пути
    QString torrcPath;
    QString torDataDir;
    QString serverConfigPath;
    QString torExecutablePath;
    QString openVPNExecutablePath;

    // Пути к сертификатам
    QString certsDir;
    QString caCertPath;
    QString serverCertPath;
    QString serverKeyPath;
    QString dhParamPath;
    QString taKeyPath;

    // Мосты
    QStringList configuredBridges;
    QMap<QString, QString> transportPluginPaths;

    // Константы
    static const int DEFAULT_TOR_SOCKS_PORT;
    static const int DEFAULT_TOR_CONTROL_PORT;
    static const int DEFAULT_VPN_SERVER_PORT;
    static const int MW_MAX_LOG_LINES;
    static const int MW_BRIDGE_TEST_TIMEOUT;
    static const int MW_CLIENT_STATS_UPDATE_INTERVAL;

    // Состояние расширенное
    QString  logLevel = "all";
    QDateTime serverStartTime;
    QString  allLogsFilePath;
    int torCrashCount = 0;

    // Вспомогательные функции форматирования
    QString formatBytes(qint64 bytes) const;
    QString formatDuration(qint64 seconds) const;
    QString formatSpeed(qint64 bps) const;

};

#endif // MAINWINDOW_H
