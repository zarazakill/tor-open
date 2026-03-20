#ifndef TGBOT_MANAGER_H
#define TGBOT_MANAGER_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <QVariant>
#include <QDateTime>
#include <QDate>
#include <QSettings>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QMutex>
#include <QPointer>

// ─── UI виджеты ──────────────────────────────────────────────────────────────
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QTabWidget>
#include <QSplitter>
#include <QScrollArea>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QProgressBar>
#include <QListWidget>
#include <QFrame>

// ─── Предварительное объявление ─────────────────────────────────────────────
class MainWindow;  // ВАЖНО: добавляем предварительное объявление

// ─── Структура ClientRecord из mainwindow.h ─────────────────────────────────
struct ClientRecord;

// ─── Структуры данных ─────────────────────────────────────────────────────────

/**
 * Информация о VPN-клиенте из реестра
 */
struct TgClientRecord {
    QString cn;                // Common Name (уникальный идентификатор)
    QString displayName;       // Отображаемое имя
    QDate   expiryDate;        // Дата истечения (нулевая = без ограничений)
    QDate   registeredAt;      // Дата регистрации
    qint64  telegramId = 0;    // Telegram user ID (для поиска по пользователю)
    QDateTime firstSeen;
    QDateTime lastSeen;
    qint64  totalBytesRx = 0;
    qint64  totalBytesTx = 0;
    int     sessionCount = 0;
    qint64  totalOnlineSecs = 0;
    bool    isBanned = false;
    QStringList knownIPs;
};

/**
 * Структура для результата генерации сертификата
 */
struct CertGenerationResult {
    bool success = false;
    QString errorDetails;       // Детальное описание ошибки
    QString opensslOutput;      // Вывод openssl stdout
    QString opensslError;       // Вывод openssl stderr
    QString step;               // На каком шаге произошла ошибка
};

/**
 * Входящее сообщение из Telegram
 */
struct TgMessage {
    qint64  messageId = 0;
    qint64  chatId = 0;
    qint64  userId = 0;
    QString firstName;
    QString lastName;
    QString username;
    QString text;
    QString callbackData;       // для inline кнопок
    bool    isCallback = false;
    int     lastBotMessageId = 0;  // для редактирования сообщений бота
};

/**
 * Состояние диалога создания пользователя (ConversationHandler аналог)
 */
enum class ConvState {
    None,
    WaitingConsent,          // пользователь должен принять соглашение
    WaitingName,
    WaitingExpiryChoice,
    WaitingExpiryDate,
    WaitingConfirm,
    WaitingRevokeConfirm,
};

struct ConvContext {
    ConvState state = ConvState::None;
    QString   pendingCN;
    QDate     pendingExpiry;
    bool      hasExpiry = false;
    qint64    chatId = 0;
    int       lastBotMessageId = 0;
    QString   revokeTarget;
    QString   pendingFirstName;  // имя для согласия
    QString   pendingLastName;
    QString   pendingUsername;
};

// ─── Основной класс управления ботом ─────────────────────────────────────────

/**
 * TelegramBotManager — управляет Telegram ботом через Bot API (long polling).
 * Интегрируется как вкладка в MainWindow.
 * Все операции с сетью — через QNetworkAccessManager (асинхронно).
 * Генерация сертификатов — через QtConcurrent (не блокирует event loop).
 */
class TelegramBotManager : public QObject
{
    Q_OBJECT

public:
    explicit TelegramBotManager(QSettings *settings, QObject *parent = nullptr);
    ~TelegramBotManager();

    // Панель управления ботом (статус + кнопка Запуск/Стоп) — встраивается в вкладку «Клиенты»
    QWidget* buildBotControlPanel(QWidget *parent = nullptr);

    // Секция настроек бота — встраивается в вкладку «Настройки» MainWindow
    QWidget* buildSettingsSection(QWidget *parent = nullptr);

    // Секция журнала бота — встраивается в вкладку «Журналы» MainWindow
    QWidget* buildLogSection(QWidget *parent = nullptr);

    // Синхронизация путей с MainWindow
    void setCertsDir(const QString &path) {
        certsDir = path;
        if (lblCertsDirInfo) lblCertsDirInfo->setText(path);
    }

    void setServerConfPath(const QString &path) {
        serverConfPath = path;
    }

    void setRegistryIni(const QString &path) {
        registryIniPath = path;
        if (lblRegistryInfo) lblRegistryInfo->setText(path);
    }

    void setServerAddress(const QString &addr, int port, const QString &proto = "udp") {
        serverAddress = addr;
        serverPort = port;
        serverProto = proto;
    }

    bool isRunning() const { return pollingActive; }

    void setBotToken(const QString &token) {
        botToken = token;
        if (edtToken) edtToken->setText(token);
    }

signals:
    void logMessage(const QString &msg, const QString &level = "info");
    void statusChanged(bool running);
    void clientCreated(const QString &cn);
    void clientRevoked(const QString &cn);
    void clientsChanged();   // запросить обновление общей таблицы клиентов
    void caGenerated();      // CA был создан ботом — mainwindow может обновить пути

public slots:
    void startPolling();
    void stopPolling();
    void onRegistryUpdated(const QMap<QString, ClientRecord> &registry);  // Получение реестра из MainWindow
    void handleStats(const TgMessage &msg);
    void handleSuspicious(const TgMessage &msg);
    void handleTopDomains(const TgMessage &msg);

private slots:
    // Long polling
    void doPoll();
    void onPollReply(QNetworkReply *reply);

    // Отправка сообщений
    void sendMessage(qint64 chatId, const QString &text,
                     const QJsonArray &inlineKeyboard = QJsonArray(),
                     const QString &parseMode = "HTML");
    void editMessage(qint64 chatId, int messageId, const QString &text,
                     const QJsonArray &inlineKeyboard = QJsonArray(),
                     const QString &parseMode = "HTML");
    void answerCallbackQuery(const QString &callbackQueryId, const QString &text = "");
    void sendDocument(qint64 chatId, const QString &filePath,
                      const QString &caption = "", const QString &parseMode = "HTML");

    // Обработка обновлений
    void processUpdate(const QJsonObject &update);
    void processMessage(const TgMessage &msg);
    void processCallback(const TgMessage &msg);

    // Команды
    void handleStart(const TgMessage &msg);
    void handleHelp(const TgMessage &msg);
    void handleStatus(const TgMessage &msg);
    void handleList(const TgMessage &msg);
    void handleNewUser(const TgMessage &msg);
    void handleRevoke(const TgMessage &msg);
    void handleGetConfig(const TgMessage &msg);
    void handleInstruction(const TgMessage &msg);
    void handleCancel(const TgMessage &msg);
    void handleNewUserForNonAdmin(const TgMessage &msg);   // автосоздание для обычных юзеров
    void sendExistingConfig(const TgMessage &msg, const QString &cn); // повтор отправки

    // ConversationHandler
    void handleConvMessage(const TgMessage &msg, ConvContext &ctx);
    void handleConvCallback(const TgMessage &msg, ConvContext &ctx);
    void convConsent(const TgMessage &msg, ConvContext &ctx);          // обработка согласия
    void convGotName(const TgMessage &msg, ConvContext &ctx);
    void convExpiryChoice(const TgMessage &msg, ConvContext &ctx);
    void convGotExpiryDate(const TgMessage &msg, ConvContext &ctx);
    void convDoConfirm(const TgMessage &msg, ConvContext &ctx);
    void convConfirm(const TgMessage &msg, ConvContext &ctx);
    void convRevokeConfirm(const TgMessage &msg, ConvContext &ctx);

    // Операции с клиентами
    bool generateCA();                          // создать CA если нет
    CertGenerationResult generateClientCertDetailed(const QString &cn); // детальная генерация
    QString buildOvpnConfig(const QString &cn, const QDate &expiry);
    bool writeRegistryClient(const QString &cn, const QDate &expiry);
    bool banClientInRegistry(const QString &cn, bool banned);
    bool revokeClientCerts(const QString &cn);
    QMap<QString, TgClientRecord> readRegistry();  // теперь использует кэш
    void saveRegistry(const QMap<QString, TgClientRecord> &clients);
    void saveClient(const QString &cn, TgClientRecord &rec); // добавить/обновить одну запись
    void logCertGenerationError(const QString &cn, const CertGenerationResult &result);

    // Асинхронная генерация сертификата (не блокирует event loop)
    void generateClientCertAsync(const QString &cn, const QDate &expiry,
                                 bool hasExpiry, qint64 chatId,
                                 int editMsgId, bool isAdminFlow,
                                 const TgMessage &originalMsg);

    // Обработка сетевых ошибок
    void handleNetworkError(QNetworkReply *reply, const QString &context);
    void retryFailedRequest(QNetworkRequest request, QByteArray payload,
                            int retryCount, qint64 chatId, const QString &context);

    // UI слоты
    void onBtnStartStop();
    void onBtnSaveSettings();
    void onBtnTestToken();
    void onBtnAddAdmin();
    void onBtnRemoveAdmin();
    void onBtnSendBroadcast();
    void onBtnClearLog();
    void onTokenVisibilityToggle();
    void onPollTimeout();

    void addLog(const QString &msg, const QString &level = "info");
    void updateStatusLabel();
    void updateClientsTable();
    void refreshAdminsList();

private:
    // Вспомогательные
    bool isAdmin(qint64 userId) const;
    QString sanitizeCN(const QString &name) const;
    QString formatBytes(qint64 bytes) const;
    QString formatDuration(qint64 seconds) const;
    QDate   parseDate(const QString &s) const;
    bool    certExists(const QString &cn) const;
    QString escapeMarkdownV2(const QString &text) const;
    QString htmlEscape(const QString &text) const;
    QString extractTaKeyFromServerConf();
    QJsonArray makeInlineKeyboard(const QList<QList<QPair<QString,QString>>> &rows) const;
    QNetworkRequest createTelegramRequest(const QString &url, int timeoutMs = 45000);

    bool checkOpenSSLInstalled();
    bool generateClientCert(const QString &cn);

    // Функции шифрования
    QString simpleXorEncrypt(const QString &input, const QString &key);
    QString simpleXorDecrypt(const QString &input, const QString &key);

    // НОВЫЙ МЕТОД: получение MainWindow
    MainWindow* getMainWindow() const;  // Объявление метода

    // Настройки
    QSettings *settings = nullptr;
    void loadSettings();
    void saveSettings();

    // Состояние бота
    bool    pollingActive = false;
    QString botToken;
    QList<qint64> adminIds;
    int     pollingOffset = 0;
    int     pollTimeoutSec = 25;
    int     reconnectDelay = 1000;
    QString serverAddress;
    int     serverPort    = 1194;
    QString serverProto   = "udp";
    QString certsDir;
    QString serverConfPath;   // Путь к server.conf — для извлечения актуального ta.key
    QString registryIniPath;
    QString ovpnOutDir;

    // Пользователи давшие согласие на обработку данных
    QSet<qint64> consentGiven;
    QSet<qint64> pendingCertChats;  // защита от двойного нажатия кнопки

    // Сеть
    QNetworkAccessManager *nam = nullptr;
    QNetworkReply *currentPollReply = nullptr;
    QTimer *reconnectTimer = nullptr;

    // Структура для отслеживания повторных попыток
    struct PendingRequest {
        QNetworkRequest request;
        QByteArray payload;
        int retryCount = 0;
        qint64 chatId = 0;
        QString context;
    };
    QMap<QNetworkReply*, PendingRequest> pendingRequests;

    // Диалоги (chatId -> контекст) с мьютексом
    QMap<qint64, ConvContext> conversations;
    QMutex conversationsMutex;

    // Кэш реестра (синхронизируется с MainWindow)
    QMutex registryMutex;
    QMap<QString, TgClientRecord> cachedRegistry;

    // ─── UI элементы ─────────────────────────────────────────────────────────
    QWidget    *tabWidget      = nullptr;

    // Вкладка конфигурации
    QLineEdit  *edtToken       = nullptr;
    QPushButton *btnToggleToken = nullptr;
    QListWidget *lstAdmins     = nullptr;
    QLineEdit  *edtNewAdminId  = nullptr;
    QPushButton *btnAddAdmin   = nullptr;
    QPushButton *btnRemoveAdmin = nullptr;
    QLineEdit  *edtServerAddr  = nullptr;
    QSpinBox   *spinPort       = nullptr;
    QComboBox  *cboProto       = nullptr;
    QLineEdit  *edtOvpnOutDir  = nullptr;
    QPushButton *btnBrowseOvpnOut   = nullptr;
    QLabel     *lblCertsDirInfo     = nullptr;   // отображает текущий путь
    QLabel     *lblRegistryInfo     = nullptr;   // отображает текущий путь
    QSpinBox   *spinPollTimeout     = nullptr;
    QPushButton *btnSaveSettings    = nullptr;
    QPushButton *btnTestToken       = nullptr;
    QPushButton *btnStartStop       = nullptr;
    QLabel     *lblBotStatus        = nullptr;
    QLabel     *lblBotInfo          = nullptr;
    QProgressBar *pbPollActivity    = nullptr;

    // Вкладка трансляции
    QTextEdit  *edtBroadcast        = nullptr;
    QPushButton *btnSendBroadcast   = nullptr;
    QComboBox  *cboBroadcastTarget  = nullptr;

    // Вкладка лога
    QTextEdit  *txtBotLog           = nullptr;
    QPushButton *btnClearLog        = nullptr;
    QComboBox  *cboLogFilter        = nullptr;

    // Стили
    static const QString STYLE_RUNNING;
    static const QString STYLE_STOPPED;
    static const QString STYLE_ERROR;
};

#endif // TGBOT_MANAGER_H
