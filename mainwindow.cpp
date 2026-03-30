#include "mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QCloseEvent>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QMenuBar>
#include <QStatusBar>
#include <QInputDialog>
#include <QNetworkProxy>
#include <QClipboard>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QDesktopServices>
#include <QApplication>
#include <QHeaderView>
#include <QMenu>
#include <QCalendarWidget>
#include <QDialogButtonBox>
#include <QProgressDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <csignal>
#include <QGuiApplication>
#include <QStringConverter>

#ifdef Q_OS_LINUX
#include <unistd.h>
#include <sys/types.h>
#endif

#include "tgbot_manager.h"
#include "mtproxymanager.h"

// Определения статических констант
const int MainWindow::DEFAULT_TOR_SOCKS_PORT = 9050;
const int MainWindow::DEFAULT_TOR_CONTROL_PORT = 9051;
const int MainWindow::DEFAULT_VPN_SERVER_PORT = 1194;
const int MainWindow::MAX_LOG_LINES = 10000;
const int MainWindow::BRIDGE_TEST_TIMEOUT = 5000;
const int MainWindow::CLIENT_STATS_UPDATE_INTERVAL = 5000;

// ========== РЕАЛИЗАЦИЯ CERTIFICATEGENERATOR ==========

CertificateGenerator::CertificateGenerator(QObject *parent)
: QObject(parent)
, currentProcess(nullptr)
, currentCommandIndex(0)
, useEasyRSAFlag(false)
{
    currentProcess = new QProcess(this);

    connect(currentProcess, &QProcess::finished,
            this, &CertificateGenerator::onProcessFinished);
    connect(currentProcess, &QProcess::errorOccurred,
            this, &CertificateGenerator::onProcessError);
    connect(currentProcess, &QProcess::readyReadStandardOutput,
            this, &CertificateGenerator::onProcessOutput);
    connect(currentProcess, &QProcess::readyReadStandardError,
            this, &CertificateGenerator::onProcessErrorOutput);
}

void CertificateGenerator::generateCertificates(const QString &certsDir,
                                                const QString &openVPNPath,
                                                bool useEasyRSA)
{
    this->certsDirectory = certsDir;
    this->openVPNPath = openVPNPath;
    this->useEasyRSAFlag = useEasyRSA;
    this->commandQueue.clear();
    this->currentCommandIndex = 0;

    QDir().mkpath(certsDir);

    emit logMessage("Начало генерации сертификатов...", "info");
    emit progress(0);

    if (useEasyRSAFlag) {
        commandQueue << "init-pki"
        << "build-ca nopass"
        << "build-server-full server nopass"
        << "gen-dh";
    } else {
        commandQueue << "genrsa_ca"
        << "req_ca"
        << "genrsa_server"
        << "req_server"
        << "sign_server"
        << "dhparam"
        << "tlsauth";
    }

    runNextCommand();
}

void MainWindow::diagnoseBotConnection()
{
    addLogMessage("=== ДИАГНОСТИКА TELEGRAM БОТА ===", "info");

    if (!tgBotManager) {
        addLogMessage("❌ tgBotManager не инициализирован", "error");
        return;
    }

    addLogMessage("✅ tgBotManager инициализирован", "info");

    // Проверка токена
    QString token = settings->value("telegram/botToken").toString();
    addLogMessage(QString("Токен: %1...%2")
    .arg(token.left(8))
    .arg(token.right(4)), token.isEmpty() ? "error" : "info");

    // Проверка сети
    QNetworkAccessManager *nam = new QNetworkAccessManager(this);
    QNetworkReply *reply = nam->get(QNetworkRequest(QUrl("https://api.telegram.org")));
    connect(reply, &QNetworkReply::finished, this, [reply, nam, this]() {
        if (reply->error() == QNetworkReply::NoError) {
            addLogMessage("✅ Доступ к Telegram API есть", "success");
        } else {
            addLogMessage(QString("❌ Ошибка доступа к Telegram: %1")
            .arg(reply->errorString()), "error");
        }
        reply->deleteLater();
        nam->deleteLater();
    });
}

void CertificateGenerator::runNextCommand()
{
    if (currentCommandIndex >= commandQueue.size()) {
        // Копируем файлы в целевую директорию
        if (useEasyRSAFlag) {
            QString workDir = certsDirectory + "/easy-rsa/pki";
            QFile::copy(workDir + "/ca.crt", certsDirectory + "/ca.crt");
            QFile::copy(workDir + "/issued/server.crt", certsDirectory + "/server.crt");
            QFile::copy(workDir + "/private/server.key", certsDirectory + "/server.key");
            QFile::copy(workDir + "/dh.pem", certsDirectory + "/dh.pem");
        }

        emit finished(true);
        return;
    }

    QString cmd = commandQueue[currentCommandIndex];
    emit progress((currentCommandIndex * 100) / commandQueue.size());

    if (useEasyRSAFlag) {
        runEasyRSACommand(cmd, cmd);
    } else {
        if (cmd == "genrsa_ca") {
            runOpenSSLCommand({"genrsa", "-out", certsDirectory + "/ca.key", "2048"},
                              "Генерация CA ключа");
        } else if (cmd == "req_ca") {
            runOpenSSLCommand({"req", "-new", "-x509", "-days", "3650",
                "-key", certsDirectory + "/ca.key",
                "-out", certsDirectory + "/ca.crt",
                "-subj", "/C=RU/ST=Moscow/L=Moscow/O=TorManager/CN=TorManager CA"},
                "Генерация CA сертификата");
        } else if (cmd == "genrsa_server") {
            runOpenSSLCommand({"genrsa", "-out", certsDirectory + "/server.key", "2048"},
                              "Генерация ключа сервера");
        } else if (cmd == "req_server") {
            runOpenSSLCommand({"req", "-new",
                "-key", certsDirectory + "/server.key",
                "-out", certsDirectory + "/server.csr",
                "-subj", "/C=RU/ST=Moscow/L=Moscow/O=TorManager/CN=server"},
                "Генерация CSR сервера");
        } else if (cmd == "sign_server") {
            runOpenSSLCommand({"x509", "-req",
                "-in", certsDirectory + "/server.csr",
                "-CA", certsDirectory + "/ca.crt",
                "-CAkey", certsDirectory + "/ca.key",
                "-CAcreateserial",
                "-out", certsDirectory + "/server.crt",
                // FIX: server cert 365 дней — слишком мало. CA живёт 10 лет (3650).
                // Устанавливаем 3 года (1095 дней) для серверного сертификата.
                "-days", "1095"},
                "Подпись сертификата сервера");
        } else if (cmd == "dhparam") {
            emit logMessage("Генерация DH параметров (может занять несколько минут)...", "info");
            runOpenSSLCommand({"dhparam", "-out", certsDirectory + "/dh.pem", "2048"},
                              "Генерация DH параметров");
        } else if (cmd == "tlsauth") {
            QString openvpn = openVPNPath.isEmpty() ? "/usr/sbin/openvpn" : openVPNPath;
            QProcess *taProcess = new QProcess(this);
            connect(taProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    [this, taProcess](int code, QProcess::ExitStatus) {
                        if (code == 0) {
                            emit logMessage("TLS-Auth ключ сгенерирован", "success");
                        } else {
                            emit logMessage("Ошибка генерации TLS-Auth ключа", "error");
                        }
                        taProcess->deleteLater();
                        currentCommandIndex++;
                        runNextCommand();
                    });
            taProcess->start(openvpn, {"--genkey", "--secret", certsDirectory + "/ta.key"});
            return;
        }
    }
}

void CertificateGenerator::runOpenSSLCommand(const QStringList &args, const QString &description)
{
    emit logMessage("Выполняется: " + description, "info");

    currentProcess->setProgram("openssl");
    currentProcess->setArguments(args);
    currentProcess->setWorkingDirectory(certsDirectory);
    currentProcess->start();
}

void CertificateGenerator::runEasyRSACommand(const QString &cmd, const QString &description)
{
    emit logMessage("Выполняется EasyRSA: " + description, "info");

    QString easyRSAPath = "/usr/share/easy-rsa/easyrsa";

    QStringList args;
    if (cmd == "init-pki") {
        args << "init-pki";
    } else if (cmd == "build-ca nopass") {
        args << "build-ca" << "nopass";
    } else if (cmd == "build-server-full server nopass") {
        args << "build-server-full" << "server" << "nopass";
    } else if (cmd == "gen-dh") {
        args << "gen-dh";
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("EASYRSA_BATCH", "1");
    env.insert("EASYRSA_REQ_CN", "TorManager");
    env.insert("EASYRSA_REQ_ORG", "TorManager");

    currentProcess->setProgram(easyRSAPath);
    currentProcess->setArguments(args);
    currentProcess->setProcessEnvironment(env);
    currentProcess->setWorkingDirectory(certsDirectory + "/easy-rsa");

    QDir().mkpath(certsDirectory + "/easy-rsa");
    currentProcess->start();
}

void CertificateGenerator::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        emit logMessage("Команда выполнена успешно", "success");
        currentCommandIndex++;
        runNextCommand();
    } else {
        emit logMessage(QString("Ошибка выполнения команды (код %1)").arg(exitCode), "error");
        emit finished(false);
    }
}

void CertificateGenerator::onProcessError(QProcess::ProcessError error)
{
    QString errorStr;
    switch (error) {
        case QProcess::FailedToStart:
            errorStr = "Не удалось запустить процесс";
            break;
        case QProcess::Crashed:
            errorStr = "Процесс аварийно завершился";
            break;
        case QProcess::Timedout:
            errorStr = "Таймаут процесса";
            break;
        default:
            errorStr = "Неизвестная ошибка";
    }

    emit logMessage("Ошибка: " + errorStr, "error");
    emit finished(false);
}

void CertificateGenerator::onProcessOutput()
{
    QString output = QString::fromUtf8(currentProcess->readAllStandardOutput());
    if (!output.isEmpty()) {
        emit logMessage(output.trimmed(), "info");
    }
}

void CertificateGenerator::onProcessErrorOutput()
{
    QString error = QString::fromUtf8(currentProcess->readAllStandardError());
    if (!error.isEmpty()) {
        if (!error.contains("warning") && !error.contains("deprecated")) {
            emit logMessage("stderr: " + error.trimmed(), "warning");
        }
    }
}

// ========== РЕАЛИЗАЦИЯ MAINWINDOW ==========

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, tabWidget(nullptr)
, torTab(nullptr)
, btnStartTor(nullptr)
, btnStopTor(nullptr)
, btnRestartTor(nullptr)
, btnNewCircuit(nullptr)
, lblTorStatus(nullptr)
, lblTorIP(nullptr)
, lblCircuitInfo(nullptr)
, txtTorLog(nullptr)
, cboBridgeType(nullptr)
, lstBridges(nullptr)
, btnAddBridge(nullptr)
, btnRemoveBridge(nullptr)
, lblTrafficStats(nullptr)
, btnImportBridges(nullptr)
, btnTestBridge(nullptr)
, lblBridgeStats(nullptr)
, serverTab(nullptr)
, serverGroup(nullptr)
, spinServerPort(nullptr)
, txtServerNetwork(nullptr)
, chkRouteThroughTor(nullptr)
, btnGenerateCerts(nullptr)
, btnCheckCerts(nullptr)
, btnStartServer(nullptr)
, btnStopServer(nullptr)
, lblServerStatus(nullptr)
, lblConnectedClients(nullptr)
, txtServerLog(nullptr)
, lblCurrentIP(nullptr)
, btnCheckIP(nullptr)
, btnGenerateClientConfig(nullptr)
, btnDiagnose(nullptr)
, btnTestConfig(nullptr)
, clientsTab(nullptr)
, clientsTable(nullptr)
, txtClientsLog(nullptr)
, btnDisconnectClient(nullptr)
, btnDisconnectAll(nullptr)
, btnRefreshClients(nullptr)
, btnClientDetails(nullptr)
, btnBanClient(nullptr)
, btnExportClientsLog(nullptr)
, btnClearClientsLog(nullptr)
, lblTotalClients(nullptr)
, lblActiveClients(nullptr)
, clientsRefreshTimer(nullptr)
, settingsTab(nullptr)
, spinTorSocksPort(nullptr)
, spinTorControlPort(nullptr)
, chkAutoStart(nullptr)
, chkKillSwitch(nullptr)
, chkBlockIPv6(nullptr)
, chkDNSLeakProtection(nullptr)
, chkStartMinimized(nullptr)
, txtTorPath(nullptr)
, txtOpenVPNPath(nullptr)
, btnApplySettings(nullptr)
, btnBrowseTor(nullptr)
, btnBrowseOpenVPN(nullptr)
, logsTab(nullptr)
, txtAllLogs(nullptr)
, cboLogLevel(nullptr)
, btnClearLogs(nullptr)
, btnSaveLogs(nullptr)
, trayMenu(nullptr)
, torProcess(nullptr)
, openVPNServerProcess(nullptr)
, certGenerator(nullptr)
, controlSocket(nullptr)
, ipCheckManager(nullptr)
, statusTimer(nullptr)
, trafficTimer(nullptr)
, clientStatsTimer(nullptr)
, realtimeTimer(nullptr)
, settings(nullptr)
, killSwitchEnabled(false)
, controlSocketConnected(false)
, serverStopPending(false)
, serverTorWaitRetries(0)
, currentConnectionState("disconnected")
, currentIP()
, torIP()
, bytesReceived(0)
, bytesSent(0)
, connectedClients(0)
, tempLinkPath()
, torrcPath()
, torDataDir()
, serverConfigPath()
, torExecutablePath()
, openVPNExecutablePath()
, certsDir()
, caCertPath()
, serverCertPath()
, serverKeyPath()
, dhParamPath()
, taKeyPath()
, configuredBridges()
, transportPluginPaths()
{
    // Задержка перед запуском бота для полной инициализации
    QTimer::singleShot(2000, this, [this]() {
        if (tgBotManager) {
            addLogMessage("🤖 Запуск Telegram бота...", "info");
            // Если есть метод start - вызовите его
            // tgBotManager->start();
            syncBotWithSettings();
        }
    });
    // Инициализация компонентов
    settings = new QSettings("TorManager", "TorVPN", this);
    torProcess = new QProcess(this);
    openVPNServerProcess = new QProcess(this);
    openVPNServerProcess->setProcessChannelMode(QProcess::MergedChannels);
    controlSocket = new QTcpSocket(this);
    ipCheckManager = new QNetworkAccessManager(this);

    statusTimer = new QTimer(this);
    trafficTimer = new QTimer(this);
    clientStatsTimer = new QTimer(this);
    clientsRefreshTimer = new QTimer(this);
    realtimeTimer = new QTimer(this);

    // FIX #1: logFlushTimer не был подключён — журналы не отображались
    logFlushTimer = new QTimer(this);
    logFlushTimer->setInterval(500);
    connect(logFlushTimer, &QTimer::timeout, this, &MainWindow::flushLogs);
    logFlushTimer->start();

    // Инициализация Telegram бота
    tgBotManager = new TelegramBotManager(settings, this);

    // Инициализация ClientStatsManager
    clientStats = new ClientStatsManager();

    // Инициализация MTProxy
    mtproxyManager = new MTProxyManager(this);
    mtproxyWidget = new MTProxyWidget(mtproxyManager, this);

    connect(mtproxyManager, &MTProxyManager::proxyStarted, this, &MainWindow::onMTProxyStarted);
    connect(mtproxyManager, &MTProxyManager::proxyStopped, this, &MainWindow::onMTProxyStopped);
    connect(mtproxyManager, &MTProxyManager::logMessage, this, &MainWindow::onMTProxyLog);

    // Поиск пути к mtproto-proxy (расширенный список)
    QStringList mtprotoPaths = {
        "/usr/bin/mtproto-proxy",
        "/usr/local/bin/mtproto-proxy",
        "/tmp/MTProxy/objs/bin/mtproto-proxy",
        "/tmp/MTProxy_build/objs/bin/mtproto-proxy",
        QDir::homePath() + "/MTProxy/objs/bin/mtproto-proxy",
        QDir::homePath() + "/Загрузки/MTProxy/objs/bin/mtproto-proxy",
        QDir::homePath() + "/Downloads/MTProxy/objs/bin/mtproto-proxy",
        QDir::homePath() + "/.local/bin/mtproto-proxy",
        "/opt/MTProxy/mtproto-proxy",
        "/opt/MTProxy/objs/bin/mtproto-proxy"
    };

    for (const QString &path : mtprotoPaths) {
        if (QFile::exists(path)) {
            mtproxyManager->setExecutablePath(path);
            addLogMessage("MTProto-Proxy найден: " + path, "success");
            break;
        }
    }

    // FIX #2: warn if mtproto-proxy not found
    if (mtproxyManager->getExecutablePath().isEmpty()) {
        addLogMessage("[MTProxy] mtproto-proxy binary not found. Use the Install button in the MTProxy tab.", "warning");
    }

    // Установка путей
    tgBotManager->setCertsDir(certsDir);
    tgBotManager->setServerConfPath(serverConfigPath); // для извлечения ta.key
    QString registryPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
    + "/client_registry.ini";
tgBotManager->setRegistryIni(registryPath);

// Подключение сигналов бота
connect(tgBotManager, &TelegramBotManager::logMessage,
        this, [this](const QString &msg, const QString &lvl){
            addLogMessage(msg, lvl);
        });
connect(tgBotManager, &TelegramBotManager::clientCreated,
        this, &MainWindow::onBotClientCreated);
connect(tgBotManager, &TelegramBotManager::clientRevoked,
        this, &MainWindow::onBotClientRevoked);
connect(tgBotManager, &TelegramBotManager::caGenerated,
        this, [this]() {
            addLogMessage("✅ CA сертификат создан через Telegram бот", "success");
            bool hasCa   = QFile::exists(caCertPath);
            bool hasCert = QFile::exists(serverCertPath);
            bool hasKey  = QFile::exists(serverKeyPath);
            addLogMessage(QString("CA: %1 | Cert: %2 | Key: %3")
            .arg(hasCa?"✓":"✗").arg(hasCert?"✓":"✗").arg(hasKey?"✓":"✗"), "info");
        });

//Подключение сигнала обновления реестра к боту
connect(this, &MainWindow::registryUpdated,
        tgBotManager, &TelegramBotManager::onRegistryUpdated,
        Qt::QueuedConnection);
// Настройка путей
QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
QDir().mkpath(appData);
torDataDir = appData + "/tor_data";
torrcPath = appData + "/torrc";
serverConfigPath = appData + "/server.conf";
QDir().mkpath(torDataDir);

// Пути для сертификатов
certsDir = appData + "/certs";
QDir().mkpath(certsDir);
caCertPath = certsDir + "/ca.crt";
serverCertPath = certsDir + "/server.crt";
serverKeyPath = certsDir + "/server.key";
dhParamPath = certsDir + "/dh.pem";

// ta.key может быть в нескольких местах — особенно при использовании easy-rsa
{
    QStringList taKeyOptions = {
        certsDir + "/ta.key",
        certsDir + "/easy-rsa/pki/ta.key",
        certsDir + "/easy-rsa/pki/private/ta.key"
    };
    taKeyPath = certsDir + "/ta.key"; // fallback
    for (const QString &p : taKeyOptions) {
        if (QFile::exists(p)) { taKeyPath = p; break; }
    }
}

// Создаём UI
setupUI();

// Загружаем настройки
loadSettings();

// Синхронизируем бота с основными настройками
syncBotWithSettings();

// Загружаем реестр клиентов
loadClientRegistry();  // Это вызовет emit registryUpdated()

// Проверка наличия Tor и OpenVPN
if (!checkTorInstalled()) {
    addLogMessage("Предупреждение: Tor не найден. Укажите путь в настройках.", "warning");
}

if (!checkOpenVPNInstalled()) {
    addLogMessage("Предупреждение: OpenVPN не найден. Укажите путь в настройках.", "warning");
}

setupTrayIcon();
setupConnections();
createTorConfig();
loadBridgesFromSettings();

// Запуск таймеров
statusTimer->start(5000);
trafficTimer->start(2000);
clientStatsTimer->start(CLIENT_STATS_UPDATE_INTERVAL);
clientsRefreshTimer->start(3000);
realtimeTimer->start(1000);

connect(realtimeTimer, &QTimer::timeout, this, &MainWindow::updateRealtimeDurations);

// Автозапуск если включен
if (settings->value("autoStart", false).toBool()) {
    QTimer::singleShot(1000, this, &MainWindow::startTor);
}

setWindowTitle("Tor Manager с OpenVPN (Сервер)");
resize(1000, 750);

addLogMessage("Tor Manager успешно инициализирован", "info");

// Инициализируем путь основного лога
allLogsFilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
+ "/tormanager.log";
}

MainWindow::~MainWindow()
{
    // Останавливаем мониторинг URL
    if (dnsMonitor) {
        dnsMonitor->stopMonitoring();
        delete dnsMonitor;
    }

    // Останавливаем все процессы принудительно
    if (torProcess && torProcess->state() == QProcess::Running) {
        addLogMessage("Принудительное завершение Tor...", "warning");
        torProcess->terminate();
        if (!torProcess->waitForFinished(3000)) {
            torProcess->kill();
            torProcess->waitForFinished(1000);
        }
    }

    if (openVPNServerProcess && openVPNServerProcess->state() == QProcess::Running) {
        addLogMessage("Принудительное завершение OpenVPN...", "warning");
        openVPNServerProcess->terminate();
        if (!openVPNServerProcess->waitForFinished(3000)) {
            openVPNServerProcess->kill();
            openVPNServerProcess->waitForFinished(1000);
        }
    }

    if (killSwitchEnabled) {
        disableKillSwitch();
    }

    // Очистка временных ссылок
    if (!tempLinkPath.isEmpty()) {
        QDir(tempLinkPath).removeRecursively();
    }

    // Очистка оберток скриптов
    QFile::remove("/tmp/tormgr_scripts/up_wrapper.sh");
    QFile::remove("/tmp/tormgr_scripts/down_wrapper.sh");
    QDir("/tmp/tormgr_scripts").rmdir("/tmp/tormgr_scripts");

    saveSettings();
    saveBridgesToSettings();
    saveClientRegistry();

    // Остановка и очистка MTProxy
    if (mtproxyManager) {
        mtproxyManager->stopProxy();
        delete mtproxyManager;
        mtproxyManager = nullptr;
    }

    // Очистка ClientStatsManager
    if (clientStats) {
        delete clientStats;
        clientStats = nullptr;
    }
}

// ========== НАСТРОЙКА ИНТЕРФЕЙСА ==========

void MainWindow::setupUI()
{
    createMenuBar();
    createTabWidget();
    setCentralWidget(tabWidget);
    statusBar()->showMessage("Готов");
}

void MainWindow::createMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);

    QMenu *fileMenu = menuBar->addMenu("&Файл");
    fileMenu->addAction("&Настройки", this, &MainWindow::showSettings);
    fileMenu->addSeparator();
    fileMenu->addAction("&Выход", this, &QWidget::close);

    QMenu *connMenu = menuBar->addMenu("&Подключение");
    connMenu->addAction("Запустить &Tor", this, &MainWindow::startTor);
    connMenu->addAction("Остановить T&or", this, &MainWindow::stopTor);
    connMenu->addAction("&Перезапустить Tor", this, &MainWindow::restartTor);
    connMenu->addSeparator();
    connMenu->addAction("Запустить VPN &сервер", this, &MainWindow::startOpenVPNServer);
    connMenu->addAction("Остановить VPN се&рвер", this, &MainWindow::stopOpenVPNServer);
    connMenu->addSeparator();
    connMenu->addAction("&Новая цепочка", this, &MainWindow::requestNewCircuit);

    QMenu *toolsMenu = menuBar->addMenu("&Инструменты");
    toolsMenu->addAction("Проверить &IP-адрес", this, &MainWindow::checkIPLeak);
    toolsMenu->addSeparator();
    toolsMenu->addAction("Включить Kill Switch", this, &MainWindow::enableKillSwitch);
    toolsMenu->addAction("Отключить Kill Switch", this, &MainWindow::disableKillSwitch);
    toolsMenu->addSeparator();
    toolsMenu->addAction("Сгенерировать сертификаты", this, &MainWindow::generateCertificates);
    toolsMenu->addAction("Проверить сертификаты", this, &MainWindow::checkCertificates);
    toolsMenu->addSeparator();
    toolsMenu->addAction("Диагностика сервера", this, &MainWindow::diagnoseConnection);
    toolsMenu->addAction("Проверить конфигурацию", this, &MainWindow::testServerConfig);
    toolsMenu->addAction("Проверить маршрутизацию", this, &MainWindow::verifyRouting);

    QMenu *helpMenu = menuBar->addMenu("&Помощь");
    helpMenu->addAction("&О программе", this, &MainWindow::showAbout);

    setMenuBar(menuBar);
}

void MainWindow::createTabWidget()
{
    tabWidget = new QTabWidget(this);

    createTorTab();
    createServerTab();    // создаёт пустой serverTab для совместимости
    createClientsTab();
    createUrlHistoryTab(); // НОВАЯ ВКЛАДКА
    createSettingsTab();
    createLogsTab();
    createMTProxyTab();

    // Структура: 6 вкладок
    tabWidget->addTab(torTab,      "🌐 Tor + VPN");
    tabWidget->addTab(clientsTab,  "👥 Клиенты");
    tabWidget->addTab(urlHistoryTab, "🌐 URL История"); // НОВАЯ
    tabWidget->addTab(settingsTab, "⚙️ Настройки");
    tabWidget->addTab(logsTab,     "📋 Журналы");
    tabWidget->addTab(mtproxyWidget, "🚀 MTProxy");

    // Telegram Bot — подвкладка в Настройках
    if (tgBotManager) {
        QTabWidget *st = settingsTab->findChild<QTabWidget*>();
        if (st) {
            QScrollArea *sa = new QScrollArea();
            sa->setWidgetResizable(true);
            sa->setFrameShape(QFrame::NoFrame);
            QWidget *pg = new QWidget();
            QVBoxLayout *vl = new QVBoxLayout(pg);
            sa->setWidget(pg);
            vl->addWidget(tgBotManager->buildSettingsSection(pg));
            vl->addStretch();
            st->addTab(sa, "🤖 Telegram Bot");
        }
    }
}

void MainWindow::createTorTab()
{
    torTab = new QWidget();
    QVBoxLayout *outerVl = new QVBoxLayout(torTab);
    outerVl->setContentsMargins(4, 4, 4, 4);
    outerVl->setSpacing(6);

    // ════════════════════ ВЕРХНЯЯ ПАНЕЛЬ: статусы ════════════════════
    QHBoxLayout *statusRow = new QHBoxLayout();
    statusRow->setSpacing(6);

    // -- Блок Tor --
    QGroupBox *torStatusGrp = new QGroupBox("🧅 Tor");
    torStatusGrp->setMinimumWidth(220);
    QVBoxLayout *tsVl = new QVBoxLayout(torStatusGrp);
    tsVl->setSpacing(2);
    lblTorStatus     = new QLabel("Статус: <b style='color:red;'>Отключен</b>");
    lblTorIP         = new QLabel("IP: Неизвестно");
    lblCircuitInfo   = new QLabel("Цепочка: Н/Д");
    lblTrafficStats  = new QLabel("Трафик: ↓ 0 Б  ↑ 0 Б");
    for (auto *l : {lblTorStatus, lblTorIP, lblCircuitInfo, lblTrafficStats}) {
        tsVl->addWidget(l);
    }

    // Кнопки Tor
    QHBoxLayout *torBtns = new QHBoxLayout();
    btnStartTor    = new QPushButton("▶ Tor");
    btnStopTor     = new QPushButton("■ Стоп");
    btnRestartTor  = new QPushButton("↺");
    btnNewCircuit  = new QPushButton("⟳ Цепь");
    btnRestartTor->setToolTip("Перезапустить Tor");
    btnStopTor->setEnabled(false);
    btnNewCircuit->setEnabled(false);
    for (auto *b : {btnStartTor, btnStopTor, btnRestartTor, btnNewCircuit}) {
        torBtns->addWidget(b);
    }
    tsVl->addLayout(torBtns);
    statusRow->addWidget(torStatusGrp, 1);

    // -- Блок VPN --
    QGroupBox *vpnStatusGrp = new QGroupBox("🔒 OpenVPN Сервер");
    QVBoxLayout *vsVl = new QVBoxLayout(vpnStatusGrp);
    vsVl->setSpacing(2);
    lblServerStatus     = new QLabel("Сервер: <b style='color:red;'>Остановлен</b>");
    lblConnectedClients = new QLabel("Подключений: 0");
    lblCurrentIP        = new QLabel("Внешний IP: Неизвестно");
    for (auto *l : {lblServerStatus, lblConnectedClients, lblCurrentIP}) {
        vsVl->addWidget(l);
    }

    QHBoxLayout *vpnBtns = new QHBoxLayout();
    btnGenerateCerts       = new QPushButton("🔑 Сертификаты"); btnGenerateCerts->hide();
    btnCheckCerts          = new QPushButton("✅ Проверить");   btnCheckCerts->hide();
    btnStartServer         = new QPushButton("▶ Запустить");
    btnStopServer          = new QPushButton("■ Остановить");
    btnCheckIP             = new QPushButton("🌐 IP");
    btnStopServer->setEnabled(false);
    btnGenerateClientConfig = new QPushButton(); btnGenerateClientConfig->hide();
    btnDiagnose   = new QPushButton(); btnDiagnose->hide();
    btnTestConfig = new QPushButton(); btnTestConfig->hide();
    for (auto *b : {btnStartServer, btnStopServer, btnCheckIP}) {
        vpnBtns->addWidget(b);
    }
    vsVl->addLayout(vpnBtns);
    statusRow->addWidget(vpnStatusGrp, 1);

    outerVl->addLayout(statusRow);

    // ── Панель управления Telegram ботом (логично здесь — рядом с VPN) ───
    if (tgBotManager) {
        outerVl->addWidget(tgBotManager->buildBotControlPanel(torTab));
    }

    // ════════════════════ МОСТЫ ════════════════════
    QGroupBox *bridgeGroup = new QGroupBox("🌉 Мосты Tor (обход блокировок)");
    bridgeGroup->setCheckable(true);
    bridgeGroup->setChecked(false);
    QVBoxLayout *bridgeLayout = new QVBoxLayout(bridgeGroup);

    QHBoxLayout *bridgeCtrl = new QHBoxLayout();
    cboBridgeType = new QComboBox();
    cboBridgeType->addItems({"Нет", "obfs4 (lyrebird)", "webtunnel", "snowflake", "Автоопределение"});
    btnAddBridge    = new QPushButton("＋ Добавить");
    btnRemoveBridge = new QPushButton("✕ Удалить");
    btnImportBridges= new QPushButton("📋 Импорт");
    btnTestBridge   = new QPushButton("🔍 Тест");
    bridgeCtrl->addWidget(new QLabel("Тип:"));
    bridgeCtrl->addWidget(cboBridgeType, 1);
    bridgeCtrl->addWidget(btnAddBridge);
    bridgeCtrl->addWidget(btnRemoveBridge);
    bridgeCtrl->addWidget(btnImportBridges);
    bridgeCtrl->addWidget(btnTestBridge);
    lblBridgeStats = new QLabel("Мосты: 0");
    lblBridgeStats->setStyleSheet("color:gray;font-size:10px;");
    lstBridges = new QListWidget();
    lstBridges->setMaximumHeight(90);
    lstBridges->setSelectionMode(QAbstractItemView::ExtendedSelection);
    lstBridges->setContextMenuPolicy(Qt::CustomContextMenu);
    QLabel *bridgeHint = new QLabel(
        "<span style='color:gray;font-size:9px;'>"
        "<b>obfs4:</b> obfs4 IP:PORT FINGERPRINT cert=... iat-mode=...<br>"
        "<b>webtunnel:</b> webtunnel [IP]:PORT FINGERPRINT url=https://domain.example/path ver=0.0.3<br>"
        "⚠️ webtunnel требует рабочего DNS и доступности домена из url=. "
        "Используйте кнопку <b>🔍 Тест</b> для диагностики моста."
        "</span>");
    bridgeHint->setWordWrap(true);
    bridgeLayout->addLayout(bridgeCtrl);
    bridgeLayout->addWidget(lblBridgeStats);
    bridgeLayout->addWidget(lstBridges);
    bridgeLayout->addWidget(bridgeHint);
    outerVl->addWidget(bridgeGroup);

    // ════════════════════ ЖУРНАЛЫ ════════════════════
    QHBoxLayout *logsRow = new QHBoxLayout();
    logsRow->setSpacing(6);

    QGroupBox *torLogGrp = new QGroupBox("Журнал Tor");
    QVBoxLayout *tlVl = new QVBoxLayout(torLogGrp);
    txtTorLog = new QTextEdit();
    txtTorLog->setReadOnly(true);
    txtTorLog->setFont(QFont("Monospace", 8));
    tlVl->addWidget(txtTorLog);
    logsRow->addWidget(torLogGrp, 1);

    QGroupBox *vpnLogGrp = new QGroupBox("Журнал VPN сервера");
    QVBoxLayout *vlVl = new QVBoxLayout(vpnLogGrp);
    txtServerLog = new QTextEdit();
    txtServerLog->setReadOnly(true);
    txtServerLog->setFont(QFont("Monospace", 8));
    vlVl->addWidget(txtServerLog);
    logsRow->addWidget(vpnLogGrp, 1);

    outerVl->addLayout(logsRow, 1);
}

void MainWindow::createServerTab()
{
    // Вкладка VPN Сервер теперь не используется — всё перенесено в Tor+VPN
    // Оставляем пустой виджет для совместимости
    serverTab = new QWidget();
}

void MainWindow::createClientsTab()
{
    clientsTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(clientsTab);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Подключаем сигнал обновления таблицы от бота
    if (tgBotManager) {
        connect(tgBotManager, &TelegramBotManager::clientsChanged,
                this, &MainWindow::updateRegistryTable);
    }

    // ── Статистика ────────────────────────────────────────────────────────
    QGroupBox *statsGroup = new QGroupBox("Статистика");
    QHBoxLayout *statsLayout = new QHBoxLayout(statsGroup);
    statsLayout->setContentsMargins(6, 4, 6, 4);

    lblTotalClients  = new QLabel("Зарегистрировано: <b>0</b>");
    lblActiveClients = new QLabel("Активных сейчас: <b>0</b>");
    lblOnlineClients = new QLabel("Онлайн в реестре: <b>0</b>");

    statsLayout->addWidget(lblTotalClients);
    statsLayout->addStretch();
    statsLayout->addWidget(lblActiveClients);
    statsLayout->addStretch();
    statsLayout->addWidget(lblOnlineClients);
    mainLayout->addWidget(statsGroup);

    // ── Поиск и фильтр ────────────────────────────────────────────────────
    QHBoxLayout *filterBar = new QHBoxLayout();
    regSearchEdit = new QLineEdit();
    regSearchEdit->setPlaceholderText("🔍 Поиск по имени клиента...");
    regSearchEdit->setClearButtonEnabled(true);

    regFilterCombo = new QComboBox();
    regFilterCombo->addItems({"Все клиенты", "🟢 Онлайн", "⚫ Офлайн", "🚫 Заблокированные"});
    regFilterCombo->setFixedWidth(180);

    filterBar->addWidget(regSearchEdit);
    filterBar->addWidget(regFilterCombo);
    mainLayout->addLayout(filterBar);

    // ── Объединённая таблица клиентов ─────────────────────────────────────
    registryTable = new QTableWidget(0, 11);
    registryTable->setHorizontalHeaderLabels({
        "Клиент", "VPN IP", "Статус",
        "↓ Скорость", "↑ Скорость",
        "Трафик ↓/↑", "Шифрование",
        "Истекает", "⚡ Лимит", "Сессий", "Последнее подкл."
    });
    registryTable->horizontalHeader()->setStretchLastSection(true);
    registryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    registryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    registryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    registryTable->setSelectionMode(QAbstractItemView::SingleSelection);
    registryTable->setAlternatingRowColors(true);
    registryTable->setSortingEnabled(true);
    registryTable->setContextMenuPolicy(Qt::CustomContextMenu);
    registryTable->verticalHeader()->setDefaultSectionSize(24);

    mainLayout->addWidget(registryTable, 3);

    // Также держим clientsTable как алиас
    clientsTable = new QTableWidget(0, 6);
    clientsTable->hide();

    // ── Кнопки управления ─────────────────────────────────────────────────
    QHBoxLayout *btnBar = new QHBoxLayout();

    btnRefreshClients = new QPushButton("🔄 Обновить");
    btnAddNamedClient = new QPushButton("➕ Добавить");
    btnAddNamedClient->setToolTip("Создать именной .ovpn с датой истечения");

    QPushButton *btnHistoryBtn = new QPushButton("📜 История");
    btnHistoryBtn->setEnabled(false);

    QPushButton *btnEditExpiry = new QPushButton("📅 Срок");
    btnEditExpiry->setEnabled(false);

    QPushButton *btnSpeedLimit = new QPushButton("⚡ Лимит скорости");
    btnSpeedLimit->setToolTip("Установить ограничение скорости для выбранного клиента");
    btnSpeedLimit->setEnabled(false);

    btnBanClient = new QPushButton("🚫 Блок/Разблок");
    btnBanClient->setEnabled(false);

    QPushButton *btnDeleteClient = new QPushButton("🗑 Удалить из реестра");
    btnDeleteClient->setEnabled(false);

    // НОВАЯ КНОПКА: Полное удаление клиента
    btnDeleteClientPermanent = new QPushButton("🗑 ПОЛНОСТЬЮ УДАЛИТЬ");
    btnDeleteClientPermanent->setToolTip("Удалить клиента из реестра, сертификаты и .ovpn файлы");
    btnDeleteClientPermanent->setEnabled(false);
    btnDeleteClientPermanent->setStyleSheet(
        "QPushButton { background: #c0392b; color: white; font-weight: bold; }"
        "QPushButton:hover { background: #e74c3c; }"
        "QPushButton:disabled { background: #7f8c8d; }"
    );

    btnDisconnectClient = new QPushButton("❌ Отключить");
    btnDisconnectClient->setEnabled(false);
    btnDisconnectAll = new QPushButton("⛔ Отключить всех");

    // Скрытые заглушки для совместимости
    btnClientDetails = new QPushButton();
    btnClientDetails->hide();
    btnClientAnalytics = new QPushButton();
    btnClientAnalytics->hide();
    btnClientHistory = new QPushButton();
    btnClientHistory->hide();

    // Добавляем кнопки в панель
    btnBar->addWidget(btnRefreshClients);
    btnBar->addWidget(btnAddNamedClient);
    btnBar->addWidget(btnHistoryBtn);
    btnBar->addWidget(btnEditExpiry);
    btnBar->addWidget(btnSpeedLimit);
    btnBar->addWidget(btnBanClient);
    btnBar->addWidget(btnDeleteClient);
    btnBar->addWidget(btnDeleteClientPermanent);  // НОВАЯ КНОПКА
    btnBar->addStretch();
    btnBar->addWidget(btnDisconnectClient);
    btnBar->addWidget(btnDisconnectAll);

    mainLayout->addLayout(btnBar);

    // ── Журнал подключений ────────────────────────────────────────────────
    QGroupBox *logGroup = new QGroupBox("Журнал подключений");
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    logLayout->setContentsMargins(4, 4, 4, 4);

    txtClientsLog = new QTextEdit();
    txtClientsLog->setReadOnly(true);
    txtClientsLog->setMaximumHeight(130);
    txtClientsLog->setFont(QFont("Monospace", 9));

    QHBoxLayout *logBtnLayout = new QHBoxLayout();
    btnExportClientsLog = new QPushButton("💾 Экспорт");
    btnClearClientsLog = new QPushButton("🗑️ Очистить");
    logBtnLayout->addStretch();
    logBtnLayout->addWidget(btnExportClientsLog);
    logBtnLayout->addWidget(btnClearClientsLog);

    logLayout->addWidget(txtClientsLog);
    logLayout->addLayout(logBtnLayout);
    mainLayout->addWidget(logGroup, 1);

    // ── Сигналы для таблицы реестра ───────────────────────────────────────
    connect(registryTable, &QTableWidget::itemSelectionChanged, this,
            [this, btnEditExpiry, btnDeleteClient, btnHistoryBtn, btnSpeedLimit]() {
                // btnDeleteClientPermanent уже доступен через this->
                bool has = !registryTable->selectedItems().isEmpty();
                if (has) {
                    int row = registryTable->selectedItems().first()->row();
                    QString cn = registryTable->item(row, 0) ? registryTable->item(row, 0)->data(Qt::UserRole).toString() : "";
                    bool isOnline = activeIPsPerClient.contains(cn);

                    btnDisconnectClient->setEnabled(isOnline);
                    btnBanClient->setEnabled(true);

                    bool isBanned = clientRegistry.value(cn).isBanned;
                    btnBanClient->setText(isBanned ? "✅ Разблокировать" : "🚫 Заблокировать");

                    int lim = clientRegistry.value(cn).speedLimitKbps;
                    btnSpeedLimit->setText(lim > 0 ? QString("⚡ %1 КБит/с").arg(lim) : "⚡ Лимит скорости");

                    // Новая кнопка всегда доступна при выборе
                    this->btnDeleteClientPermanent->setEnabled(true);  // ИСПРАВЛЕНО
                } else {
                    btnDisconnectClient->setEnabled(false);
                    btnBanClient->setEnabled(false);
                    btnBanClient->setText("🚫 Блок/Разблок");
                    btnSpeedLimit->setText("⚡ Лимит скорости");
                    this->btnDeleteClientPermanent->setEnabled(false);  // ИСПРАВЛЕНО
                }
                btnEditExpiry->setEnabled(has);
                btnDeleteClient->setEnabled(has);
                btnHistoryBtn->setEnabled(has);
                btnSpeedLimit->setEnabled(has);
            });

    // Диалог ограничения скорости
    connect(btnSpeedLimit, &QPushButton::clicked, this, [this]() {
        auto sel = registryTable->selectedItems();
        if (sel.isEmpty()) return;
        QString cn = registryTable->item(sel.first()->row(), 0)->data(Qt::UserRole).toString();
        if (!clientRegistry.contains(cn)) return;

        int current = clientRegistry[cn].speedLimitKbps;

        QDialog dlg(this);
        dlg.setWindowTitle("Ограничение скорости: " + cn);
        dlg.setMinimumWidth(340);
        QVBoxLayout *vl = new QVBoxLayout(&dlg);

        QLabel *hint = new QLabel(
            QString("<b>%1</b><br><br>"
            "Текущий лимит: <b>%2</b><br>"
            "Значение 0 = без ограничений")
            .arg(cn, current > 0 ? QString("%1 КБит/с").arg(current) : "Без ограничений"));
        hint->setWordWrap(true);
        vl->addWidget(hint);

        QFormLayout *fl = new QFormLayout();
        QSpinBox *spin = new QSpinBox();
        spin->setRange(0, 1000000);
        spin->setValue(current);
        spin->setSuffix(" КБит/с");
        spin->setSpecialValueText("Без ограничений");
        fl->addRow("Лимит скорости:", spin);

        // Быстрые пресеты
        QHBoxLayout *presets = new QHBoxLayout();
        presets->addWidget(new QLabel("Пресеты:"));
        for (int kbps : {0, 512, 1024, 2048, 5120, 10240}) {
            QPushButton *pb = new QPushButton(kbps == 0 ? "∞" : QString("%1К").arg(kbps));
            pb->setFixedWidth(52);
            connect(pb, &QPushButton::clicked, spin, [spin, kbps]() { spin->setValue(kbps); });
            presets->addWidget(pb);
        }
        vl->addLayout(fl);
        vl->addLayout(presets);

        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        vl->addWidget(bb);

        if (dlg.exec() == QDialog::Accepted) {
            applySpeedLimit(cn, spin->value());
        }
    });

    connect(registryTable, &QTableWidget::doubleClicked, this, &MainWindow::showClientSessionHistory);

    // НОВЫЙ СЛОТ: Полное удаление клиента
    connect(btnDeleteClientPermanent, &QPushButton::clicked, this, [this]() {
        auto sel = registryTable->selectedItems();
        if (sel.isEmpty()) return;

        QString cn = registryTable->item(sel.first()->row(), 0)->data(Qt::UserRole).toString();

        QMessageBox::StandardButton reply = QMessageBox::warning(
            this,
            "⚠️ ПОЛНОЕ УДАЛЕНИЕ КЛИЕНТА",
            QString("<b>ВНИМАНИЕ!</b><br><br>"
            "Вы действительно хотите ПОЛНОСТЬЮ УДАЛИТЬ клиента <b>'%1'</b>?<br><br>"
            "<b>Будут удалены:</b><br>"
            "• Запись из реестра<br>"
            "• Сертификаты клиента (.crt, .key)<br>"
            "• .ovpn конфигурационные файлы<br>"
            "• История сессий<br>"
            "• CCD файлы<br><br>"
            "<font color='red'>Это действие НЕОБРАТИМО!</font>")
            .arg(cn),
                                                                 QMessageBox::Yes | QMessageBox::No,
                                                                 QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            // Дополнительное подтверждение
            QMessageBox::StandardButton confirm = QMessageBox::question(
                this,
                "Последнее подтверждение",
                QString("Введите имя клиента для подтверждения: <b>%1</b>").arg(cn),
                                                                        QMessageBox::Yes | QMessageBox::No
            );

            if (confirm == QMessageBox::Yes) {
                deleteClientPermanently(cn);
            }
        }
    });

    // Контекстное меню
    connect(registryTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        auto sel = registryTable->selectedItems();
        if (sel.isEmpty()) return;
        int row = sel.first()->row();
        QTableWidgetItem *nameItem = registryTable->item(row, 0);
        if (!nameItem) return;
        QString cn = nameItem->data(Qt::UserRole).toString();
        bool isBanned = clientRegistry.value(cn).isBanned;

        QMenu menu(this);
        menu.addAction("📜 История сессий", this, &MainWindow::showClientSessionHistory);
        menu.addAction("📊 Аналитика", this, &MainWindow::showClientAnalytics);
        menu.addSeparator();
        if (isBanned) {
            menu.addAction("✅ Разблокировать", this, [this, cn]() {
                if (clientRegistry.contains(cn)) {
                    clientRegistry[cn].isBanned = false;
                    saveClientRegistry();
                    updateRegistryTable();
                }
            });
        } else {
            menu.addAction("🚫 Заблокировать", this, &MainWindow::banClient);
        }
        menu.addSeparator();
        menu.addAction("📅 Изменить срок", this, [this, cn]() {
            if (!clientRegistry.contains(cn)) return;
            bool ok;
            QString cur = clientRegistry[cn].expiryDate.isValid()
            ? clientRegistry[cn].expiryDate.toString("dd.MM.yyyy") : "";
            QString ds = QInputDialog::getText(this, "Изменить срок",
                                               "Введите дату (дд.мм.гггг) или пусто — без ограничений:",
                                               QLineEdit::Normal, cur, &ok);
            if (!ok) return;
            clientRegistry[cn].expiryDate = ds.trimmed().isEmpty()
            ? QDate() : QDate::fromString(ds.trimmed(), "dd.MM.yyyy");
            saveClientRegistry();
            updateRegistryTable();
        });
        menu.addSeparator();
        menu.addAction("📋 Копировать имя", this, [cn]() {
            QGuiApplication::clipboard()->setText(cn);
        });

        // НОВЫЙ ПУНКТ: Полное удаление
        menu.addSeparator();
        QAction *deleteAction = menu.addAction("🗑 ПОЛНОСТЬЮ УДАЛИТЬ", this, [this, cn]() {
            QMessageBox::StandardButton reply = QMessageBox::warning(
                this,
                "⚠️ Полное удаление",
                QString("Удалить клиента <b>%1</b> полностью?").arg(cn),
                                                                     QMessageBox::Yes | QMessageBox::No
            );
            if (reply == QMessageBox::Yes) {
                deleteClientPermanently(cn);
            }
        });
        deleteAction->setIcon(QIcon::fromTheme("edit-delete"));

        menu.exec(registryTable->mapToGlobal(pos));
    });

    // Поиск/фильтр
    connect(regSearchEdit, &QLineEdit::textChanged, this, &MainWindow::applyRegistryFilter);
    connect(regFilterCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::applyRegistryFilter);

    connect(btnHistoryBtn, &QPushButton::clicked, this, &MainWindow::showClientSessionHistory);

    connect(btnEditExpiry, &QPushButton::clicked, this, [this]() {
        auto sel = registryTable->selectedItems();
        if (sel.isEmpty()) return;
        QString cn = registryTable->item(sel.first()->row(), 0)->data(Qt::UserRole).toString();
        if (!clientRegistry.contains(cn)) return;

        QDialog dlg(this);
        dlg.setWindowTitle("Срок действия: " + cn);
        QVBoxLayout *dl = new QVBoxLayout(&dlg);
        dl->addWidget(new QLabel("Выберите новую дату истечения:"));

        QCalendarWidget *cal = new QCalendarWidget(&dlg);
        cal->setMinimumDate(QDate::currentDate());
        QDate cur = clientRegistry[cn].expiryDate;
        cal->setSelectedDate(cur.isValid() ? cur : QDate::currentDate().addMonths(1));
        dl->addWidget(cal);

        QCheckBox *chk = new QCheckBox("Без ограничений");
        chk->setChecked(!cur.isValid());
        dl->addWidget(chk);

        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        dl->addWidget(bb);

        if (dlg.exec() == QDialog::Accepted) {
            clientRegistry[cn].expiryDate = chk->isChecked() ? QDate() : cal->selectedDate();
            saveClientRegistry();
            updateRegistryTable();
        }
    });

    connect(btnDeleteClient, &QPushButton::clicked, this, [this]() {
        auto sel = registryTable->selectedItems();
        if (sel.isEmpty()) return;
        QString cn = registryTable->item(sel.first()->row(), 0)->data(Qt::UserRole).toString();
        if (QMessageBox::question(this, "Удаление из реестра",
            QString("Удалить '%1' из реестра?\nСертификат останется.").arg(cn),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            clientRegistry.remove(cn);
        saveClientRegistry();
        updateRegistryTable();
                                  }
    });

    // Подключение остальных кнопок
    connect(btnRefreshClients, &QPushButton::clicked, this, &MainWindow::refreshClientsNow);
    connect(btnDisconnectClient, &QPushButton::clicked, this, &MainWindow::disconnectSelectedClient);
    connect(btnDisconnectAll, &QPushButton::clicked, this, &MainWindow::disconnectAllClients);
    connect(btnBanClient, &QPushButton::clicked, this, &MainWindow::banClient);
    connect(btnExportClientsLog, &QPushButton::clicked, this, &MainWindow::exportClientsLog);
    connect(btnClearClientsLog, &QPushButton::clicked, this, &MainWindow::clearClientsLog);
    connect(btnAddNamedClient, &QPushButton::clicked, this, &MainWindow::generateNamedClientConfig);

    // Инициализация DNS монитора
    dnsMonitor = new DnsMonitor(this);
    connect(dnsMonitor, &DnsMonitor::urlAccessed,
            this, &MainWindow::addUrlAccess,
            Qt::QueuedConnection);  // QueuedConnection — гарантирует выполнение в главном потоке
    connect(dnsMonitor, &DnsMonitor::logMessage,
            this, &MainWindow::addLogMessage);

    // Запускаем мониторинг с задержкой после старта сервера —
    // tun0 поднимается не мгновенно, нужно подождать ~3 сек.
    // Если tun0 ещё не появился — повторяем ещё через 3 сек.
    connect(this, &MainWindow::serverStarted, [this]() {
        if (!dnsMonitor) return;
        QString network = txtServerNetwork ? txtServerNetwork->text() : "";
        // Первая попытка через 3 сек (tun0 обычно поднимается за 1-2 сек)
        QTimer::singleShot(3000, this, [this, network]() {
            if (!dnsMonitor) return;
            dnsMonitor->startMonitoring(network);
            // Вторая попытка через ещё 4 сек — на случай медленного старта
            QTimer::singleShot(4000, this, [this, network]() {
                if (!dnsMonitor) return;
                dnsMonitor->startMonitoring(network);
            });
        });
    });

    connect(this, &MainWindow::clientsUpdated, [this]() {
        if (dnsMonitor) {
            QMap<QString, QString> ipMap;
            for (auto it = clientsCache.begin(); it != clientsCache.end(); ++it) {
                ipMap[it.value().virtualAddress] = it.value().commonName;
            }
            dnsMonitor->setClientMap(ipMap);
        }
    });

}

void MainWindow::createUrlHistoryTab()
{
    urlHistoryTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(urlHistoryTab);
    layout->setContentsMargins(4, 4, 4, 4);

    // ── Панель фильтров ──────────────────────────────────────────────────
    QGroupBox *filterGroup = new QGroupBox("Фильтры");
    QHBoxLayout *filterLayout = new QHBoxLayout(filterGroup);

    filterLayout->addWidget(new QLabel("Клиент:"));
    urlClientFilter = new QComboBox();
    urlClientFilter->addItem("Все");
    urlClientFilter->setMinimumWidth(150);
    filterLayout->addWidget(urlClientFilter);

    filterLayout->addWidget(new QLabel("Метод:"));
    urlMethodFilter = new QComboBox();
    urlMethodFilter->addItems({"Все", "DNS", "HTTP", "HTTPS"});
    urlMethodFilter->setMinimumWidth(100);
    filterLayout->addWidget(urlMethodFilter);

    filterLayout->addStretch();

    QPushButton *btnClearUrls = new QPushButton("🗑 Очистить историю");
    btnClearUrls->setStyleSheet("QPushButton { background: #e74c3c; color: white; }");
    connect(btnClearUrls, &QPushButton::clicked, [this]() {
        if (QMessageBox::question(this, "Очистка истории",
            "Очистить всю историю URL-запросов?",
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            urlHistory.clear();
        updateUrlTable();
        addLogMessage("История URL очищена", "info");
            }
    });
    filterLayout->addWidget(btnClearUrls);

    layout->addWidget(filterGroup);

    // ── Таблица URL ──────────────────────────────────────────────────────
    urlTable = new QTableWidget();
    urlTable->setColumnCount(5);
    urlTable->setHorizontalHeaderLabels({"Время", "Клиент", "VPN IP", "Метод", "URL/Домен"});
    urlTable->horizontalHeader()->setStretchLastSection(true);
    urlTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    urlTable->setAlternatingRowColors(true);
    urlTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    urlTable->setSortingEnabled(true);

    layout->addWidget(urlTable);

    // ── Статистика ───────────────────────────────────────────────────────
    QHBoxLayout *statsLayout = new QHBoxLayout();
    urlStatsLabel = new QLabel("Всего записей: 0");
    urlStatsLabel->setStyleSheet("color: gray; font-size: 10px;");

    QPushButton *btnExportUrls = new QPushButton("💾 Экспорт в CSV");
    connect(btnExportUrls, &QPushButton::clicked, [this]() {
        QString filename = QFileDialog::getSaveFileName(this,
                                                        "Сохранить историю URL",
                                                        QDir::homePath() + "/url_history_" +
                                                        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".csv",
                                                        "CSV Files (*.csv)");
        if (filename.isEmpty()) return;

        QFile file(filename);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            out << "Время;Клиент;VPN IP;Метод;URL\n";

            for (const UrlAccess &acc : urlHistory) {
                out << acc.timestamp.toString("dd.MM.yyyy HH:mm:ss") << ";"
                << acc.clientName << ";"
                << acc.clientIP << ";"
                << acc.method << ";"
                << acc.url << "\n";
            }
            file.close();
            addLogMessage("История URL экспортирована: " + filename, "success");
        }
    });

    statsLayout->addWidget(urlStatsLabel);
    statsLayout->addStretch();
    statsLayout->addWidget(btnExportUrls);

    layout->addLayout(statsLayout);

    // Подключаем фильтры
    connect(urlClientFilter, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::updateUrlTable);
    connect(urlMethodFilter, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::updateUrlTable);
}

// НОВЫЙ МЕТОД: Добавление URL доступа

void MainWindow::addUrlAccess(const UrlAccess &access)
{
    // Этот метод вызывается через Qt::QueuedConnection — всегда в главном потоке
    urlHistory.prepend(access);  // новые сверху

    // Ограничиваем размер истории (макс 10000 записей)
    while (urlHistory.size() > 10000) {
        urlHistory.removeLast();
    }

    // Обновляем фильтр клиентов
    if (urlClientFilter && urlClientFilter->findText(access.clientName) == -1) {
        urlClientFilter->addItem(access.clientName);
    }

    // Обновляем таблицу только если вкладка видима (оптимизация)
    updateUrlTable();

    // Логируем в журнал клиентов (только если виджет существует)
    if (txtClientsLog) {
        QString timestamp = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
        txtClientsLog->append(QString(
            "<span style='color:#3498db;'>[%1] 🌐 <b>%2</b> → %3 [%4]</span>")
        .arg(timestamp)
        .arg(access.clientName.toHtmlEscaped())
        .arg(access.url.toHtmlEscaped())
        .arg(access.method));
    }
}

// НОВЫЙ МЕТОД: Обновление таблицы URL

void MainWindow::updateUrlTable()
{
    if (!urlTable || !urlClientFilter || !urlMethodFilter) return;

    QString filterClient = urlClientFilter->currentText();
    QString filterMethod = urlMethodFilter->currentText();

    // Фильтруем записи
    QList<UrlAccess> filtered;
    for (const UrlAccess &acc : urlHistory) {
        if (filterClient != "Все" && acc.clientName != filterClient) continue;
        if (filterMethod != "Все" && acc.method != filterMethod) continue;
        filtered.append(acc);
    }

    // Блокируем сигналы при обновлении
    urlTable->blockSignals(true);
    urlTable->setSortingEnabled(false);
    urlTable->setRowCount(filtered.size());

    int row = 0;
    for (const UrlAccess &acc : filtered) {
        // Время
        QTableWidgetItem *timeItem = new QTableWidgetItem(
            acc.timestamp.toString("dd.MM.yyyy HH:mm:ss"));
        timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
        urlTable->setItem(row, 0, timeItem);

        // Клиент
        QTableWidgetItem *clientItem = new QTableWidgetItem(acc.clientName);
        clientItem->setFlags(clientItem->flags() & ~Qt::ItemIsEditable);
        urlTable->setItem(row, 1, clientItem);

        // VPN IP
        QTableWidgetItem *ipItem = new QTableWidgetItem(acc.clientIP);
        ipItem->setFlags(ipItem->flags() & ~Qt::ItemIsEditable);
        urlTable->setItem(row, 2, ipItem);

        // Метод
        QTableWidgetItem *methodItem = new QTableWidgetItem(acc.method);
        methodItem->setFlags(methodItem->flags() & ~Qt::ItemIsEditable);

        // Цвет метода
        if (acc.method == "DNS") {
            methodItem->setForeground(QColor(52, 152, 219));  // синий
        } else if (acc.method == "HTTP") {
            methodItem->setForeground(QColor(46, 204, 113));  // зеленый
        } else if (acc.method == "HTTPS") {
            methodItem->setForeground(QColor(155, 89, 182));  // фиолетовый
        }

        urlTable->setItem(row, 3, methodItem);

        // URL
        QTableWidgetItem *urlItem = new QTableWidgetItem(acc.url);
        urlItem->setFlags(urlItem->flags() & ~Qt::ItemIsEditable);
        urlItem->setToolTip(acc.url);  // полный URL в подсказке
        urlTable->setItem(row, 4, urlItem);

        row++;
    }

    urlTable->setSortingEnabled(true);
    urlTable->blockSignals(false);

    // Обновляем статистику
    urlStatsLabel->setText(QString("Всего записей: %1 (отображено: %2)")
    .arg(urlHistory.size())
    .arg(filtered.size()));
}

//Полное удаление клиента
void MainWindow::deleteClientPermanently(const QString &cn)
{
    QStringList filesToDelete;
    int deleted = 0;
    int failed = 0;

    // Сертификаты клиента
    QStringList certExts = {".crt", ".key", ".csr", ".p12"};
    for (const QString &ext : certExts) {
        QString path = certsDir + "/" + cn + ext;
        if (QFile::exists(path)) {
            filesToDelete << path;
        }
    }

    // 2. Конфигурации .ovpn
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    filesToDelete << appData + "/ovpn/" + cn + ".ovpn";
    filesToDelete << QDir::homePath() + "/" + cn + ".ovpn";

    // 3. CCD файл
    filesToDelete << appData + "/ccd/" + cn;

    // 4. История сессий (JSON файл)
    QString safeKey = QString(cn).replace('/', '_').replace('\\', '_').replace(' ', '_');
    filesToDelete << appData + "/sessions/" + safeKey + ".json";

    // 5. Временные файлы бота
    if (tgBotManager) {
        filesToDelete << QDir::tempPath() + "/tormanager_ovpn/" + cn + ".ovpn";
    }

    // Удаляем файлы
    for (const QString &path : filesToDelete) {
        if (QFile::exists(path)) {
            if (QFile::remove(path)) {
                deleted++;
                addLogMessage(QString("  ✓ Удалён: %1").arg(path), "info");
            } else {
                failed++;
                addLogMessage(QString("  ✗ Ошибка удаления: %1").arg(path), "error");
            }
        }
    }

    // 6. Удаляем из реестра (под мьютексом)
    {
        QMutexLocker locker(&registryMutex);
        if (clientRegistry.contains(cn)) {
            clientRegistry.remove(cn);
            addLogMessage("  ✓ Клиент удалён из реестра", "info");
        }
    }
    saveClientRegistry();

    // 7. Удаляем из кэша активных клиентов
    QStringList toRemove;
    for (auto it = clientsCache.begin(); it != clientsCache.end(); ++it) {
        if (it.value().commonName == cn) {
            toRemove.append(it.key());
        }
    }
    for (const QString &key : toRemove) {
        clientsCache.remove(key);
        addLogMessage(QString("  ✓ Удалён из кэша активных: %1").arg(key), "info");
    }

    // 8. Удаляем из activeIPsPerClient
    activeIPsPerClient.remove(cn);

    // 9. Удаляем из URL истории
    int urlRemoved = 0;
    for (int i = urlHistory.size() - 1; i >= 0; --i) {
        if (urlHistory[i].clientName == cn) {
            urlHistory.removeAt(i);
            urlRemoved++;
        }
    }
    if (urlRemoved > 0) {
        addLogMessage(QString("  ✓ Удалено записей из URL истории: %1").arg(urlRemoved), "info");
        updateUrlTable();
    }

    // Обновляем таблицы
    updateRegistryTable();
    updateClientsTable();

    // Итоговое сообщение
    QString resultMsg = QString("✅ Клиент '%1' полностью удалён.\n"
    "Удалено файлов: %2\n"
    "Ошибок удаления: %3")
    .arg(cn).arg(deleted).arg(failed);

    addLogMessage(resultMsg, "success");

    QMessageBox::information(this, "Удаление завершено", resultMsg);

    // Если клиент был онлайн - отключаем его
    if (!toRemove.isEmpty()) {
        addLogMessage("⚠️ Клиент был онлайн, его сессия будет завершена", "warning");
    }
}

void MainWindow::createSettingsTab()
{
    settingsTab = new QWidget();
    QVBoxLayout *outerLayout = new QVBoxLayout(settingsTab);
    outerLayout->setContentsMargins(4, 4, 4, 4);

    QTabWidget *settingsTabs = new QTabWidget();

    // ══════════ TAB 1: Tor ══════════
    {
        QScrollArea *sa = new QScrollArea(); sa->setWidgetResizable(true); sa->setFrameShape(QFrame::NoFrame);
        QWidget *pg = new QWidget(); QVBoxLayout *vl = new QVBoxLayout(pg); sa->setWidget(pg);

        QGroupBox *pathGrp = new QGroupBox("Исполняемый файл");
        QFormLayout *fl = new QFormLayout(pathGrp);
        txtTorPath = new QLineEdit();
        btnBrowseTor = new QPushButton("📂"); btnBrowseTor->setFixedWidth(32);
        QHBoxLayout *torRow = new QHBoxLayout(); torRow->addWidget(txtTorPath); torRow->addWidget(btnBrowseTor);
        fl->addRow("Путь к Tor:", torRow);
        vl->addWidget(pathGrp);

        QGroupBox *portGrp = new QGroupBox("Порты");
        QFormLayout *pfl = new QFormLayout(portGrp);
        spinTorSocksPort = new QSpinBox(); spinTorSocksPort->setRange(1024,65535); spinTorSocksPort->setValue(DEFAULT_TOR_SOCKS_PORT);
        spinTorControlPort = new QSpinBox(); spinTorControlPort->setRange(1024,65535); spinTorControlPort->setValue(DEFAULT_TOR_CONTROL_PORT);
        pfl->addRow("SOCKS-порт:", spinTorSocksPort);
        pfl->addRow("Управляющий порт:", spinTorControlPort);
        vl->addWidget(portGrp);

        // ── Внешний доступ ────────────────────────────────────────────────
        QGroupBox *extGrp = new QGroupBox("🌐 Внешний доступ (для клиентов в сети)");
        extGrp->setToolTip("Позволяет другим устройствам в сети использовать Tor через этот сервер");
        QVBoxLayout *evl = new QVBoxLayout(extGrp);

        chkExternalSocks = new QCheckBox("Разрешить внешний доступ к SOCKS-порту (0.0.0.0)");
        chkExternalSocks->setToolTip(
            "Если включено — SOCKS порт будет слушать на всех интерфейсах (0.0.0.0).\n"
            "Клиенты смогут использовать Tor как SOCKS5-прокси.\n"
            "⚠️ Убедитесь что порт защищён фаерволом!");
        evl->addWidget(chkExternalSocks);

        chkExternalTrans = new QCheckBox("Включить прозрачный прокси (TransPort)");
        chkExternalTrans->setToolTip(
            "TransPort перехватывает TCP-трафик прозрачно — клиенту не нужно настраивать прокси.\n"
            "Используется совместно с iptables для маршрутизации трафика VPN-клиентов через Tor.");
        evl->addWidget(chkExternalTrans);

        QHBoxLayout *transRow = new QHBoxLayout();
        transRow->addSpacing(20);
        transRow->addWidget(new QLabel("TransPort:"));
        spinTransPort = new QSpinBox(); spinTransPort->setRange(1024,65535); spinTransPort->setValue(9040);
        transRow->addWidget(spinTransPort);
        transRow->addStretch();
        evl->addLayout(transRow);

        chkExternalDns = new QCheckBox("Включить DNS через Tor (DNSPort)");
        chkExternalDns->setToolTip(
            "DNSPort разрешает DNS-запросы через Tor.\n"
            "Нужен для маршрутизации DNS VPN-клиентов через Tor (защита от DNS-утечек).");
        evl->addWidget(chkExternalDns);

        QHBoxLayout *dnsRow = new QHBoxLayout();
        dnsRow->addSpacing(20);
        dnsRow->addWidget(new QLabel("DNSPort:"));
        spinDnsPort = new QSpinBox(); spinDnsPort->setRange(1024,65535); spinDnsPort->setValue(5353);
        dnsRow->addWidget(spinDnsPort);
        dnsRow->addStretch();
        evl->addLayout(dnsRow);

        auto *extHint = new QLabel(
            "⚠️ <b>Внимание:</b> Внешние порты доступны для всех в сети.\n"
            "Откройте только нужные порты в фаерволе (ufw/iptables).\n"
            "Настройки применяются при следующем запуске Tor."
        );
        extHint->setWordWrap(true);
        extHint->setStyleSheet("color: #e67e22; font-size: 10px;");
        evl->addWidget(extHint);

        // Связываем чекбоксы с зависимыми спинбоксами
        connect(chkExternalTrans, &QCheckBox::toggled, spinTransPort, &QSpinBox::setEnabled);
        connect(chkExternalDns,   &QCheckBox::toggled, spinDnsPort,   &QSpinBox::setEnabled);
        spinTransPort->setEnabled(false);
        spinDnsPort->setEnabled(false);

        vl->addWidget(extGrp);
        vl->addStretch();

        connect(btnBrowseTor, &QPushButton::clicked, this, [this]() {
            QString p = QFileDialog::getOpenFileName(this, "Tor", "/usr/bin", "Все файлы (*)");
            if (!p.isEmpty()) txtTorPath->setText(p);
        });
            settingsTabs->addTab(sa, "🧅 Tor");
    }

    // ══════════ TAB 2: VPN ══════════
    {
        QScrollArea *sa = new QScrollArea(); sa->setWidgetResizable(true); sa->setFrameShape(QFrame::NoFrame);
        QWidget *pg = new QWidget(); QVBoxLayout *vl = new QVBoxLayout(pg); sa->setWidget(pg);

        // Исполняемый файл OpenVPN
        QGroupBox *vpnGrp = new QGroupBox("Исполняемый файл");
        QFormLayout *fl = new QFormLayout(vpnGrp);
        txtOpenVPNPath = new QLineEdit();
        btnBrowseOpenVPN = new QPushButton("📂"); btnBrowseOpenVPN->setFixedWidth(32);
        QHBoxLayout *vpnRow = new QHBoxLayout(); vpnRow->addWidget(txtOpenVPNPath); vpnRow->addWidget(btnBrowseOpenVPN);
        fl->addRow("Путь к OpenVPN:", vpnRow);
        txtServerAddress = new QLineEdit();
        txtServerAddress->setPlaceholderText("Внешний IP или домен (для .ovpn клиентов)");
        QPushButton *btnDetectIP = new QPushButton("🔍 Определить");
        QHBoxLayout *addrRow = new QHBoxLayout(); addrRow->addWidget(txtServerAddress); addrRow->addWidget(btnDetectIP);
        fl->addRow("Адрес сервера:", addrRow);
        vl->addWidget(vpnGrp);

        // Параметры сервера
        QGroupBox *paramGrp = new QGroupBox("Параметры сервера");
        QFormLayout *pfl = new QFormLayout(paramGrp);
        spinServerPort = new QSpinBox(); spinServerPort->setRange(1024,65535); spinServerPort->setValue(DEFAULT_VPN_SERVER_PORT);
        cboServerProto = new QComboBox(); cboServerProto->addItems({"TCP","UDP"});
        cboServerProto->setToolTip("UDP быстрее, TCP надёжнее через файрволы");
        txtServerNetwork = new QLineEdit("10.8.0.0 255.255.255.0");
        spinMaxClients = new QSpinBox(); spinMaxClients->setRange(1,1000); spinMaxClients->setValue(10);
        spinMtu = new QSpinBox(); spinMtu->setRange(576,9000); spinMtu->setValue(1500); spinMtu->setSuffix(" байт");
        chkRouteThroughTor = new QCheckBox("Маршрутизировать трафик через Tor");
        chkRouteThroughTor->setChecked(true);
        chkDuplicateCN = new QCheckBox("Разрешить несколько подключений с одним CN");
        chkDuplicateCN->setChecked(true);
        chkClientToClient = new QCheckBox("Трафик между клиентами (client-to-client)");
        chkClientToClient->setChecked(false);
        pfl->addRow("Порт:", spinServerPort);
        pfl->addRow("Протокол:", cboServerProto);
        pfl->addRow("Сеть клиентов:", txtServerNetwork);
        pfl->addRow("Макс. клиентов:", spinMaxClients);
        pfl->addRow("MTU:", spinMtu);
        pfl->addRow("", chkRouteThroughTor);
        pfl->addRow("", chkDuplicateCN);
        pfl->addRow("", chkClientToClient);
        vl->addWidget(paramGrp);

        // Сертификаты
        QGroupBox *certGrp = new QGroupBox("🔑 Сертификаты");
        QVBoxLayout *cvl = new QVBoxLayout(certGrp);
        QLabel *certHint = new QLabel("Необходимо сгенерировать один раз перед первым запуском сервера.");
        certHint->setStyleSheet("color:#666;font-size:10px;"); certHint->setWordWrap(true);
        cvl->addWidget(certHint);
        QHBoxLayout *certBtnRow = new QHBoxLayout();
        certBtnRow->addWidget(btnGenerateCerts); certBtnRow->addWidget(btnCheckCerts); certBtnRow->addStretch();
        btnGenerateCerts->show(); btnCheckCerts->show();
        cvl->addLayout(certBtnRow);
        vl->addWidget(certGrp);
        vl->addStretch();

        // Скрытые виджеты Security Settings — создаём но не показываем
        // (нужны для generateServerConfig)
        cboDataCipher  = new QComboBox(); cboDataCipher->addItems({"AES-256-GCM","AES-128-GCM","CHACHA20-POLY1305","AES-256-CBC","AES-128-CBC"}); cboDataCipher->hide();
        cboTlsCipher   = new QComboBox(); cboTlsCipher->addItems({"По умолчанию","TLS-ECDHE-RSA-WITH-AES-256-GCM-SHA384"}); cboTlsCipher->hide();
        cboHmacAuth    = new QComboBox(); cboHmacAuth->addItems({"SHA256","SHA384","SHA512","SHA1"}); cboHmacAuth->hide();
        cboTlsMinVer   = new QComboBox(); cboTlsMinVer->addItems({"1.2","1.3"}); cboTlsMinVer->hide();
        cboTlsMode     = new QComboBox(); cboTlsMode->addItems({"tls-auth","tls-crypt","Отключено"}); cboTlsMode->setCurrentIndex(1); cboTlsMode->hide();
        chkPfs         = new QCheckBox(); chkPfs->setChecked(true); chkPfs->hide();
        chkCompression = new QCheckBox(); chkCompression->setChecked(false); chkCompression->hide();

        connect(btnBrowseOpenVPN, &QPushButton::clicked, this, [this]() {
            QString p = QFileDialog::getOpenFileName(this, "OpenVPN", "/usr/sbin", "Все файлы (*)");
            if (!p.isEmpty()) txtOpenVPNPath->setText(p);
        });
            connect(btnDetectIP, &QPushButton::clicked, this, [this]() {
                addLogMessage("Определение внешнего IP...", "info");
                QProcess *curl = new QProcess(this);
                connect(curl, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
                        this, [this,curl](int code, QProcess::ExitStatus) {
                            QString ip = QString::fromUtf8(curl->readAllStandardOutput()).trimmed();
                            curl->deleteLater();
                            if (code==0 && !ip.isEmpty()) { txtServerAddress->setText(ip); addLogMessage("IP: "+ip,"success"); }
                            else addLogMessage("Не удалось определить IP","warning");
                        });
                            curl->start("curl", {"-s","--max-time","5","ifconfig.me"});
            });
            settingsTabs->addTab(sa, "🔒 VPN");
    }

    // ══════════ TAB 3: Безопасность ══════════
    {
        QScrollArea *sa = new QScrollArea(); sa->setWidgetResizable(true); sa->setFrameShape(QFrame::NoFrame);
        QWidget *pg = new QWidget(); QVBoxLayout *vl = new QVBoxLayout(pg); sa->setWidget(pg);

        QGroupBox *secGrp = new QGroupBox("🛡️ Защита трафика");
        QVBoxLayout *svl = new QVBoxLayout(secGrp);
        chkKillSwitch        = new QCheckBox("Kill Switch — блокировать трафик вне Tor/VPN");
        chkBlockIPv6         = new QCheckBox("Блокировать IPv6");
        chkDNSLeakProtection = new QCheckBox("Защита от утечек DNS");
        svl->addWidget(chkKillSwitch); svl->addWidget(chkBlockIPv6); svl->addWidget(chkDNSLeakProtection);
        vl->addWidget(secGrp);
        vl->addStretch();
        settingsTabs->addTab(sa, "🛡️ Безопасность");
    }

    // ══════════ TAB 4: Общие ══════════
    {
        QScrollArea *sa = new QScrollArea(); sa->setWidgetResizable(true); sa->setFrameShape(QFrame::NoFrame);
        QWidget *pg = new QWidget(); QVBoxLayout *vl = new QVBoxLayout(pg); sa->setWidget(pg);

        QGroupBox *genGrp = new QGroupBox("Поведение");
        QVBoxLayout *gvl = new QVBoxLayout(genGrp);
        chkAutoStart              = new QCheckBox("Автоматически запускать Tor при старте");
        chkStartMinimized         = new QCheckBox("Запускать свёрнутым в трей");
        chkShowTrayNotifications  = new QCheckBox("Уведомления в трее");
        gvl->addWidget(chkAutoStart); gvl->addWidget(chkStartMinimized); gvl->addWidget(chkShowTrayNotifications);
        QFormLayout *ifl = new QFormLayout();
        spinRefreshInterval = new QSpinBox(); spinRefreshInterval->setRange(1,60); spinRefreshInterval->setValue(3); spinRefreshInterval->setSuffix(" сек");
        ifl->addRow("Интервал обновления:", spinRefreshInterval);
        gvl->addLayout(ifl);
        vl->addWidget(genGrp);

        QGroupBox *logGrp = new QGroupBox("Логирование");
        QFormLayout *lfl = new QFormLayout(logGrp);
        cboLogLevelSetting = new QComboBox();
        cboLogLevelSetting->addItems({"Все сообщения","INFO и выше","WARNING и выше","Только ОШИБКИ"});
        lfl->addRow("Уровень:", cboLogLevelSetting);
        chkWriteAllLogs = new QCheckBox("Записывать в файл");
        lfl->addRow("", chkWriteAllLogs);
        QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/tormanager.log";
        QLabel *lblLogPath = new QLabel(logPath); lblLogPath->setStyleSheet("color:gray;font-size:10px;"); lblLogPath->setWordWrap(true);
        lfl->addRow("Файл:", lblLogPath);
        QHBoxLayout *logBtnRow = new QHBoxLayout();
        QPushButton *btnOpenLogDir = new QPushButton("📂 Открыть");
        QPushButton *btnRotateLogs = new QPushButton("🔄 Ротировать");
        logBtnRow->addWidget(btnOpenLogDir); logBtnRow->addWidget(btnRotateLogs); logBtnRow->addStretch();
        lfl->addRow("", logBtnRow);
        vl->addWidget(logGrp);
        vl->addStretch();

        connect(btnOpenLogDir, &QPushButton::clicked, this, [logPath]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(logPath).absolutePath()));
        });
        connect(btnRotateLogs, &QPushButton::clicked, this, [this,logPath]() {
            QString arch = logPath+"."+QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
            if (QFile::rename(logPath,arch)) addLogMessage("Лог ротирован → "+arch,"info");
            else addLogMessage("Не удалось ротировать лог","warning");
        });
            settingsTabs->addTab(sa, "⚙️ Общие");
    }

    outerLayout->addWidget(settingsTabs);
    btnApplySettings = new QPushButton("✅ Применить настройки");
    btnApplySettings->setMinimumHeight(36);
    outerLayout->addWidget(btnApplySettings);

    connect(btnApplySettings, &QPushButton::clicked, this, &MainWindow::applySettings);
}

void MainWindow::createLogsTab()
{
    logsTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(logsTab);

    // ── Панель управления ─────────────────────────────────────────────────
    QHBoxLayout *controlLayout = new QHBoxLayout();

    cboLogLevel = new QComboBox();
    cboLogLevel->addItems({"Все", "INFO", "ПРЕДУПР", "ОШИБКА", "УСПЕХ"});
    cboLogLevel->setToolTip("Фильтр отображения записей журнала");
    cboLogLevel->setFixedWidth(130);

    QLineEdit *logSearchEdit = new QLineEdit();
    logSearchEdit->setPlaceholderText("🔍 Поиск в журнале...");
    logSearchEdit->setClearButtonEnabled(true);

    btnClearLogs = new QPushButton("🗑 Очистить");
    btnSaveLogs  = new QPushButton("💾 Сохранить");
    QPushButton *btnOpenErrLog = new QPushButton("⛔ Открыть errors.log");
    btnOpenErrLog->setToolTip("Открыть файл только с ошибками");

    controlLayout->addWidget(new QLabel("Фильтр:"));
    controlLayout->addWidget(cboLogLevel);
    controlLayout->addWidget(logSearchEdit, 1);
    controlLayout->addWidget(btnClearLogs);
    controlLayout->addWidget(btnSaveLogs);
    controlLayout->addWidget(btnOpenErrLog);

    mainLayout->addLayout(controlLayout);

    txtAllLogs = new QTextEdit();
    txtAllLogs->setReadOnly(true);
    txtAllLogs->setFont(QFont("Monospace", 9));
    mainLayout->addWidget(txtAllLogs);

    // ── Строка быстрой статистики ─────────────────────────────────────────
    QLabel *lblLogStats = new QLabel("Строк в журнале: 0");
    lblLogStats->setStyleSheet("color: gray; font-size: 10px;");
    mainLayout->addWidget(lblLogStats);

    // ── Сигналы ────────────────────────────────────────────────────────────
    // Фильтр по уровню и поиску — скрываем/показываем строки через setVisible
    auto applyLogFilter = [this, logSearchEdit]() {
        QString filterType  = cboLogLevel->currentText();   // "Все" / "INFO" / ...
        QString filterText  = logSearchEdit->text().trimmed().toLower();
        // Простой подход: скрытие строк в QTextEdit невозможно напрямую.
        // Вместо этого подсвечиваем — полноценная фильтрация требует отдельного виджета.
        // Реализуем подсветку найденного текста.
        if (filterText.isEmpty() && filterType == "Все") {
            // Убрать подсветку
            QTextCharFormat fmt;
            fmt.setBackground(Qt::transparent);
            QTextCursor c(txtAllLogs->document());
            c.select(QTextCursor::Document);
            c.setCharFormat(fmt);
            return;
        }
        // Подсвечиваем строки, содержащие искомый текст
        if (!filterText.isEmpty()) {
            QTextCharFormat hl;
            hl.setBackground(QColor(255, 255, 150));
            QTextDocument *doc = txtAllLogs->document();
            QTextCursor cursor(doc);
            while (!cursor.isNull() && !cursor.atEnd()) {
                cursor = doc->find(filterText, cursor, QTextDocument::FindCaseSensitively);
                if (!cursor.isNull()) cursor.mergeCharFormat(hl);
            }
        }
    };
    connect(cboLogLevel,  qOverload<int>(&QComboBox::currentIndexChanged), this, applyLogFilter);
    connect(logSearchEdit, &QLineEdit::textChanged, this, applyLogFilter);

    // Обновляем счётчик строк
    connect(txtAllLogs, &QTextEdit::textChanged, this, [this, lblLogStats]() {
        int lines = txtAllLogs->document()->lineCount();
        lblLogStats->setText(QString("Строк в журнале: %1 / %2").arg(lines).arg(MAX_LOG_LINES));
        if (lines > MAX_LOG_LINES * 0.9)
            lblLogStats->setStyleSheet("color: orange; font-size: 10px;");
        else
            lblLogStats->setStyleSheet("color: gray; font-size: 10px;");
    });

    connect(btnClearLogs, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(this, "Очистка журнала",
            "Очистить журнал в приложении?\n(Файл на диске не удаляется)",
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            txtAllLogs->clear();
        addLogMessage("Журнал очищен", "info");
                                  }
    });

    connect(btnSaveLogs, &QPushButton::clicked, this, [this]() {
        QString filename = QFileDialog::getSaveFileName(this, "Сохранить журнал",
                                                        QDir::homePath() + "/tormanager_log_" +
                                                        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".txt",
                                                        "Текстовые файлы (*.txt)");
        if (filename.isEmpty()) return;
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << txtAllLogs->toPlainText();
            addLogMessage("Журнал сохранён: " + filename, "success");
        } else {
            addLogMessage("Ошибка сохранения журнала: " + filename, "error");
        }
    });

    connect(btnOpenErrLog, &QPushButton::clicked, this, [this]() {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/tormanager_errors.log";
            if (QFile::exists(path)) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            } else {
                QMessageBox::information(this, "Файл не найден",
                                         "Файл errors.log не существует.\nОшибки ещё не возникали.");
            }
    });

    // ── Секция журнала Telegram бота ───────────────────────────────────────
    if (tgBotManager) {
        mainLayout->addWidget(tgBotManager->buildLogSection(logsTab));
    }
}

void MainWindow::createMTProxyTab()
{
    if (mtproxyWidget) {
        // Вкладка добавляется в createTabWidget(), здесь только лог
        addLogMessage("Вкладка MTProxy добавлена", "info");
    }
}

void MainWindow::setupTrayIcon()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon::fromTheme("network-vpn"));

    trayMenu = new QMenu(this);
    trayMenu->addAction("Показать/Скрыть", this, [this]() {
        setVisible(!isVisible());
    });
    trayMenu->addSeparator();
    trayMenu->addAction("Запустить Tor", this, &MainWindow::startTor);
    trayMenu->addAction("Остановить Tor", this, &MainWindow::stopTor);
    trayMenu->addSeparator();
    trayMenu->addAction("Запустить VPN сервер", this, &MainWindow::startOpenVPNServer);
    trayMenu->addAction("Остановить VPN сервер", this, &MainWindow::stopOpenVPNServer);
    trayMenu->addSeparator();
    trayMenu->addAction("Выход", this, &QWidget::close);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
}

void MainWindow::setupConnections()
{
    // Процесс Tor
    connect(torProcess, &QProcess::started, this, &MainWindow::onTorStarted);
    connect(torProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onTorFinished);
    connect(torProcess, &QProcess::errorOccurred, this, &MainWindow::onTorError);
    connect(torProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onTorReadyRead);
    connect(torProcess, &QProcess::readyReadStandardError, this, [this]() {
        QString error = QString::fromUtf8(torProcess->readAllStandardError());
        if (!error.isEmpty()) {
            addLogMessage("Tor stderr: " + error.trimmed(), "error");
        }
    });

    // Процесс сервера
    connect(openVPNServerProcess, &QProcess::started, this, &MainWindow::onServerStarted);
    connect(openVPNServerProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onServerFinished);
    connect(openVPNServerProcess, &QProcess::errorOccurred, this, &MainWindow::onServerError);
    connect(openVPNServerProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onServerReadyRead);

    // Управляющий сокет
    connect(controlSocket, &QTcpSocket::connected, this, &MainWindow::onControlSocketConnected);
    connect(controlSocket, &QTcpSocket::readyRead, this, &MainWindow::onControlSocketReadyRead);
    connect(controlSocket, &QTcpSocket::errorOccurred, this, &MainWindow::onControlSocketError);
    connect(controlSocket, &QTcpSocket::disconnected, this, [this]() {
        addLogMessage("Управляющий сокет отключен", "warning");
        controlSocketConnected = false;
        torCircuitSubscribed = false;
        controlSocketBuffer.clear();
    });

    // Кнопки Tor
    connect(btnStartTor, &QPushButton::clicked, this, &MainWindow::startTor);
    connect(btnStopTor, &QPushButton::clicked, this, &MainWindow::stopTor);
    connect(btnRestartTor, &QPushButton::clicked, this, &MainWindow::restartTor);
    connect(btnNewCircuit, &QPushButton::clicked, this, &MainWindow::requestNewCircuit);
    connect(btnAddBridge, &QPushButton::clicked, this, &MainWindow::addBridge);
    connect(btnRemoveBridge, &QPushButton::clicked, this, &MainWindow::removeBridge);
    connect(btnImportBridges, &QPushButton::clicked, this, &MainWindow::importBridgesFromText);
    connect(btnTestBridge, &QPushButton::clicked, this, [this]() {
        auto item = lstBridges->currentItem();
        if (item) {
            testBridgeConnection(item->text());
        }
    });
    connect(lstBridges, &QListWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu contextMenu;
        contextMenu.addAction("Проверить формат", this, &MainWindow::validateBridgeFormat);
        contextMenu.addAction("Проверить подключение", this, [this]() {
            auto item = lstBridges->currentItem();
            if (item) testBridgeConnection(item->text());
        });
            contextMenu.addSeparator();
            contextMenu.addAction("Копировать", [this]() {
                auto item = lstBridges->currentItem();
                if (item) QGuiApplication::clipboard()->setText(item->text());
            });
                contextMenu.exec(lstBridges->mapToGlobal(pos));
    });

    // Кнопки сервера
    connect(btnStartServer, &QPushButton::clicked, this, &MainWindow::startOpenVPNServer);
    connect(btnStopServer, &QPushButton::clicked, this, &MainWindow::stopOpenVPNServer);
    connect(btnGenerateCerts, &QPushButton::clicked, this, &MainWindow::generateCertificates);
    connect(btnCheckCerts, &QPushButton::clicked, this, &MainWindow::checkCertificates);
    connect(btnCheckIP, &QPushButton::clicked, this, &MainWindow::checkIPLeak);
    connect(btnGenerateClientConfig, &QPushButton::clicked, this, &MainWindow::generateClientConfig);

    // Подключаем новые кнопки диагностики
    if (btnDiagnose) {
        connect(btnDiagnose, &QPushButton::clicked, this, &MainWindow::diagnoseConnection);
    }

    if (btnTestConfig) {
        connect(btnTestConfig, &QPushButton::clicked, this, &MainWindow::testServerConfig);
    }

    // ========== НОВЫЕ ПОДКЛЮЧЕНИЯ ДЛЯ КЛИЕНТОВ ==========
    connect(clientsRefreshTimer, &QTimer::timeout, this, &MainWindow::updateClientsTable);
    connect(btnRefreshClients, &QPushButton::clicked, this, &MainWindow::refreshClientsNow);
    connect(btnDisconnectClient, &QPushButton::clicked, this, &MainWindow::disconnectSelectedClient);
    connect(btnDisconnectAll, &QPushButton::clicked, this, &MainWindow::disconnectAllClients);
    connect(btnClientDetails, &QPushButton::clicked, this, &MainWindow::showClientDetails);
    connect(btnClientAnalytics, &QPushButton::clicked, this, &MainWindow::showClientAnalytics);
    connect(btnClientHistory, &QPushButton::clicked, this, &MainWindow::showClientSessionHistory);
    connect(btnBanClient, &QPushButton::clicked, this, &MainWindow::banClient);
    connect(btnExportClientsLog, &QPushButton::clicked, this, &MainWindow::exportClientsLog);
    connect(btnClearClientsLog, &QPushButton::clicked, this, &MainWindow::clearClientsLog);
    connect(clientsTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::onClientTableContextMenu);
    connect(clientsTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        bool hasSelection = !clientsTable->selectedItems().isEmpty();
        btnDisconnectClient->setEnabled(hasSelection);
        btnClientDetails->setEnabled(hasSelection);
        btnClientAnalytics->setEnabled(hasSelection);
        btnClientHistory->setEnabled(hasSelection);
    });

    // Кнопки настроек и журналов подключены в createSettingsTab() и createLogsTab()
    // (убрано дублирование - сигналы уже подключены при создании виджетов)

    // Таймеры
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::updateStatus);
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::checkTorStatus);
    connect(trafficTimer, &QTimer::timeout, this, &MainWindow::updateTrafficStats);
    connect(clientStatsTimer, &QTimer::timeout, this, &MainWindow::updateClientStats);
    connect(ipCheckManager, &QNetworkAccessManager::finished, this, &MainWindow::onIPCheckFinished);
}

// ========== УПРАВЛЕНИЕ TOR ==========

void MainWindow::startTor()
{
    if (torRunning) {
        addLogMessage("Tor уже запущен", "warning");
        return;
    }

    if (!checkTorInstalled()) {
        QMessageBox::critical(this, "Ошибка", "Tor не найден. Укажите путь в настройках.");
        return;
    }

    // Дополнительная проверка: файл существует и является исполняемым
    QFileInfo torBin(torExecutablePath);
    if (!torBin.exists() || !torBin.isExecutable()) {
        addLogMessage("Ошибка: файл Tor не найден или не исполняемый: " + torExecutablePath, "error");
        QMessageBox::critical(this, "Ошибка", "Файл Tor не найден или нет прав на запуск:\n" + torExecutablePath);
        return;
    }

    // Проверка torrc пути — не должен содержать инъекций
    if (torrcPath.contains(QRegularExpression("[;&|`$<>]"))) {
        addLogMessage("Ошибка: недопустимые символы в пути конфига: " + torrcPath, "error");
        return;
    }

    torCrashCount = 0;  // Сброс счётчика крэшей при ручном запуске
    addLogMessage("Запуск Tor: " + torExecutablePath, "info");
    createTorConfig();

    // Запуск строго через аргументы, без shell
    QStringList args;
    args << "-f" << torrcPath;

    torProcess->start(torExecutablePath, args);

    if (!torProcess->waitForStarted(5000)) {
        QString errStr = torProcess->errorString();
        addLogMessage("Не удалось запустить процесс Tor: " + errStr, "error");
        QMessageBox::critical(this, "Ошибка запуска", "Не удалось запустить Tor:\n" + errStr);
        btnStartTor->setEnabled(true);
        btnStopTor->setEnabled(false);
        return;
    }

    btnStartTor->setEnabled(false);
    btnStopTor->setEnabled(true);
}

void MainWindow::stopTor()
{
    if (!torRunning && torProcess->state() == QProcess::NotRunning) {
        addLogMessage("Tor не запущен", "warning");
        return;
    }

    addLogMessage("Остановка Tor...", "info");

    if (controlSocketConnected) {
        sendTorCommand("SIGNAL SHUTDOWN");
    }

    // Сначала terminate() (SIGTERM), через 5 сек — kill() (SIGKILL)
    if (torProcess->state() == QProcess::Running) {
        torProcess->terminate();
        QTimer::singleShot(5000, this, [this]() {
            if (torProcess->state() == QProcess::Running) {
                addLogMessage("Tor не завершился по SIGTERM, отправляем SIGKILL", "warning");
                torProcess->kill();
            }
        });
    }

    torRunning = false;
    controlSocketConnected = false;

    btnStartTor->setEnabled(true);
    btnStopTor->setEnabled(false);
    btnNewCircuit->setEnabled(false);

    lblTorStatus->setText("Статус: <b style='color:red;'>Отключен</b>");
    if (lblCircuitInfo) lblCircuitInfo->setText("Цепочка: Н/Д");
    setConnectionState("disconnected");
}

void MainWindow::restartTor()
{
    addLogMessage("Перезапуск Tor...", "info");
    stopTor();
    QTimer::singleShot(2000, this, &MainWindow::startTor);
}

void MainWindow::onTorStarted()
{
    addLogMessage("Процесс Tor запущен", "info");
    torRunning = true;
    lblTorStatus->setText("Статус: <b style='color:orange;'>Подключение...</b>");

    QTimer::singleShot(3000, this, [this]() {
        controlSocket->connectToHost("127.0.0.1", spinTorControlPort->value());
    });
}

void MainWindow::onTorFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    torRunning = false;
    controlSocketConnected = false;

    // Логируем stderr если есть
    QString stderrOut = QString::fromUtf8(torProcess->readAllStandardError()).trimmed();
    if (!stderrOut.isEmpty())
        addLogMessage("Tor stderr при завершении: " + stderrOut, "error");

    QString message = QString("Процесс Tor завершён (код %1, статус: %2)")
    .arg(exitCode)
    .arg(exitStatus == QProcess::NormalExit ? "нормально" : "аварийно");
    addLogMessage(message, exitStatus == QProcess::NormalExit && exitCode == 0 ? "info" : "error");

    lblTorStatus->setText("Статус: <b style='color:red;'>Отключен</b>");
    btnStartTor->setEnabled(true);
    btnStopTor->setEnabled(false);
    btnNewCircuit->setEnabled(false);

    setConnectionState("disconnected");

    // Автоперезапуск только при аварийном завершении, не более 3 раз подряд
    // BUG FIX: был static int crashCount — не сбрасывался при ручном старте.
    // Перенесён в член класса torCrashCount (объявлен в .h).
    if (exitStatus == QProcess::CrashExit) {
        torCrashCount++;
        if (torCrashCount <= 3) {
            addLogMessage(QString("Tor аварийно завершился (попытка %1/3), перезапуск через 5 сек...").arg(torCrashCount), "error");
            QTimer::singleShot(5000, this, &MainWindow::startTor);
        } else {
            addLogMessage("Tor аварийно завершился 3 раза подряд. Автоперезапуск отключён.", "error");
            QMessageBox::critical(this, "Критическая ошибка Tor",
                                  "Tor аварийно завершался 3 раза подряд.\nПроверьте логи и конфигурацию.");
            torCrashCount = 0;
        }
    } else {
        torCrashCount = 0;  // Нормальное завершение — сбрасываем счётчик
    }
}

void MainWindow::onTorError(QProcess::ProcessError error)
{
    QString errorMsg;
    QString hint;
    switch (error) {
        case QProcess::FailedToStart:
            errorMsg = "Не удалось запустить Tor";
            hint = QString("Путь: %1\nПроверьте путь в Настройках и наличие пакета 'tor'.")
            .arg(torExecutablePath.isEmpty() ? "(не задан)" : torExecutablePath);
            break;
        case QProcess::Crashed:
            errorMsg = "Процесс Tor аварийно завершился";
            hint = "Проверьте tormanager_errors.log для деталей.";
            break;
        case QProcess::Timedout:
            errorMsg = "Таймаут запуска Tor";
            hint = "Сервер не ответил вовремя.";
            break;
        default:
            errorMsg = "Неизвестная ошибка процесса Tor: " + QString::number(error);
            hint = QString();
    }

    torRunning = false;
    if (lblTorStatus)
        lblTorStatus->setText("Статус: <b style='color:red;'>Ошибка</b>");
    if (btnStartTor)  btnStartTor->setEnabled(true);
    if (btnStopTor)   btnStopTor->setEnabled(false);

    addLogMessage(errorMsg + (hint.isEmpty() ? "" : " — " + hint), "error");
    QMessageBox::critical(this, "Ошибка Tor",
                          errorMsg + (hint.isEmpty() ? "" : "\n\n" + hint));
}

void MainWindow::onTorReadyRead()
{
    QString output = QString::fromUtf8(torProcess->readAllStandardOutput());
    txtTorLog->append(output);
    addLogMessage("Tor: " + output.trimmed(), "info");

    if (output.contains("Bootstrapped 100%")) {
        addLogMessage("Подключение Tor установлено!", "info");
        lblTorStatus->setText("Статус: <b style='color:green;'>Подключен</b>");
        btnNewCircuit->setEnabled(true);
        setConnectionState("tor_only");
        QTimer::singleShot(2000, this, &MainWindow::requestExternalIP);
        // Запускаем мониторинг сразу как Tor поднялся (для ControlPort ADDRMAP)
        if (dnsMonitor) {
            QTimer::singleShot(1500, this, [this]() {
                dnsMonitor->startMonitoring("");
            });
        }
    }

    // Детектирование нерабочих мостов
    if (output.contains("unrecognized reply") || output.contains("Error dialing")) {
        static int bridgeFailCount = 0;
        ++bridgeFailCount;
        if (bridgeFailCount == 3) {
            bridgeFailCount = 0;
            addLogMessage(
                "⚠️ Мосты не работают (unrecognized reply). "
                "Получите свежие мосты на https://bridges.torproject.org "
                "и обновите список во вкладке «Управление Tor».", "warning");
            if (lblBridgeStats)
                lblBridgeStats->setStyleSheet("color: red; font-size: 10px; font-weight: bold;");
        }
    }

    // Сброс счётчика при успешном подключении через мост
    if (output.contains("Bootstrapped") && output.contains("%")) {
        if (lblBridgeStats && !configuredBridges.isEmpty())
            lblBridgeStats->setStyleSheet("color: green; font-size: 10px;");
    }
}

void MainWindow::onControlSocketConnected()
{
    addLogMessage("Подключено к управляющему порту Tor", "info");
    controlSocketConnected = true;
    torCircuitSubscribed   = false;
    controlSocketBuffer.clear();

    // Читаем cookie-файл (CookieAuthentication 1 в torrc)
    QString cookiePath = torDataDir + "/control_auth_cookie";
    QFile cookieFile(cookiePath);
    if (cookieFile.open(QIODevice::ReadOnly)) {
        QByteArray cookie = cookieFile.readAll();
        cookieFile.close();
        sendTorCommand("AUTHENTICATE " + cookie.toHex());
        addLogMessage("Аутентификация Tor: cookie (" + QString::number(cookie.size()) + " байт)", "info");
    } else {
        // Fallback если cookie недоступен
        addLogMessage("Cookie-файл не найден: " + cookiePath + ", пробуем без пароля", "warning");
        sendTorCommand("AUTHENTICATE \"\"");
    }
}

void MainWindow::onControlSocketReadyRead()
{
    controlSocketBuffer += QString::fromUtf8(controlSocket->readAll());

    // Разбиваем на строки, оставляем незавершённую в буфере
    while (true) {
        int nl = controlSocketBuffer.indexOf('\n');
        if (nl < 0) break;
        QString line = controlSocketBuffer.left(nl).trimmed();
        controlSocketBuffer.remove(0, nl + 1);
        if (line.isEmpty()) continue;

        // ── Трафик ────────────────────────────────────────────────────────
        if (line.contains("traffic/read=")) {
            QRegularExpression re("traffic/read=(\\d+)");
            auto m = re.match(line);
            if (m.hasMatch()) bytesReceived = m.captured(1).toULongLong();
            continue;
        }
        if (line.contains("traffic/written=")) {
            QRegularExpression re("traffic/written=(\\d+)");
            auto m = re.match(line);
            if (m.hasMatch()) bytesSent = m.captured(1).toULongLong();
            continue;
        }

        // ── После AUTHENTICATE: 250 OK ────────────────────────────────────
        // Важно: только первый 250 OK (до подписки) запускает инициализацию
        if (line == "250 OK" && !torCircuitSubscribed) {
            torCircuitSubscribed = true;
            // 1. Подписываемся на события цепочки и потоков
            sendTorCommand("SETEVENTS CIRC STREAM");
            // 2. Запрашиваем статус установки цепочки
            sendTorCommand("GETINFO status/circuit-established");
            // 3. Запрашиваем список уже существующих цепочек
            sendTorCommand("GETINFO circuit-status");
            continue;
        }

        // ── status/circuit-established ────────────────────────────────────
        // Tor возвращает: "250-status/circuit-established=1" (с дефисом)
        if (line.contains("circuit-established=")) {
            bool ok = line.endsWith("=1");
            if (lblCircuitInfo) {
                if (ok)
                    lblCircuitInfo->setText(
                        "Цепочка: <b style='color:green;'>Установлена</b>");
                    else
                        lblCircuitInfo->setText(
                            "Цепочка: <b style='color:orange;'>Строится...</b>");
            }
            continue;
        }

        // ── circuit-status многострочный ответ ────────────────────────────
        // Формат каждой строки: "<id> BUILT <fp>~<alias>,... PURPOSE=GENERAL ..."
        // Нас интересуют строки со статусом BUILT
        if (line.contains(" BUILT ") && !line.startsWith("650")) {
            parseTorCircuitLine(line);
            continue;
        }

        // ── Асинхронное событие 650 CIRC ─────────────────────────────────
        // "650 CIRC <id> BUILT <fp>~<alias>,... BUILD_FLAGS=... PURPOSE=..."
        // "650 CIRC <id> EXTENDED <fp>~<alias>,..."
        // "650 CIRC <id> LAUNCHED"
        if (line.startsWith("650 CIRC ")) {
            if (line.contains(" BUILT ")) {
                parseTorCircuitLine(line);
            } else if (line.contains(" CLOSED ") || line.contains(" FAILED ")) {
                // Если была показана эта цепочка — запрашиваем обновление
                if (lblCircuitInfo && lblCircuitInfo->text().contains("→"))
                    sendTorCommand("GETINFO circuit-status");
            }
            continue;
        }

        // ── Ошибка аутентификации ─────────────────────────────────────────
        if (line.startsWith("515 ") || line.startsWith("551 ")) {
            addLogMessage("Tor control: ошибка аутентификации — " + line, "error");
            if (lblCircuitInfo)
                lblCircuitInfo->setText(
                    "Цепочка: <b style='color:red;'>Ошибка аутентификации</b>");
        }
    }
}

void MainWindow::parseTorCircuitLine(const QString &line)
{
    // Ищем первую BUILT цепочку и извлекаем узлы
    // Строка может быть:
    //   "3 BUILT $FP~Guard,$FP~Middle,$FP~Exit PURPOSE=GENERAL ..."
    //   "650 CIRC 3 BUILT $FP~Guard,... PURPOSE=GENERAL ..."
    int builtIdx = line.indexOf(" BUILT ");
    if (builtIdx < 0) return;
    QString afterBuilt = line.mid(builtIdx + 7).trimmed();
    // Первое слово — путь, остальное — атрибуты
    QString path = afterBuilt.split(' ').first();
    QStringList nodes;
    for (const QString &hop : path.split(',')) {
        QString h = hop.trimmed();
        if (h.isEmpty()) continue;
        // "$FINGERPRINT~Alias" или просто "$FINGERPRINT"
        QString alias = h.contains('~') ? h.split('~').last() : h.left(8).mid(1); // убираем $
        if (!alias.isEmpty()) nodes << alias;
    }
    if (nodes.isEmpty()) return;
    if (lblCircuitInfo)
        lblCircuitInfo->setText("Цепочка: " + nodes.join(" → "));
}

void MainWindow::onControlSocketError()
{
    addLogMessage("Ошибка управляющего сокета: " + controlSocket->errorString(), "error");
    controlSocketConnected = false;
}

void MainWindow::sendTorCommand(const QString &command)
{
    if (!controlSocketConnected) {
        addLogMessage("Нет подключения к управляющему порту", "warning");
        return;
    }

    QString cmd = command + "\r\n";
    controlSocket->write(cmd.toUtf8());
    controlSocket->flush();
}

void MainWindow::requestNewCircuit()
{
    if (!controlSocketConnected) {
        addLogMessage("Не могу запросить новую цепочку: нет подключения к управляющему порту", "warning");
        return;
    }

    addLogMessage("Запрос новой цепочки Tor...", "info");
    sendTorCommand("SIGNAL NEWNYM");

    QMessageBox::information(this, "Новая цепочка",
                             "Запрошена новая цепочка Tor. "
                             "Подождите 10 секунд перед новыми подключениями.");
}

void MainWindow::checkTorStatus()
{
    if (!torRunning || !controlSocketConnected) return;
    sendTorCommand("GETINFO status/circuit-established");
    sendTorCommand("GETINFO traffic/read");
    sendTorCommand("GETINFO traffic/written");
    // Если цепочка ещё не отображается — запрашиваем список текущих
    if (lblCircuitInfo && (lblCircuitInfo->text() == "Цепочка: Н/Д" ||
        lblCircuitInfo->text().contains("Строится")))
        sendTorCommand("GETINFO circuit-status");
}

// ========== УПРАВЛЕНИЕ OPENVPN СЕРВЕРОМ ==========

void MainWindow::startOpenVPNServer()
{
    if (serverMode) {
        addLogMessage("Сервер уже запущен", "warning");
        return;
    }

    if (serverStopPending) {
        addLogMessage("Сервер в процессе остановки, подождите...", "warning");
        return;
    }

    if (!checkOpenVPNInstalled()) {
        QMessageBox::critical(this, "Ошибка", "OpenVPN не найден");
        serverTorWaitRetries = 0;
        return;
    }

    // Проверяем наличие сертификатов
    bool usePfs = (chkPfs && chkPfs->isChecked());
    bool certsOk = QFile::exists(caCertPath) &&
    QFile::exists(serverCertPath) &&
    QFile::exists(serverKeyPath) &&
    (usePfs || QFile::exists(dhParamPath));

    if (!certsOk) {
        QString missing;
        if (!QFile::exists(caCertPath))     missing += "\n• CA сертификат: " + caCertPath;
        if (!QFile::exists(serverCertPath)) missing += "\n• Сертификат сервера: " + serverCertPath;
        if (!QFile::exists(serverKeyPath))  missing += "\n• Ключ сервера: " + serverKeyPath;
        if (!usePfs && !QFile::exists(dhParamPath)) missing += "\n• DH параметры: " + dhParamPath;

        addLogMessage("Отсутствуют файлы:" + missing, "error");

        QMessageBox::StandardButton reply = QMessageBox::question(this, "Сертификаты не найдены",
                                                                  "Не найдены следующие файлы:" + missing + "\n\nСгенерировать сертификаты сейчас?",
                                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            generateCertificates();
        } else {
            serverTorWaitRetries = 0;
        }
        return;
    }

    // Проверяем нужен ли Tor
    if (chkRouteThroughTor->isChecked() && !torRunning) {
        if (serverTorWaitRetries == 0) {
            addLogMessage("Запуск Tor перед стартом сервера...", "info");
            startTor();
        }

        serverTorWaitRetries++;
        if (serverTorWaitRetries > 5) {
            serverTorWaitRetries = 0;
            addLogMessage("Tor не запустился за отведённое время. Отмена старта сервера.", "error");
            QMessageBox::critical(this, "Ошибка", "Не удалось запустить Tor. Сервер не запущен.");
            return;
        }

        addLogMessage(QString("Ожидание Tor для сервера... попытка %1/5").arg(serverTorWaitRetries), "info");
        QTimer::singleShot(5000, this, &MainWindow::startOpenVPNServer);
        return;
    }

    serverTorWaitRetries = 0;

    addLogMessage("Запуск OpenVPN сервера...", "info");
    addLogMessage(QString("Директория сертификатов: %1").arg(certsDir), "info");
    addLogMessage(QString("  CA:   %1 [%2]").arg(caCertPath,     QFile::exists(caCertPath)     ? "✓" : "✗ НЕТ"), "info");
    addLogMessage(QString("  cert: %1 [%2]").arg(serverCertPath, QFile::exists(serverCertPath) ? "✓" : "✗ НЕТ"), "info");
    addLogMessage(QString("  key:  %1 [%2]").arg(serverKeyPath,  QFile::exists(serverKeyPath)  ? "✓" : "✗ НЕТ"), "info");
    addLogMessage(QString("  dh:   %1 [%2]").arg(dhParamPath,    QFile::exists(dhParamPath)    ? "✓" : "✗ НЕТ"), "info");

    createServerConfig();

    if (!validateServerConfig())
        return;

    QStringList args;
    args << "--config" << serverConfigPath;

    QString command = openVPNExecutablePath;

    // Проверка что бинарник существует и исполняемый
    QFileInfo ovpnBin(openVPNExecutablePath);
    if (!ovpnBin.exists() || !ovpnBin.isExecutable()) {
        addLogMessage("Ошибка: OpenVPN не найден или не исполняемый: " + openVPNExecutablePath, "error");
        QMessageBox::critical(this, "Ошибка запуска",
                              "OpenVPN не найден или нет прав на запуск:\n" + openVPNExecutablePath +
                              "\n\nПроверьте путь в Настройках.");
        return;
    }

    #ifdef Q_OS_LINUX
    if (geteuid() != 0) {
        args.prepend(command);
        command = "pkexec";
    }
    #endif

    // Проверяем порт перед запуском
    int port = spinServerPort ? spinServerPort->value() : DEFAULT_VPN_SERVER_PORT;
    QProcess portCheck;
    portCheck.start("sh", {"-c", QString("ss -ulnp 2>/dev/null | grep -q ':%1 '").arg(port)});
    portCheck.waitForFinished(2000);
    if (portCheck.exitCode() == 0) {
        // Порт занят — ищем кем
        QProcess lsofProc;
        lsofProc.start("sh", {"-c", QString("lsof -ti udp:%1 2>/dev/null || "
            "ss -ulnp 2>/dev/null | grep ':%1 ' | "
            "grep -oP 'pid=\\K[0-9]+'").arg(port)});
        lsofProc.waitForFinished(2000);
        QString pids = QString::fromUtf8(lsofProc.readAllStandardOutput()).trimmed();

        addLogMessage(QString("✗ Порт UDP/%1 уже занят%2!")
        .arg(port)
        .arg(pids.isEmpty() ? "" : QString(" (PID: %1)").arg(pids.replace('\n', ','))),
                      "error");

        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Порт занят",
            QString("Порт UDP/%1 уже используется%2.\n\n"
            "Завершить занявший порт процесс и запустить сервер?")
            .arg(port)
            .arg(pids.isEmpty() ? "" : QString(" (PID: %1)").arg(pids)),
                                                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
        );

        if (reply == QMessageBox::Yes) {
            QProcess killProc;
            if (!pids.isEmpty()) {
                for (const QString &pid : pids.split(',')) {
                    QString p = pid.trimmed();
                    if (!p.isEmpty()) {
                        killProc.start("kill", {"-TERM", p});
                        killProc.waitForFinished(2000);
                    }
                }
            } else {
                killProc.start("pkill", {"-TERM", "openvpn"});
                killProc.waitForFinished(2000);
            }
            addLogMessage("Процесс завершён. Повторный запуск через 2 секунды...", "info");
            QTimer::singleShot(2000, this, &MainWindow::startOpenVPNServer);
        }
        return;
    }

    openVPNServerProcess->start(command, args);

    // Проверяем что процесс начал запускаться
    if (!openVPNServerProcess->waitForStarted(5000)) {
        addLogMessage("Ошибка: OpenVPN не запустился (waitForStarted failed): "
        + openVPNServerProcess->errorString(), "error");
        btnStartServer->setEnabled(true);
        btnStopServer->setEnabled(false);
        return;
    }

    // Процесс запустился — кнопка Стоп должна быть активна
    btnStartServer->setEnabled(false);
    btnStopServer->setEnabled(true);
}

// В MainWindow::stopOpenVPNServer добавить:

void MainWindow::stopOpenVPNServer()
{
    if (!serverMode) {
        addLogMessage("Сервер не запущен", "warning");
        return;
    }

    if (serverStopPending) {
        addLogMessage("Сервер уже останавливается...", "warning");
        return;
    }

    addLogMessage("Остановка OpenVPN сервера...", "info");
    serverStopPending = true;

    // Останавливаем мониторинг URL
    if (dnsMonitor) {
        dnsMonitor->stopMonitoring();
    }

    openVPNServerProcess->terminate();

    QTimer::singleShot(5000, this, [this]() {
        if (openVPNServerProcess->state() == QProcess::Running) {
            addLogMessage("Сервер не завершился по terminate, применяем kill...", "warning");
            openVPNServerProcess->kill();
        }
    });

    btnStartServer->setEnabled(false);
    btnStopServer->setEnabled(false);
}

void MainWindow::onServerStarted()
{
    serverMode = true;
    serverStartTime = QDateTime::currentDateTime();
    btnStartServer->setEnabled(false);
    btnStopServer->setEnabled(true);
    addLogMessage(QString("[OpenVPN] Сервер запущен | порт %1 | PID %2")
    .arg(spinServerPort ? spinServerPort->value() : 0)
    .arg(openVPNServerProcess ? (int)openVPNServerProcess->processId() : 0), "success");
    lblServerStatus->setText("Сервер: <b style='color:green;'>Запущен</b>");
    setConnectionState("server_mode");

    // Синхронизируем адрес с Telegram-ботом
    if (tgBotManager && txtServerAddress && spinServerPort) {
        QString proto = (cboServerProto && cboServerProto->currentText() == "UDP") ? "udp" : "tcp";
        tgBotManager->setServerAddress(txtServerAddress->text().trimmed(),
                                       spinServerPort->value(), proto);
    }

    enableIPForwarding();
    QTimer::singleShot(1000, this, &MainWindow::checkIPLeak);

    addLogMessage("Ожидание инициализации tun-интерфейса...", "info");
    QTimer::singleShot(8000, this, [this]() {
        addLogMessage("Проверка маршрутизации после старта сервера...", "info");
        bool routingOk = false;

        // Безопасно получаем сеть VPN — только первое слово, проверяем формат
        QString rawNet = txtServerNetwork->text().split(' ').first();
        // Валидируем: только цифры и точки — защита от injection
        static const QRegularExpression validNet("^[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}$");
        if (!validNet.match(rawNet).hasMatch()) {
            addLogMessage("Предупреждение: некорректный формат сети VPN: " + rawNet, "warning");
            rawNet = "10.8.0.0";
        }
        QString vpnNet = rawNet + "/24";

        // Запускаем iptables напрямую через аргументы — без sh -c
        QProcess checkProc;
        checkProc.start("iptables", {"-t", "nat", "-L", "POSTROUTING", "-n"});
        checkProc.waitForFinished(3000);
        QString checkNat = QString::fromUtf8(checkProc.readAllStandardOutput());

        if (checkNat.contains(rawNet)) {
            addLogMessage("✓ NAT правило активно (настроено скриптом up)", "success");
            routingOk = true;
        }

        if (!routingOk) {
            addLogMessage("NAT правило не найдено, применяем вручную...", "warning");
            bool success = setupIPTablesRules(true);
            if (success) {
                addLogMessage("Маршрутизация настроена успешно (fallback)", "success");
            } else {
                addLogMessage("Ошибка настройки маршрутизации, пробуем applyRoutingManually...", "error");
                applyRoutingManually();
            }
        }

        QTimer::singleShot(3000, this, &MainWindow::verifyRouting);
    });

    emit serverStarted();

}

void MainWindow::onServerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    serverMode = false;
    serverStopPending = false;

    QString remainingOut = QString::fromUtf8(openVPNServerProcess->readAllStandardOutput()).trimmed();
    QString remainingErr = QString::fromUtf8(openVPNServerProcess->readAllStandardError()).trimmed();
    if (!remainingOut.isEmpty()) {
        addLogMessage("OpenVPN последний вывод:\n" + remainingOut, "info");
    }
    if (!remainingErr.isEmpty()) {
        addLogMessage("OpenVPN ошибка при завершении:\n" + remainingErr, "error");
    }

    QString message = QString("Сервер OpenVPN завершен с кодом %1").arg(exitCode);
    addLogMessage(message, exitStatus == QProcess::NormalExit ? "info" : "error");

    // Детектируем ошибку занятого порта
    bool portInUse = remainingErr.contains("Address already in use") ||
    remainingOut.contains("Address already in use") ||
    remainingErr.contains("bind failed") ||
    remainingOut.contains("bind failed");

    if (portInUse) {
        int port = spinServerPort ? spinServerPort->value() : DEFAULT_VPN_SERVER_PORT;
        addLogMessage(QString("✗ Порт %1 уже занят другим процессом!").arg(port), "error");

        // Ищем PID процесса занявшего порт
        QProcess lsofProc;
        lsofProc.start("sh", {"-c", QString("lsof -ti :%1 -sTCP:LISTEN 2>/dev/null || "
            "ss -tlnp 2>/dev/null | grep ':%1 ' | "
            "grep -oP 'pid=\\K[0-9]+'").arg(port)});
        lsofProc.waitForFinished(3000);
        QString pids = QString::fromUtf8(lsofProc.readAllStandardOutput()).trimmed();

        QString hint = QString("Порт %1 занят").arg(port);
        if (!pids.isEmpty()) {
            hint += QString(" (PID: %1)").arg(pids.replace('\n', ','));
        }
        addLogMessage(hint + ". Возможно, уже запущен другой экземпляр OpenVPN.", "warning");

        // Предлагаем убить старый процесс
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Порт занят",
            QString("Порт %1 уже используется.\n\n%2\n\n"
            "Завершить занявший порт процесс и попробовать снова?")
            .arg(port)
            .arg(pids.isEmpty() ? "" : QString("PID: %1").arg(pids)),
                                                                  QMessageBox::Yes | QMessageBox::No,
                                                                  QMessageBox::Yes
        );

        if (reply == QMessageBox::Yes) {
            QProcess killProc;
            if (!pids.isEmpty()) {
                for (const QString &pid : pids.split(',')) {
                    QString p = pid.trimmed();
                    if (!p.isEmpty()) {
                        killProc.start("kill", {"-TERM", p});
                        killProc.waitForFinished(2000);
                    }
                }
            } else {
                killProc.start("pkill", {"-TERM", "openvpn"});
                killProc.waitForFinished(2000);
            }
            addLogMessage("Процесс завершён. Повторный запуск через 2 секунды...", "info");
            QTimer::singleShot(2000, this, &MainWindow::startOpenVPNServer);
        }
        return;
    }

    if (exitCode == 1 && remainingErr.isEmpty() && remainingOut.isEmpty()) {
        addLogMessage("Подсказка: запустите вручную для диагностики:", "warning");
        addLogMessage("  openvpn --config " + serverConfigPath, "warning");
        addLogMessage("  Или: cat /tmp/openvpn-server.log", "warning");
    }

    lblServerStatus->setText("Сервер: <b style='color:red;'>Остановлен</b>");
    lblConnectedClients->setText("Всего подключений: 0");
    btnStartServer->setEnabled(true);
    btnStopServer->setEnabled(false);
    connectedClients = 0;

    // Очищаем таблицу клиентов
    clientsCache.clear();
    updateClientsTable();

    setupIPTablesRules(false);

    setConnectionState(torRunning ? "tor_only" : "disconnected");
}

void MainWindow::onServerError(QProcess::ProcessError error)
{
    QString errorMsg;
    QString hint;
    switch (error) {
        case QProcess::FailedToStart:
            errorMsg = "Не удалось запустить OpenVPN сервер";
            hint = QString("Путь: %1\nПроверьте путь в Настройках. Для запуска сервера нужны права root или pkexec.")
            .arg(openVPNExecutablePath.isEmpty() ? "(не задан)" : openVPNExecutablePath);
            break;
        case QProcess::Crashed:
            errorMsg = "OpenVPN сервер аварийно завершился";
            hint = "Проверьте журнал сервера и tormanager_errors.log для деталей.";
            break;
        case QProcess::Timedout:
            errorMsg = "Таймаут запуска OpenVPN сервера";
            hint = "";
            break;
        default:
            errorMsg = QString("Неизвестная ошибка OpenVPN: %1").arg(error);
            hint = "";
    }

    // Сбрасываем состояние
    serverMode = false;
    serverStopPending = false;
    if (lblServerStatus)
        lblServerStatus->setText("Сервер: <b style='color:red;'>Ошибка</b>");
    if (btnStartServer) btnStartServer->setEnabled(true);
    if (btnStopServer)  btnStopServer->setEnabled(false);

    addLogMessage(errorMsg + (hint.isEmpty() ? "" : " — " + hint), "error");
    QMessageBox::critical(this, "Ошибка OpenVPN",
                          errorMsg + (hint.isEmpty() ? "" : "\n\n" + hint));
}

void MainWindow::onServerReadyRead()
{
    QString output = QString::fromUtf8(openVPNServerProcess->readAll());
    if (output.isEmpty()) return;

    QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        txtServerLog->append(trimmed);

        // Анализируем ключевые события
        if (trimmed.contains("Initialization Sequence Completed")) {
            addLogMessage("[OpenVPN] ✓ Сервер готов к приёму подключений!", "success");
            QTimer::singleShot(1000, this, &MainWindow::updateClientsTable);

        } else if (trimmed.contains("Peer Connection Initiated with")) {
            QRegularExpression ipRe("\\[AF_INET\\](\\S+)");
            QRegularExpressionMatch m = ipRe.match(trimmed);
            QString clientAddr = m.hasMatch() ? m.captured(1) : "?";
            addLogMessage("[OpenVPN] ✓ Клиент подключился: " + clientAddr, "success");

            // Добавляем в журнал клиентов
            QString timestamp = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
            txtClientsLog->append(QString("<span style='color:green;'>[%1] Подключение: %2</span>")
            .arg(timestamp).arg(clientAddr));

            QTimer::singleShot(2000, this, &MainWindow::updateClientsTable);
            QTimer::singleShot(1500, this, &MainWindow::checkIPLeak);

        } else if (trimmed.contains("will cause previous active sessions")) {
            addLogMessage("[OpenVPN] Клиент переподключился (предыдущая сессия закрыта)", "info");
            QTimer::singleShot(2000, this, &MainWindow::updateClientsTable);

        } else if (trimmed.contains("client-instance exiting")) {
            addLogMessage("[OpenVPN] Клиент отключился", "info");

            // Добавляем в журнал клиентов
            QString timestamp = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
            txtClientsLog->append(QString("<span style='color:orange;'>[%1] Отключение клиента</span>")
            .arg(timestamp));

            QTimer::singleShot(1500, this, &MainWindow::updateClientsTable);

        } else if (trimmed.contains("connection-reset") ||
            trimmed.contains("client-instance restarting")) {
            QTimer::singleShot(3000, this, &MainWindow::updateClientsTable);

            } else if (trimmed.contains("MULTI: bad source address")) {
                static int badSrcCount = 0;
                if (++badSrcCount <= 2)
                    addLogMessage("[OpenVPN] ⚠ bad source address (LAN-клиент, норма при старте)", "warning");

            } else if (trimmed.contains("FATAL") ||
                (trimmed.contains("ERROR") && !trimmed.contains("error:0"))) {
            addLogMessage("[OpenVPN] ✗ " + trimmed, "error");

                } else if (trimmed.contains("WARNING") &&
                   !trimmed.contains("net30") &&
                   !trimmed.contains("data-ciphers-fallback")) {
            addLogMessage("[OpenVPN] ⚠ " + trimmed, "warning");
                   }
    }

    QTextCursor cursor = txtServerLog->textCursor();
    cursor.movePosition(QTextCursor::End);
    txtServerLog->setTextCursor(cursor);
}

// ========== НОВЫЕ МЕТОДЫ ДЛЯ УПРАВЛЕНИЯ КЛИЕНТАМИ ==========

void MainWindow::updateClientsTable()
{
    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    if (!serverMode) {
        lblActiveClients->setText("Активных сейчас: <b>0</b>");
        if (clientsTable) clientsTable->setRowCount(0);
        activeIPsPerClient.clear();
        updateRegistryTable();
        return;
    }

    QFile statusFile("/tmp/openvpn-status.log");
    if (!statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        updateRegistryTable();
        return;
    }

    // Читаем файл
    QByteArray rawData = statusFile.readAll();
    statusFile.close();

    // Декодируем содержимое
    QString statusContent;
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QStringDecoder utf8Decoder(QStringDecoder::Utf8);
    statusContent = utf8Decoder(rawData);
    if (utf8Decoder.hasError()) {
        QTextStream stream(rawData);
        statusContent = stream.readAll();
        addLogMessage("⚠️ Обнаружены проблемы с UTF-8 в статус-файле", "warning");
    }
    #else
    QTextCodec *utf8Codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::ConverterState utf8State;
    statusContent = utf8Codec->toUnicode(rawData.constData(), rawData.size(), &utf8State);
    if (utf8State.invalidChars > 0 || utf8State.remainingChars > 0) {
        QTextCodec *systemCodec = QTextCodec::codecForLocale();
        statusContent = systemCodec->toUnicode(rawData);
        addLogMessage("⚠️ Обнаружены проблемы с UTF-8 в статус-файле", "warning");
    }
    #endif

    // Парсинг status-version 2
    QMap<QString, ClientInfo> newClients;
    QMap<QString, QStringList> newActiveIPsPerClient;

    QStringList lines = statusContent.split('\n', Qt::SkipEmptyParts);
    for (const QString &rawLine : lines) {
        QString line = rawLine.trimmed();
        if (!line.startsWith("CLIENT_LIST,")) continue;

        QStringList p = line.split(',');
        if (p.size() < 8) continue;

        ClientInfo client;

        QByteArray cnData = p[1].trimmed().toUtf8();
        client.commonName = decodeClientName(cnData);

        client.realAddress         = p[2].trimmed();
        client.virtualAddress      = p[3].trimmed();
        client.virtualIPv6         = p[4].trimmed();
        client.bytesReceived       = p[5].toLongLong();
        client.bytesSent           = p[6].toLongLong();
        client.connectedSinceEpoch = p.size() > 8 ? p[8].toLongLong() : 0;

        if (client.connectedSinceEpoch > 0) {
            client.connectedSince = QDateTime::fromSecsSinceEpoch(client.connectedSinceEpoch);
        } else {
            client.connectedSince = QDateTime::fromString(p[7].trimmed(), "ddd MMM dd HH:mm:ss yyyy");
            if (!client.connectedSince.isValid()) {
                client.connectedSince = QDateTime::fromString(p[7].trimmed(), "yyyy-MM-dd HH:mm:ss");
            }
        }

        client.pid      = p.size() > 9 ? p[9].toLongLong() : 0;
        client.isActive = true;

        if (client.commonName.isEmpty() || client.commonName == "UNDEF") continue;

        QString ipOnly = client.realAddress.split(':').first();
        newActiveIPsPerClient[client.commonName].append(ipOnly);

        // Расчет скорости со сглаживанием
        QString key = client.commonName + ":" + client.realAddress;
        if (clientsCache.contains(key)) {
            const ClientInfo &prev = clientsCache[key];
            qint64 dtMs = nowMs - lastClientRefreshMs;
            if (dtMs > 0) {
                double dt = dtMs / 1000.0;
                qint64 instantRxBps = qMax(0LL, (qint64)((client.bytesReceived - prev.bytesReceived) / dt));
                qint64 instantTxBps = qMax(0LL, (qint64)((client.bytesSent - prev.bytesSent) / dt));

                // Экспоненциальное сглаживание
                client.speedRxBps = prev.speedRxBps * 0.7 + instantRxBps * 0.3;
                client.speedTxBps = prev.speedTxBps * 0.7 + instantTxBps * 0.3;
            }
        } else {
            client.speedRxBps = 0;
            client.speedTxBps = 0;
        }
        newClients[key] = client;
    }

    // Обновление статистики скорости через ClientStats
    // ВАЖНО: делаем это ДО сохранения в clientsCache, чтобы скорости попали в кэш
    QSet<QString> activeClients;
    for (auto it = newClients.begin(); it != newClients.end(); ++it) {
        activeClients.insert(it.value().commonName);
    }

    for (auto it = newClients.begin(); it != newClients.end(); ++it) {
        ClientInfo &client = it.value();
        QString clientId = client.commonName;

        if (clientStats) {
            qint64 interval = nowMs - lastClientRefreshMs;
            if (interval > 0) {
                clientStats->updateSpeed(clientId, client.bytesReceived,
                                         client.bytesSent, interval);
                client.speedRxBps = clientStats->getAvgRxSpeed(clientId);
                client.speedTxBps = clientStats->getAvgTxSpeed(clientId);
            }
        }
    }

    if (clientStats) {
        clientStats->cleanup(activeClients);
    }

    // Теперь сохраняем кэш уже с правильными скоростями
    activeIPsPerClient = newActiveIPsPerClient;
    lastClientRefreshMs = nowMs;
    clientsCache = newClients;

    // Обновляем client map для DNS монитора
    if (dnsMonitor) {
        QMap<QString, QString> ipMap;
        for (auto it = clientsCache.begin(); it != clientsCache.end(); ++it) {
            ipMap[it.value().virtualAddress] = it.value().commonName;
        }
        dnsMonitor->setClientMap(ipMap);
    }

    // Детектор мульти-подключений
    static QSet<QString> warnedMultiConnections;
    QString timestamp = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");

    for (auto it = newActiveIPsPerClient.begin(); it != newActiveIPsPerClient.end(); ++it) {
        const QString &cn = it.key();
        QStringList uniqueIps = it.value();
        uniqueIps.removeDuplicates();

        if (uniqueIps.size() > 1) {
            QString warnKey = cn + ":" + QString::number(uniqueIps.size());
            if (!warnedMultiConnections.contains(warnKey)) {
                warnedMultiConnections.insert(warnKey);
                QString ipList = uniqueIps.join(", ");
                QString logMsg = QString(
                    "[%1] 🚨 МУЛЬТИ-ПОДКЛЮЧЕНИЕ: %2 — %3 разных IP: %4")
                .arg(timestamp, cn).arg(uniqueIps.size()).arg(ipList);

                // Используем addLogMessage вместо прямого append
                addLogMessage(logMsg, "warning");

                if (trayIcon && chkShowTrayNotifications && chkShowTrayNotifications->isChecked()) {
                    QMetaObject::invokeMethod(this, [this, cn, ipList, uniqueIps]() {
                        trayIcon->showMessage(
                            "⚠️ Подозрительная активность",
                            QString("Клиент '%1' подключён с %2 разных IP!\n%3")
                            .arg(cn).arg(uniqueIps.size()).arg(ipList),
                                              QSystemTrayIcon::Warning, 8000);
                    }, Qt::QueuedConnection);
                }
            }
        } else {
            for (int i = 2; i <= 5; ++i) {
                warnedMultiConnections.remove(cn + ":" + QString::number(i));
            }
        }
    }

    // Обнаруживаем новые подключения и отключения
    QDateTime now = QDateTime::currentDateTime();

    // Новые подключения
    for (auto it = newClients.begin(); it != newClients.end(); ++it) {
        if (!clientsCache.contains(it.key())) {
            const ClientInfo &c = it.value();
            QString cn = c.commonName;

            QString logMsg = QString(
                "[%1] ✅ %2 подключился | IP: %3 | VPN: %4")
            .arg(timestamp, cn, c.realAddress, c.virtualAddress);

            addLogMessage(logMsg, "info");

            // ДОБАВЛЕН МЬЮТЕКС
            {
                QMutexLocker locker(&registryMutex);
                if (!clientRegistry.contains(cn)) {
                    ClientRecord rec;
                    rec.displayName = cn;
                    rec.firstSeen = c.connectedSince;
                    rec.sessionCount = 1;
                    rec.totalBytesRx = 0;
                    rec.totalBytesTx = 0;
                    rec.totalOnlineSeconds = 0;
                    rec.isBanned = false;
                    clientRegistry[cn] = rec;
                } else {
                    clientRegistry[cn].sessionCount++;
                }
                clientRegistry[cn].lastSeen = now;
                clientRegistry[cn].knownIPs.insert(c.realAddress.split(':').first());
            }
        }
    }

    // Отключения
    for (auto it = clientsCache.begin(); it != clientsCache.end(); ++it) {
        if (!newClients.contains(it.key())) {
            const ClientInfo &c = it.value();
            QString cn = c.commonName;
            qint64 onlineSecs = c.connectedSince.isValid()
            ? qMax(0LL, c.connectedSince.secsTo(now)) : 0;

            QString logMsg = QString(
                "[%1] 🔴 %2 отключился | IP: %3 | Онлайн: %4 | ↓ %5 KB ↑ %6 KB")
            .arg(timestamp, cn, c.realAddress)
            .arg(formatDuration(onlineSecs))
            .arg(c.bytesReceived / 1024)
            .arg(c.bytesSent / 1024);

            addLogMessage(logMsg, "info");

            // ДОБАВЛЕН МЬЮТЕКС
            {
                QMutexLocker locker(&registryMutex);
                if (clientRegistry.contains(cn)) {
                    ClientRecord &rec = clientRegistry[cn];

                    SessionRecord session;
                    session.connectedAt = c.connectedSince;
                    session.disconnectedAt = now;
                    session.sourceIP = c.realAddress;
                    session.vpnIP = c.virtualAddress;
                    session.bytesReceived = c.bytesReceived;
                    session.bytesSent = c.bytesSent;
                    session.durationSeconds = onlineSecs;
                    rec.sessions.append(session);

                    rec.totalBytesRx += c.bytesReceived;
                    rec.totalBytesTx += c.bytesSent;
                    rec.totalOnlineSeconds += onlineSecs;
                    rec.lastSeen = now;
                }
            }
        }
    }

    // Сохраняем реестр
    saveClientRegistry();
    updateRegistryTable();

    emit registryUpdated(clientRegistry);
    emit clientsUpdated();
}

// Вспомогательный метод для обновления UI таблицы
void MainWindow::updateClientsTableUI(const QMap<QString, ClientInfo> &newClients, const QDateTime &now)
{
    if (!clientsTable) return;

    clientsTable->setSortingEnabled(false);
    clientsTable->setRowCount(newClients.size());

    int row = 0;
    for (auto it = newClients.begin(); it != newClients.end(); ++it) {
        const ClientInfo &client = it.value();

        bool isMulti = activeIPsPerClient.value(client.commonName).size() > 1;
        QString nameText = isMulti ? ("⚠️ " + client.commonName) : client.commonName;

        QTableWidgetItem *nameItem = new QTableWidgetItem(nameText);
        nameItem->setData(Qt::UserRole, client.realAddress);
        nameItem->setData(Qt::UserRole + 1, client.commonName);
        nameItem->setData(Qt::UserRole + 2, client.connectedSinceEpoch);
        nameItem->setFont(QFont("", -1, QFont::Bold));
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        if (isMulti) nameItem->setForeground(QColor(180, 0, 0));
        clientsTable->setItem(row, 0, nameItem);

        clientsTable->setItem(row, 1, new QTableWidgetItem(client.virtualAddress));
        clientsTable->setItem(row, 2, new QTableWidgetItem(formatBytes(client.bytesReceived)));
        clientsTable->setItem(row, 3, new QTableWidgetItem(formatBytes(client.bytesSent)));

        qint64 onlineSecs = client.connectedSince.isValid()
        ? qMax(0LL, client.connectedSince.secsTo(now)) : 0;
        QString connTime = client.connectedSince.isValid()
        ? client.connectedSince.toString("HH:mm dd.MM") : "—";
        QTableWidgetItem *timeItem = new QTableWidgetItem(
            connTime + "  (" + formatDuration(onlineSecs) + ")");
        clientsTable->setItem(row, 4, timeItem);

        // Статус
        QString statusText;
        QColor statusColor;

        // Копируем данные под мьютексом
        ClientRecord recCopy;
        bool hasRec = false;
        {
            QMutexLocker locker(&registryMutex);
            if (clientRegistry.contains(client.commonName)) {
                recCopy = clientRegistry[client.commonName];
                hasRec = true;
            }
        }

        if (hasRec) {
            if (recCopy.expiryDate.isValid()) {
                int daysLeft = QDate::currentDate().daysTo(recCopy.expiryDate);
                if (daysLeft < 0) {
                    statusText = "⛔ Истёк";
                    statusColor = QColor(255, 150, 150);
                } else if (daysLeft <= 7 && !isMulti) {
                    statusText = "⚠️ Осталось " + QString::number(daysLeft) + " дн.";
                    statusColor = QColor(255, 240, 180);
                } else {
                    statusText = "🟢 Активен";
                    statusColor = QColor(200, 255, 200);
                }
            } else if (recCopy.isBanned) {
                statusText = "🚫 Заблокирован";
                statusColor = QColor(255, 150, 150);
            } else {
                statusText = isMulti ? "⚠️ Мульти-IP" : "🟢 Активен";
                statusColor = isMulti ? QColor(255, 180, 100) : QColor(200, 255, 200);
            }
        } else {
            statusText = isMulti ? "⚠️ Мульти-IP" : "🟢 Активен";
            statusColor = isMulti ? QColor(255, 180, 100) : QColor(200, 255, 200);
        }

        QTableWidgetItem *statusItem = new QTableWidgetItem(statusText);
        statusItem->setBackground(statusColor);
        clientsTable->setItem(row, 5, statusItem);

        row++;
    }

    clientsTable->setSortingEnabled(true);

    // Обновляем статистику
    if (lblActiveClients) {
        lblActiveClients->setText(QString("Активных сейчас: <b>%1</b>").arg(newClients.size()));
    }

    if (lblConnectedClients) {
        lblConnectedClients->setText(QString("Активных клиентов: <b>%1</b>").arg(newClients.size()));
    }
}

// Вспомогательные методы для форматирования
QString MainWindow::formatBytes(qint64 bytes) const
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024) + " KB";
    if (bytes < 1024LL * 1024 * 1024)
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

QString MainWindow::formatDuration(qint64 seconds) const
{
    if (seconds < 0) seconds = 0;
    if (seconds < 60) return QString("%1 сек").arg(seconds);
    if (seconds < 3600) return QString("%1 мин %2 сек").arg(seconds / 60).arg(seconds % 60);
    if (seconds < 86400)
        return QString("%1 ч %2 мин").arg(seconds / 3600).arg((seconds % 3600) / 60);
    return QString("%1 дн %2 ч").arg(seconds / 86400).arg((seconds % 86400) / 3600);
}

QString MainWindow::decodeClientName(const QByteArray &data)
{
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt6: используем QStringDecoder
    QStringDecoder utf8Decoder(QStringDecoder::Utf8);
    QString result = utf8Decoder(data);

    // Если декодирование не удалось (есть ошибки), пробуем системную локаль
    if (utf8Decoder.hasError()) {
        QStringDecoder localDecoder(QStringDecoder::System);
        result = localDecoder(data);
    }
    return result;
    #else
    // Qt5: используем QTextCodec
    QTextCodec *utf8Codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::ConverterState utf8State;
    QString result = utf8Codec->toUnicode(data.constData(), data.size(), &utf8State);

    if (utf8State.invalidChars == 0 && utf8State.remainingChars == 0) {
        return result;  // UTF-8 корректен
    }

    // Если UTF-8 не работает, пробуем системную локаль
    QTextCodec *systemCodec = QTextCodec::codecForLocale();
    return systemCodec->toUnicode(data);
    #endif
}


// ─── Поиск ta.key по нескольким возможным расположениям ──────────────────────
// easy-rsa хранит ta.key в easy-rsa/pki/, а не в корне certsDir
// Обновляет taKeyPath и возвращает найденный путь
QString MainWindow::resolveTaKeyPath()
{
    QStringList candidates = {
        certsDir + "/ta.key",
        certsDir + "/easy-rsa/pki/ta.key",
        certsDir + "/easy-rsa/pki/private/ta.key"
    };
    for (const QString &p : candidates) {
        if (QFile::exists(p)) { taKeyPath = p; return p; }
    }
    taKeyPath = certsDir + "/ta.key"; // fallback
    return taKeyPath;
}

// ─── Извлечение ta.key из server.conf ────────────────────────────────────────
// ВАЖНО: сервер использует именно этот ключ — берём его для гарантии совпадения
// Несовпадение ta.key даёт ошибку "tls-crypt unwrap error: packet authentication failed"
QString MainWindow::extractTaKeyFromServerConf() const
{
    if (!QFile::exists(serverConfigPath)) return QString();
    QFile f(serverConfigPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    QString content = f.readAll();
    f.close();

    int start = content.indexOf("<tls-crypt>");
    int end   = content.indexOf("</tls-crypt>", start);
    if (start == -1 || end == -1) return QString();

    QString block = content.mid(start + 11, end - start - 11).trimmed();
    if (!block.contains("-----BEGIN OpenVPN Static key V1-----")) return QString();
    return block;
}

// ─── Поиск клиентского сертификата по нескольким путям ───────────────────────
QString MainWindow::resolveClientCertPath(const QString &cn) const
{
    QStringList candidates = {
        certsDir + "/" + cn + ".crt",
        certsDir + "/easy-rsa/pki/" + cn + ".crt",
        certsDir + "/easy-rsa/pki/issued/" + cn + ".crt"
    };
    for (const QString &p : candidates) {
        if (QFile::exists(p)) return p;
    }
    return certsDir + "/" + cn + ".crt"; // fallback
}

QString MainWindow::resolveClientKeyPath(const QString &cn) const
{
    QStringList candidates = {
        certsDir + "/" + cn + ".key",
        certsDir + "/easy-rsa/pki/" + cn + ".key",
        certsDir + "/easy-rsa/pki/private/" + cn + ".key"
    };
    for (const QString &p : candidates) {
        if (QFile::exists(p)) return p;
    }
    return certsDir + "/" + cn + ".key"; // fallback
}

// ─── Транслитерация и санитизация имени клиента (CN сертификата) ─────────────
// Поддерживает русские, английские имена и любые Unicode символы
QString MainWindow::sanitizeClientName(const QString &name)
{
    QString clean = name.trimmed();

    // Таблица транслитерации кириллицы
    static const QVector<QPair<QString,QString>> translit = {
        {"а","a"},{"б","b"},{"в","v"},{"г","g"},{"д","d"},
        {"е","e"},{"ё","yo"},{"ж","zh"},{"з","z"},{"и","i"},
        {"й","y"},{"к","k"},{"л","l"},{"м","m"},{"н","n"},
        {"о","o"},{"п","p"},{"р","r"},{"с","s"},{"т","t"},
        {"у","u"},{"ф","f"},{"х","kh"},{"ц","ts"},{"ч","ch"},
        {"ш","sh"},{"щ","sch"},{"ъ",""},{"ы","y"},{"ь",""},
        {"э","e"},{"ю","yu"},{"я","ya"},
        {"А","A"},{"Б","B"},{"В","V"},{"Г","G"},{"Д","D"},
        {"Е","E"},{"Ё","Yo"},{"Ж","Zh"},{"З","Z"},{"И","I"},
        {"Й","Y"},{"К","K"},{"Л","L"},{"М","M"},{"Н","N"},
        {"О","O"},{"П","P"},{"Р","R"},{"С","S"},{"Т","T"},
        {"У","U"},{"Ф","F"},{"Х","Kh"},{"Ц","Ts"},{"Ч","Ch"},
        {"Ш","Sh"},{"Щ","Sch"},{"Ъ",""},{"Ы","Y"},{"Ь",""},
        {"Э","E"},{"Ю","Yu"},{"Я","Ya"}
    };
    for (const auto &[from, to] : translit)
        clean.replace(from, to);

    // Оставляем только ASCII символы
    QString ascii;
    ascii.reserve(clean.size());
    for (const QChar &ch : clean) {
        if (ch.unicode() < 128)
            ascii += ch;
    }
    clean = ascii;

    // Заменяем недопустимые символы на '_', схлопываем повторные '_'
    clean.replace(QRegularExpression("[^a-zA-Z0-9_\\-]"), "_");
    clean.replace(QRegularExpression("_+"), "_");
    clean.remove(QRegularExpression("^_+|_+$"));

    if (clean.length() > 64) clean = clean.left(64);
    return clean.isEmpty() ? "client" : clean;
}

void MainWindow::refreshClientsNow()
{
    updateClientsTable();
    addLogMessage("Список клиентов обновлён вручную", "info");
}

// ========== ДЕТЕКТОР ОДНОВРЕМЕННЫХ ПОДКЛЮЧЕНИЙ ==========

void MainWindow::checkMultipleConnections()
{
    static QSet<QString> alreadyWarned;  // не спамим предупреждениями

    for (auto it = activeIPsPerClient.begin(); it != activeIPsPerClient.end(); ++it) {
        const QString &cn = it.key();
        const QStringList &ips = it.value();

        if (ips.size() > 1) {
            QString warnKey = cn + ":" + QString::number(ips.size());
            if (!alreadyWarned.contains(warnKey)) {
                alreadyWarned.insert(warnKey);

                QString ipList = ips.join(", ");
                QString warnMsg = QString(
                    "🚨 ВНИМАНИЕ: Клиент <b>%1</b> подключён одновременно с %2 разных IP-адресов!<br>"
                    "IP: %3<br>"
                    "Возможно, файл .ovpn используют несколько человек.")
                .arg(cn).arg(ips.size()).arg(ipList);

                // В журнал клиентов
                QString timestamp = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
                txtClientsLog->append(QString(
                    "<span style='color:red; font-weight:bold;'>[%1] 🚨 МУЛЬТИ-ПОДКЛЮЧЕНИЕ: %2 — %3 IP одновременно: %4</span>")
                .arg(timestamp).arg(cn).arg(ips.size()).arg(ipList));

                // В общий лог
                addLogMessage(QString("⚠️ МУЛЬТИ-ПОДКЛЮЧЕНИЕ: %1 — %2 IP одновременно!").arg(cn).arg(ips.size()), "error");

                // Системное уведомление через трей
                if (trayIcon && trayIcon->isVisible()) {
                    trayIcon->showMessage(
                        "⚠️ Подозрительная активность",
                        QString("Клиент '%1' подключён с %2 разных IP!\n%3")
                        .arg(cn).arg(ips.size()).arg(ipList),
                                          QSystemTrayIcon::Warning,
                                          10000
                    );
                }
            }
        } else {
            // Очищаем предупреждения если вернулся к одному IP
            alreadyWarned.remove(cn + ":2");
            alreadyWarned.remove(cn + ":3");
            alreadyWarned.remove(cn + ":4");
        }
    }
}

// ========== ИСТОРИЯ СЕССИЙ КЛИЕНТА ==========

void MainWindow::showClientSessionHistory()
{
    // Получаем чистый CN — приоритет: реестр (он всегда доступен для неактивных клиентов)
    QString cn;

    auto regSel = registryTable->selectedItems();
    auto actSel = clientsTable->selectedItems();

    if (!regSel.isEmpty()) {
        // Берём чистое имя из UserRole (без emoji-префиксов)
        QTableWidgetItem *item = registryTable->item(regSel.first()->row(), 0);
        cn = item ? item->data(Qt::UserRole).toString() : QString();
    } else if (!actSel.isEmpty()) {
        QTableWidgetItem *item = clientsTable->item(actSel.first()->row(), 0);
        cn = item ? item->data(Qt::UserRole + 1).toString() : QString();
        // Fallback: убираем emoji вручную если UserRole пуст
        if (cn.isEmpty() && item) {
            cn = item->text();
            cn.remove(QRegularExpression("^[^a-zA-Zа-яА-ЯёЁ0-9_\\-]+"));
        }
    }

    if (cn.isEmpty()) {
        QMessageBox::information(this, "История сессий", "Выберите клиента в таблице реестра или активных подключений.");
        return;
    }

    // Копируем данные под мьютексом
    ClientRecord recCopy;
    {
        QMutexLocker locker(&registryMutex);
        if (!clientRegistry.contains(cn)) {
            QMessageBox::information(this, "История сессий",
                                     QString("Клиент '%1' не найден в реестре.\nДанные появятся после первого подключения.").arg(cn));
            return;
        }
        recCopy = clientRegistry[cn];
    }

    // Создаём диалог с таблицей истории
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle("📜 История сессий: " + cn);
    dlg->resize(920, 580);
    QVBoxLayout *layout = new QVBoxLayout(dlg);

    // Заголовок со сводкой
    auto fmtBytes = [](qint64 b) -> QString {
        if (b < 1024) return QString::number(b) + " B";
        if (b < 1024*1024) return QString::number(b/1024) + " KB";
        if (b < 1024LL*1024*1024) return QString::number(b/(1024*1024.0), 'f', 1) + " MB";
        return QString::number(b/(1024*1024*1024.0), 'f', 2) + " GB";
    };
    auto fmtDur = [](qint64 s) -> QString {
        if (s <= 0) return "—";
        if (s < 60) return QString("%1 сек").arg(s);
        if (s < 3600) return QString("%1 мин %2 сек").arg(s/60).arg(s%60);
        return QString("%1 ч %2 мин").arg(s/3600).arg((s%3600)/60);
    };

    QLabel *summary = new QLabel(QString(
        "<b>%1</b> &nbsp;|&nbsp; Сессий: <b>%2</b> &nbsp;|&nbsp; "
        "Суммарно онлайн: <b>%3</b> &nbsp;|&nbsp; "
        "Трафик: <b>↓%4 ↑%5</b> &nbsp;|&nbsp; "
        "Уникальных IP: <b>%6</b>")
    .arg(cn)
    .arg(recCopy.sessions.size())
    .arg(fmtDur(recCopy.totalOnlineSeconds))
    .arg(fmtBytes(recCopy.totalBytesRx))
    .arg(fmtBytes(recCopy.totalBytesTx))
    .arg(recCopy.knownIPs.size()));
    summary->setStyleSheet("padding: 8px; background: #f0f0f0; border-radius: 4px;");
    layout->addWidget(summary);

    // Таблица сессий
    QTableWidget *tbl = new QTableWidget(0, 7, dlg);
    tbl->setHorizontalHeaderLabels({
        "Подключился", "Отключился", "Длительность",
        "IP источника", "VPN IP", "↓ Получено", "↑ Отправлено"
    });
    tbl->horizontalHeader()->setStretchLastSection(true);
    tbl->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tbl->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->setAlternatingRowColors(true);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->setSortingEnabled(true);

    tbl->setRowCount(recCopy.sessions.size());
    for (int i = 0; i < recCopy.sessions.size(); ++i) {
        const SessionRecord &sess = recCopy.sessions[i];

        QString connStr  = sess.connectedAt.isValid()
        ? sess.connectedAt.toString("dd.MM.yyyy HH:mm:ss") : "—";
        QString discStr  = sess.disconnectedAt.isValid()
        ? sess.disconnectedAt.toString("dd.MM.yyyy HH:mm:ss") : "🟢 онлайн";

        tbl->setItem(i, 0, new QTableWidgetItem(connStr));

        QTableWidgetItem *discItem = new QTableWidgetItem(discStr);
        if (!sess.disconnectedAt.isValid()) {
            discItem->setForeground(QColor(0, 150, 0));
            discItem->setFont(QFont("", -1, QFont::Bold));
        }
        tbl->setItem(i, 1, discItem);

        // Длительность: если сессия активна — считаем до сейчас
        qint64 dur = sess.durationSeconds;
        if (!sess.disconnectedAt.isValid() && sess.connectedAt.isValid())
            dur = qMax(0LL, sess.connectedAt.secsTo(QDateTime::currentDateTime()));
        tbl->setItem(i, 2, new QTableWidgetItem(fmtDur(dur)));

        // IP источника (только IP без порта для удобства чтения, полный в tooltip)
        QString shortIP = sess.sourceIP.split(':').first();
        QTableWidgetItem *ipItem = new QTableWidgetItem(shortIP);
        ipItem->setToolTip(sess.sourceIP);  // полный IP:порт в подсказке
        tbl->setItem(i, 3, ipItem);

        tbl->setItem(i, 4, new QTableWidgetItem(sess.vpnIP));
        tbl->setItem(i, 5, new QTableWidgetItem(fmtBytes(sess.bytesReceived)));
        tbl->setItem(i, 6, new QTableWidgetItem(fmtBytes(sess.bytesSent)));

        // Подсвечиваем активные сессии
        if (!sess.disconnectedAt.isValid()) {
            for (int col = 0; col < 7; ++col) {
                if (tbl->item(i, col))
                    tbl->item(i, col)->setBackground(QColor(220, 255, 220));
            }
        }
    }

    layout->addWidget(tbl);

    // Панель фильтра IP
    QGroupBox *ipGroup = new QGroupBox("Все уникальные IP-адреса клиента");
    QVBoxLayout *ipLayout = new QVBoxLayout(ipGroup);
    QTextEdit *ipList = new QTextEdit();
    ipList->setReadOnly(true);
    ipList->setMaximumHeight(80);
    ipList->setFont(QFont("Monospace", 9));

    QStringList sortedIPs = recCopy.knownIPs.values();
    std::sort(sortedIPs.begin(), sortedIPs.end());
    ipList->setPlainText(sortedIPs.join("  |  "));
    ipLayout->addWidget(ipList);
    layout->addWidget(ipGroup);

    // Кнопки
    QHBoxLayout *btnRow = new QHBoxLayout();

    QPushButton *btnExport = new QPushButton("💾 Экспорт в CSV");
    connect(btnExport, &QPushButton::clicked, this, [this, cn, &recCopy, &fmtBytes, &fmtDur]() {
        QString path = QFileDialog::getSaveFileName(this,
                                                    "Экспорт истории " + cn,
                                                    QDir::homePath() + "/" + cn + "_history_" +
                                                    QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".csv",
                                                    "CSV (*.csv)");
        if (path.isEmpty()) return;

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream out(&f);
        out.setEncoding(QStringConverter::Utf8);
        out << "Клиент;Подключился;Отключился;Длительность;IP источника;VPN IP;Получено;Отправлено\n";
        for (const SessionRecord &s : recCopy.sessions) {
            out << cn << ";"
            << (s.connectedAt.isValid() ? s.connectedAt.toString("dd.MM.yyyy HH:mm:ss") : "") << ";"
            << (s.disconnectedAt.isValid() ? s.disconnectedAt.toString("dd.MM.yyyy HH:mm:ss") : "онлайн") << ";"
            << fmtDur(s.durationSeconds) << ";"
            << s.sourceIP << ";"
            << s.vpnIP << ";"
            << fmtBytes(s.bytesReceived) << ";"
            << fmtBytes(s.bytesSent) << "\n";
        }
        f.close();
        QMessageBox::information(this, "Экспорт", "История экспортирована:\n" + path);
    });

    QPushButton *btnClose = new QPushButton("Закрыть");
    connect(btnClose, &QPushButton::clicked, dlg, &QDialog::accept);

    btnRow->addWidget(btnExport);
    btnRow->addStretch();
    btnRow->addWidget(btnClose);
    layout->addLayout(btnRow);

    dlg->exec();
    delete dlg;
}

// Обновление времени онлайн в таблице каждую секунду — без полного перечитывания файла
void MainWindow::updateRealtimeDurations()
{
    if (!clientsTable || clientsTable->rowCount() == 0) return;

    auto fmtDuration = [](qint64 secs) -> QString {
        if (secs < 0) secs = 0;
        if (secs < 60) return QString("%1 сек").arg(secs);
        if (secs < 3600) return QString("%1 мин %2 сек").arg(secs/60).arg(secs%60);
        return QString("%1 ч %2 мин").arg(secs/3600).arg((secs%3600)/60);
    };

    QDateTime now = QDateTime::currentDateTime();

    for (int row = 0; row < clientsTable->rowCount(); ++row) {
        QTableWidgetItem *nameItem = clientsTable->item(row, 0);
        if (!nameItem) continue;

        qint64 epoch = nameItem->data(Qt::UserRole + 2).toLongLong();
        if (epoch <= 0) continue;

        QDateTime connectedSince = QDateTime::fromSecsSinceEpoch(epoch);
        qint64 onlineSecs = connectedSince.secsTo(now);
        if (onlineSecs < 0) onlineSecs = 0;

        QString connTime = connectedSince.toString("HH:mm dd.MM");
        QString activeStr = connTime + "  (" + fmtDuration(onlineSecs) + ")";

        QTableWidgetItem *timeItem = clientsTable->item(row, 4);
        if (timeItem) {
            timeItem->setText(activeStr);
            // Обновляем цвет по длительности
            if (onlineSecs > 3600)
                timeItem->setForeground(QColor(0, 100, 180));
            else if (onlineSecs > 300)
                timeItem->setForeground(QColor(0, 150, 0));
            else
                timeItem->setForeground(QApplication::palette().text().color());
        }
    }
}

// ========== РЕЕСТР КЛИЕНТОВ ==========

void MainWindow::loadClientRegistry()
{
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    // --- 1. Загружаем метаданные из INI ---
    QString iniPath = appData + "/client_registry.ini";
    QSettings reg(iniPath, QSettings::IniFormat);

    clientRegistry.clear();
    for (const QString &groupKey : reg.childGroups()) {
        reg.beginGroup(groupKey);
        ClientRecord rec;
        // Оригинальное имя (на случай если в нём были спецсимволы)
        QString cn = reg.value("originalName", groupKey).toString();

        rec.displayName        = reg.value("displayName", cn).toString();
        rec.expiryDate         = QDate::fromString(reg.value("expiryDate").toString(), "yyyy-MM-dd");
        rec.firstSeen          = QDateTime::fromString(reg.value("firstSeen").toString(), Qt::ISODate);
        rec.lastSeen           = QDateTime::fromString(reg.value("lastSeen").toString(), Qt::ISODate);
        rec.totalBytesRx      = reg.value("totalBytesRx", 0).toLongLong();
        rec.totalBytesTx      = reg.value("totalBytesTx", 0).toLongLong();
        rec.sessionCount       = reg.value("sessionCount", 0).toInt();
        rec.totalOnlineSeconds = reg.value("totalOnlineSecs", 0).toLongLong();
        rec.isBanned           = reg.value("isBanned", false).toBool();
        rec.speedLimitKbps     = reg.value("speedLimitKbps", 0).toInt();

        QStringList ipList = reg.value("knownIPs").toStringList();
        rec.knownIPs = QSet<QString>(ipList.cbegin(), ipList.cend());

        reg.endGroup();

        // --- 2. Загружаем историю сессий из JSON ---
        QString safeKey = QString(cn).replace('/', '_').replace('\\', '_').replace(' ', '_');
        QString jsonPath = appData + "/sessions/" + safeKey + ".json";

        QFile f(jsonPath);
        if (f.open(QIODevice::ReadOnly)) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
            f.close();

            if (err.error == QJsonParseError::NoError && doc.isArray()) {
                QJsonArray arr = doc.array();
                for (const QJsonValue &val : arr) {
                    if (!val.isObject()) continue;
                    QJsonObject obj = val.toObject();
                    SessionRecord s;
                    s.connectedAt    = QDateTime::fromString(obj["connectedAt"].toString(), Qt::ISODate);
                    QString discStr  = obj["disconnectedAt"].toString();
                    if (!discStr.isEmpty())
                        s.disconnectedAt = QDateTime::fromString(discStr, Qt::ISODate);
                    s.sourceIP       = obj["sourceIP"].toString();
                    s.vpnIP          = obj["vpnIP"].toString();
                    s.durationSeconds= (qint64)obj["durationSecs"].toDouble();
                    s.bytesReceived  = (qint64)obj["bytesRx"].toDouble();
                    s.bytesSent      = (qint64)obj["bytesTx"].toDouble();
                    rec.sessions.append(s);
                }
            }
        }

        clientRegistry[cn] = rec;
    }

    updateRegistryTable();
}

void MainWindow::saveClientRegistry()
{
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    // Сохраняем в INI
    QString iniPath = appData + "/client_registry.ini";
    QSettings reg(iniPath, QSettings::IniFormat);
    reg.clear();

    {
        QMutexLocker locker(&registryMutex);

        for (auto it = clientRegistry.cbegin(); it != clientRegistry.cend(); ++it) {
            const ClientRecord &rec = it.value();
            QString safeKey = QString(it.key()).replace('/', '_').replace('\\', '_');

            reg.beginGroup(safeKey);
            reg.setValue("originalName", it.key());
            reg.setValue("displayName", rec.displayName);
            reg.setValue("telegramId", rec.telegramId);
            reg.setValue("registeredAt", rec.registeredAt.toString("yyyy-MM-dd"));
            reg.setValue("expiryDate", rec.expiryDate.toString("yyyy-MM-dd"));
            reg.setValue("firstSeen", rec.firstSeen.toString(Qt::ISODate));
            reg.setValue("lastSeen", rec.lastSeen.toString(Qt::ISODate));
            reg.setValue("totalBytesRx", rec.totalBytesRx);
            reg.setValue("totalBytesTx", rec.totalBytesTx);
            reg.setValue("sessionCount", rec.sessionCount);
            reg.setValue("totalOnlineSecs", rec.totalOnlineSeconds);
            reg.setValue("isBanned", rec.isBanned);
            reg.setValue("speedLimitKbps", rec.speedLimitKbps);
            reg.setValue("knownIPs", QStringList(rec.knownIPs.values()));
            reg.endGroup();
        }
    }

    reg.sync();

    // Сохраняем историю сессий в JSON (как и раньше)
    QString sessDir = appData + "/sessions";
    QDir().mkpath(sessDir);

    {
        QMutexLocker locker(&registryMutex);

        for (auto it = clientRegistry.cbegin(); it != clientRegistry.cend(); ++it) {
            const ClientRecord &rec = it.value();
            QString safeKey = QString(it.key()).replace('/', '_').replace('\\', '_').replace(' ', '_');
            QString jsonPath = sessDir + "/" + safeKey + ".json";

            QJsonArray arr;
            for (const SessionRecord &s : rec.sessions) {
                QJsonObject obj;
                obj["connectedAt"] = s.connectedAt.toString(Qt::ISODate);
                obj["disconnectedAt"] = s.disconnectedAt.isValid()
                ? s.disconnectedAt.toString(Qt::ISODate) : QString();
                obj["sourceIP"] = s.sourceIP;
                obj["vpnIP"] = s.vpnIP;
                obj["durationSecs"] = s.durationSeconds;
                obj["bytesRx"] = s.bytesReceived;
                obj["bytesTx"] = s.bytesSent;
                arr.append(obj);
            }

            QFile f(jsonPath);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
                f.close();
            }
        }
    }

    // Уведомляем подписчиков
    emit registryUpdated(clientRegistry);
}

void MainWindow::updateRegistryTable()
{
    if (!registryTable) return;

    // --- Запоминаем текущий выбор и позицию скролла ---
    QString selectedCN;
    if (!registryTable->selectedItems().isEmpty()) {
        QTableWidgetItem *sel = registryTable->item(registryTable->selectedItems().first()->row(), 0);
        if (sel) selectedCN = sel->data(Qt::UserRole).toString();
    }
    int scrollPos = registryTable->verticalScrollBar()
    ? registryTable->verticalScrollBar()->value() : 0;

    auto fmtBytes = [](qint64 b) -> QString {
        if (b < 1024) return QString::number(b) + " B";
        if (b < 1024*1024) return QString::number(b/1024) + " KB";
        if (b < 1024LL*1024*1024) return QString::number(b/(1024*1024.0), 'f', 1) + " MB";
        return QString::number(b/(1024*1024*1024.0), 'f', 2) + " GB";
    };

    // Обновляем заголовки
    registryTable->setColumnCount(11);
    registryTable->setHorizontalHeaderLabels({
        "Клиент", "VPN IP", "Статус",
        "↓ Скорость", "↑ Скорость",
        "Трафик ↓/↑", "Шифрование",
        "Истекает", "⚡ Лимит", "Сессий", "Последнее подкл."
    });

    // Блокируем сортировку во время заполнения (но НЕ сигналы — иначе кнопки перестают реагировать)
    registryTable->setSortingEnabled(false);
    registryTable->setRowCount(clientRegistry.size());

    int row = 0;
    QDate today = QDate::currentDate();

    // Вспомогательная функция форматирования скорости
    auto fmtSpeed = [](qint64 bps) -> QString {
        if (bps <= 0) return "—";
        if (bps < 1024)        return QString("%1 Б/с").arg(bps);
        if (bps < 1024*1024)   return QString("%1 КБ/с").arg(bps/1024.0, 0, 'f', 1);
        return QString("%1 МБ/с").arg(bps/(1024.0*1024.0), 0, 'f', 2);
    };

    for (auto it = clientRegistry.cbegin(); it != clientRegistry.cend(); ++it) {
        const ClientRecord &rec = it.value();
        const QString &cn = it.key();

        bool isOnline = activeIPsPerClient.contains(cn);
        bool isMulti  = activeIPsPerClient.value(cn).size() > 1;

        // Получаем живую информацию из кэша активных клиентов
        ClientInfo liveInfo;
        bool hasLive = false;
        for (const auto &ci : std::as_const(clientsCache)) {
            if (ci.commonName == cn) { liveInfo = ci; hasLive = true; break; }
        }

        // col 0 — Имя
        QString prefix = rec.isBanned ? "🚫 " : (isMulti ? "⚠️ " : (isOnline ? "🟢 " : ""));
        QTableWidgetItem *nameItem = new QTableWidgetItem(prefix + cn);
        nameItem->setData(Qt::UserRole, cn);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        if (rec.isBanned)       nameItem->setForeground(QColor(200,0,0));
        else if (isMulti)       nameItem->setForeground(QColor(180,60,0));
        else if (isOnline)      nameItem->setForeground(QColor(0,140,0));
        nameItem->setFont(QFont("", -1, QFont::Bold));
        registryTable->setItem(row, 0, nameItem);

        // col 1 — VPN IP
        QString vpnIP = hasLive ? liveInfo.virtualAddress : "—";
        auto *vpnItem = new QTableWidgetItem(vpnIP);
        vpnItem->setFlags(vpnItem->flags() & ~Qt::ItemIsEditable);
        vpnItem->setTextAlignment(Qt::AlignCenter);
        registryTable->setItem(row, 1, vpnItem);

        // col 2 — Статус
        QString statusStr;
        QColor statusColor;
        if (rec.isBanned) { statusStr = "🚫 Заблок."; statusColor = QColor(200,0,0); }
        else if (isMulti) { statusStr = "⚠️ Мульти";  statusColor = QColor(180,80,0); }
        else if (isOnline){ statusStr = "🟢 Онлайн"; statusColor = QColor(0,130,0); }
        else {
            bool expired = rec.expiryDate.isValid() && rec.expiryDate < today;
            statusStr = expired ? "❌ Истёк" : "⚫ Офлайн";
            statusColor = expired ? QColor(180,0,0) : QColor(100,100,100);
        }
        auto *stItem = new QTableWidgetItem(statusStr);
        stItem->setForeground(statusColor);
        stItem->setTextAlignment(Qt::AlignCenter);
        stItem->setFlags(stItem->flags() & ~Qt::ItemIsEditable);
        registryTable->setItem(row, 2, stItem);

        // col 3 — ↓ Скорость (мгновенная)
        QString rxSpeedStr = hasLive && isOnline ? fmtSpeed(liveInfo.speedRxBps) : "—";
        auto *rxSpeedItem = new QTableWidgetItem(rxSpeedStr);
        rxSpeedItem->setTextAlignment(Qt::AlignCenter);
        rxSpeedItem->setFlags(rxSpeedItem->flags() & ~Qt::ItemIsEditable);
        if (hasLive && isOnline && liveInfo.speedRxBps > 0)
            rxSpeedItem->setForeground(QColor(0,120,0));
        registryTable->setItem(row, 3, rxSpeedItem);

        // col 4 — ↑ Скорость (мгновенная)
        QString txSpeedStr = hasLive && isOnline ? fmtSpeed(liveInfo.speedTxBps) : "—";
        auto *txSpeedItem = new QTableWidgetItem(txSpeedStr);
        txSpeedItem->setTextAlignment(Qt::AlignCenter);
        txSpeedItem->setFlags(txSpeedItem->flags() & ~Qt::ItemIsEditable);
        if (hasLive && isOnline && liveInfo.speedTxBps > 0)
            txSpeedItem->setForeground(QColor(0,80,160));
        registryTable->setItem(row, 4, txSpeedItem);

        // col 5 — Суммарный трафик
        QString trafficStr = QString("↓%1  ↑%2")
        .arg(fmtBytes(rec.totalBytesRx)).arg(fmtBytes(rec.totalBytesTx));
        auto *trafficItem = new QTableWidgetItem(trafficStr);
        trafficItem->setFlags(trafficItem->flags() & ~Qt::ItemIsEditable);
        registryTable->setItem(row, 5, trafficItem);

        // col 6 — Шифрование
        QString cipherStr = "AES-256-CBC";
        {
            QString ovpnPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ovpn/" + cn + ".ovpn";
            QFile ovpnFile(ovpnPath);
            if (ovpnFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString oc = QString::fromUtf8(ovpnFile.readAll());
                QRegularExpression re("^cipher\\s+(\\S+)", QRegularExpression::MultilineOption);
                auto m = re.match(oc);
                if (m.hasMatch()) cipherStr = m.captured(1);
                else {
                    QRegularExpression re2("^data-ciphers\\s+(\\S+)", QRegularExpression::MultilineOption);
                    auto m2 = re2.match(oc);
                    if (m2.hasMatch()) cipherStr = m2.captured(1).split(':').first();
                }
            }
        }
        auto *cipherItem = new QTableWidgetItem(cipherStr);
        cipherItem->setTextAlignment(Qt::AlignCenter);
        cipherItem->setFlags(cipherItem->flags() & ~Qt::ItemIsEditable);
        registryTable->setItem(row, 6, cipherItem);

        // col 7 — Истекает
        QTableWidgetItem *expiryItem;
        if (rec.expiryDate.isValid()) {
            int daysLeft = today.daysTo(rec.expiryDate);
            QString exStr = rec.expiryDate.toString("dd.MM.yyyy");
            if (daysLeft < 0)     { expiryItem = new QTableWidgetItem("❌ " + exStr); expiryItem->setBackground(QColor(255,150,150)); }
            else if (daysLeft<=7) { expiryItem = new QTableWidgetItem(QString("⚠️ %1 (%2д.)").arg(exStr).arg(daysLeft)); expiryItem->setBackground(QColor(255,230,150)); }
            else                  { expiryItem = new QTableWidgetItem("✅ " + exStr); expiryItem->setBackground(QColor(220,255,220)); }
        } else {
            expiryItem = new QTableWidgetItem("∞");
        }
        expiryItem->setFlags(expiryItem->flags() & ~Qt::ItemIsEditable);
        registryTable->setItem(row, 7, expiryItem);

        // col 8 — Лимит скорости
        QString limitStr = rec.speedLimitKbps > 0
        ? QString("⚡ %1 КБит/с").arg(rec.speedLimitKbps)
        : "∞";
        auto *limitItem = new QTableWidgetItem(limitStr);
        limitItem->setTextAlignment(Qt::AlignCenter);
        limitItem->setFlags(limitItem->flags() & ~Qt::ItemIsEditable);
        if (rec.speedLimitKbps > 0)
            limitItem->setForeground(QColor(180, 80, 0));
        else
            limitItem->setForeground(QColor(100, 100, 100));
        registryTable->setItem(row, 8, limitItem);

        // col 9 — Сессий
        auto *sessItem = new QTableWidgetItem(QString::number(rec.sessionCount));
        sessItem->setTextAlignment(Qt::AlignCenter);
        sessItem->setFlags(sessItem->flags() & ~Qt::ItemIsEditable);
        registryTable->setItem(row, 9, sessItem);

        // col 10 — Последнее подкл.
        auto *lastItem = new QTableWidgetItem(
            rec.lastSeen.isValid() ? rec.lastSeen.toString("dd.MM.yyyy HH:mm") : "Никогда");
        lastItem->setFlags(lastItem->flags() & ~Qt::ItemIsEditable);
        registryTable->setItem(row, 10, lastItem);

        // Подсветка мульти-IP строки
        if (isMulti) {
            for (int col = 0; col < 11; ++col)
                if (auto *it2 = registryTable->item(row, col))
                    it2->setBackground(QColor(255,230,200));
        }

        row++;
    }

    registryTable->setSortingEnabled(true);

    // --- Восстанавливаем выделение и скролл ---
    // Строку ищем по имени клиента (Qt::UserRole), а не по индексу —
    // после setSortingEnabled(true) таблица пересортирована.

    if (!selectedCN.isEmpty()) {
        for (int r = 0; r < registryTable->rowCount(); ++r) {
            QTableWidgetItem *it2 = registryTable->item(r, 0);
            if (it2 && it2->data(Qt::UserRole).toString() == selectedCN) {
                registryTable->selectRow(r);
                break;
            }
        }
    }

    if (registryTable->verticalScrollBar())
        registryTable->verticalScrollBar()->setValue(scrollPos);

    lblTotalClients->setText(QString("Зарегистрировано: <b>%1</b>").arg(clientRegistry.size()));

    // Обновляем счётчик онлайн-клиентов
    if (lblOnlineClients) {
        int onlineCount = 0, bannedCount = 0;
        for (auto it = clientRegistry.cbegin(); it != clientRegistry.cend(); ++it) {
            if (activeIPsPerClient.contains(it.key())) ++onlineCount;
            if (it.value().isBanned) ++bannedCount;
        }
        lblOnlineClients->setText(QString("Онлайн: <b style='color:green;'>%1</b>%2")
        .arg(onlineCount)
        .arg(bannedCount > 0
        ? QString("  🚫 <b style='color:red;'>%1</b>").arg(bannedCount)
        : QString()));
    }

    // Применяем текущий поиск/фильтр (не сбрасываем при обновлении)
    applyRegistryFilter();
}

// --- Фильтрация строк реестра по поиску и статусу ---
void MainWindow::applyRegistryFilter()
{
    if (!registryTable || !regSearchEdit || !regFilterCombo) return;

    QString search = regSearchEdit->text().trimmed().toLower();
    int filterIdx  = regFilterCombo->currentIndex(); // 0=все, 1=онлайн, 2=офлайн, 3=заблок.

    for (int r = 0; r < registryTable->rowCount(); ++r) {
        QTableWidgetItem *nameItem = registryTable->item(r, 0);
        if (!nameItem) { registryTable->setRowHidden(r, false); continue; }

        QString cn = nameItem->data(Qt::UserRole).toString().toLower();
        bool matchSearch = search.isEmpty() || cn.contains(search);

        bool isOnline  = activeIPsPerClient.contains(nameItem->data(Qt::UserRole).toString());
        bool isBanned  = clientRegistry.value(nameItem->data(Qt::UserRole).toString()).isBanned;

        bool matchFilter = true;
        if      (filterIdx == 1) matchFilter = isOnline;
        else if (filterIdx == 2) matchFilter = !isOnline;
        else if (filterIdx == 3) matchFilter = isBanned;

        registryTable->setRowHidden(r, !(matchSearch && matchFilter));
    }
}

// ========== ИМЕННОЙ .ovpn С ДАТОЙ ИСТЕЧЕНИЯ ==========

void MainWindow::generateNamedClientConfig()
{
    // Диалог: имя клиента
    QString rawName = QInputDialog::getText(this, "Новый клиент",
                                            "Имя клиента (латиница, кириллица — транслитерируется автоматически):",
                                            QLineEdit::Normal, "");
    if (rawName.isEmpty()) return;

    // Транслитерация и очистка имени (CN сертификата поддерживает только ASCII)
    QString clientName = sanitizeClientName(rawName);
    if (clientName.length() < 2) {
        QMessageBox::warning(this, "Ошибка",
                             "Имя клиента слишком короткое или содержит только спецсимволы.\n"
                             "Введённое: \"" + rawName + "\"\n"
                             "Убедитесь, что имя содержит буквы.");
        return;
    }

    // Если имя изменилось после очистки — сообщаем пользователю
    if (clientName != rawName) {
        auto ans = QMessageBox::question(this, "Имя изменено",
                                         "Введённое имя транслитерировано:\n"
                                         "  Исходное: " + rawName + "\n"
                                         "  Итоговое: " + clientName + "\n\n"
                                         "Продолжить с именем «" + clientName + "»?",
                                         QMessageBox::Yes | QMessageBox::Cancel);
        if (ans != QMessageBox::Yes) return;
    }

    // Диалог: дата истечения
    bool hasExpiry = (QMessageBox::question(this, "Срок действия",
                                            "Установить срок действия доступа?",
                                            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes);

    QDate expiryDate;
    if (hasExpiry) {
        // Простой диалог выбора даты
        QDialog dateDialog(this);
        dateDialog.setWindowTitle("Дата истечения доступа");
        QVBoxLayout *dl = new QVBoxLayout(&dateDialog);

        QCalendarWidget *cal = new QCalendarWidget(&dateDialog);
        cal->setMinimumDate(QDate::currentDate().addDays(1));
        cal->setSelectedDate(QDate::currentDate().addMonths(1));
        dl->addWidget(cal);

        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(bb, &QDialogButtonBox::accepted, &dateDialog, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dateDialog, &QDialog::reject);
        dl->addWidget(bb);

        if (dateDialog.exec() != QDialog::Accepted) return;
        expiryDate = cal->selectedDate();
    }

    QString savePath = QFileDialog::getSaveFileName(this, "Сохранить конфигурацию клиента",
                                                    QDir::homePath() + "/" + clientName + ".ovpn",
                                                    "OpenVPN Config (*.ovpn)");
    if (savePath.isEmpty()) return;
    resolveTaKeyPath(); // Найти ta.key в certsDir или easy-rsa/pki/

    // Проверяем/создаем сертификаты для клиента
    // Используем resolveClientCertPath — ищем в certsDir и easy-rsa/pki/
    QString clientCertFile = resolveClientCertPath(clientName);
    QString clientKeyFile  = resolveClientKeyPath(clientName);

    if (!QFile::exists(clientCertFile) || !QFile::exists(clientKeyFile)) {
        addLogMessage("Создание сертификатов для клиента: " + clientName, "info");

        // Показываем прогресс
        QProgressDialog progress("Создание сертификата для " + clientName + "...", "Отмена", 0, 0, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(0);
        progress.show();

        // Пробуем через EasyRSA (блокирующий вариант для надёжности)
        QString easyRSAPath = findEasyRSA();
        bool certCreated = false;

        if (!easyRSAPath.isEmpty()) {
            QString workDir = certsDir + "/easy-rsa";
            QDir().mkpath(workDir);

            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            env.insert("EASYRSA_BATCH", "1");

            QProcess proc;
            proc.setWorkingDirectory(workDir);
            proc.setProcessEnvironment(env);
            proc.start(easyRSAPath, QStringList() << "build-client-full" << clientName << "nopass");
            proc.waitForFinished(30000);

            if (proc.exitCode() == 0) {
                QString pkiDir = certsDir + "/easy-rsa/pki";
                QFile::copy(pkiDir + "/issued/" + clientName + ".crt", clientCertFile);
                QFile::copy(pkiDir + "/private/" + clientName + ".key", clientKeyFile);
                certCreated = QFile::exists(clientCertFile) && QFile::exists(clientKeyFile);
            }
        }

        if (!certCreated) {
            // Fallback: OpenSSL
            QProcess genKey;
            genKey.setWorkingDirectory(certsDir);
            genKey.start("openssl", QStringList() << "genrsa" << "-out" << clientKeyFile << "2048");
            genKey.waitForFinished(15000);

            if (genKey.exitCode() == 0) {
                QProcess genCsr;
                genCsr.setWorkingDirectory(certsDir);
                genCsr.start("openssl", QStringList()
                << "req" << "-new"
                << "-key" << clientKeyFile
                << "-out" << (certsDir + "/" + clientName + ".csr")
                << "-subj" << ("/C=RU/ST=Moscow/L=Moscow/O=TorManager/CN=" + clientName));
                genCsr.waitForFinished(10000);

                if (genCsr.exitCode() == 0) {
                    QProcess signCert;
                    signCert.setWorkingDirectory(certsDir);
                    signCert.start("openssl", QStringList()
                    << "x509" << "-req"
                    << "-in" << (certsDir + "/" + clientName + ".csr")
                    << "-CA" << caCertPath
                    << "-CAkey" << (certsDir + "/ca.key")
                    << "-CAcreateserial"
                    << "-out" << clientCertFile
                    << "-days" << "3650"
                    << "-outform" << "PEM");
                    signCert.waitForFinished(10000);
                    certCreated = signCert.exitCode() == 0 && QFile::exists(clientCertFile);
                }
            }
        }

        progress.close();

        if (!certCreated) {
            QMessageBox::critical(this, "Ошибка", "Не удалось создать сертификат для клиента '" + clientName + "'.\n"
            "Убедитесь что сертификаты сервера сгенерированы.");
            return;
        }
        addLogMessage("Сертификаты для клиента " + clientName + " созданы", "success");
    }

    // Формируем .ovpn
    QString serverAddress = "wwcat.duckdns.org";
    // Используем адрес из настроек, если задан
    if (txtServerAddress && !txtServerAddress->text().trimmed().isEmpty())
        serverAddress = txtServerAddress->text().trimmed();
    int serverPort = spinServerPort->value();

    QString config;
    config += "# OpenVPN Client Configuration\n";
    config += "# Клиент: " + clientName + "\n";
    config += "# Создан: " + QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss") + "\n";
    if (expiryDate.isValid())
        config += "# Действителен до: " + expiryDate.toString("dd.MM.yyyy") + "\n";
    else
        config += "# Срок действия: без ограничений\n";
    config += "# Создан через: Tor Manager\n\n";

    config += "client\n";
    config += "dev tun\n";
    config += "proto tcp\n";
    config += "remote " + serverAddress + " " + QString::number(serverPort) + "\n";
    config += "resolv-retry infinite\n";
    config += "nobind\n";
    config += "persist-key\n";
    config += "persist-tun\n\n";

    config += "tls-client\n";
    config += "remote-cert-tls server\n";
    config += "tls-version-min 1.2\n";
    config += "data-ciphers AES-256-GCM:AES-128-GCM:CHACHA20-POLY1305:AES-256-CBC\n";
    config += "data-ciphers-fallback AES-256-CBC\n";
    config += "auth SHA256\n";
    config += "auth-nocache\n";
    config += "allow-compression no\n\n";

    // CA сертификат
    if (QFile::exists(caCertPath)) {
        config += "<ca>\n";
        QFile caFile(caCertPath);
        if (caFile.open(QIODevice::ReadOnly | QIODevice::Text)) { config += caFile.readAll(); caFile.close(); }
        config += "</ca>\n\n";
    }

    // Клиентский сертификат
    if (QFile::exists(clientCertFile)) {
        config += "<cert>\n";
        QFile certFile(clientCertFile);
        if (certFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString certContent = certFile.readAll();
            certFile.close();
            QRegularExpression pemRegex("-----BEGIN CERTIFICATE-----[\\s\\S]*?-----END CERTIFICATE-----");
            QRegularExpressionMatch m = pemRegex.match(certContent);
            config += (m.hasMatch() ? m.captured(0) : certContent) + "\n";
        }
        config += "</cert>\n\n";
    } else {
        QMessageBox::critical(this, "Ошибка", "Клиентский сертификат не найден: " + clientCertFile);
        return;
    }

    // Клиентский ключ
    if (QFile::exists(clientKeyFile)) {
        config += "<key>\n";
        QFile keyFile(clientKeyFile);
        if (keyFile.open(QIODevice::ReadOnly | QIODevice::Text)) { config += keyFile.readAll(); keyFile.close(); }
        config += "</key>\n\n";
    } else {
        QMessageBox::critical(this, "Ошибка", "Ключ клиента не найден: " + clientKeyFile);
        return;
    }

    // tls-crypt — КРИТИЧНО: должен совпадать с ключом в server.conf
    // Сначала извлекаем из server.conf (гарантированное совпадение),
    // иначе читаем файл (может не совпасть → "tls-crypt unwrap error")
    {
        QString taKeyContent = extractTaKeyFromServerConf();
        if (!taKeyContent.isEmpty()) {
            config += "<tls-crypt>\n";
            config += taKeyContent + "\n";
            config += "</tls-crypt>\n\n";
            addLogMessage("✅ ta.key извлечён из server.conf (гарантированное совпадение)", "success");
        } else {
            // Fallback: читаем из файла
            QString resolvedTaKey = resolveTaKeyPath();
            if (QFile::exists(resolvedTaKey)) {
                config += "<tls-crypt>\n";
                QFile taFile(resolvedTaKey);
                if (taFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    config += taFile.readAll();
                    taFile.close();
                }
                config += "</tls-crypt>\n\n";
                addLogMessage("⚠️ ta.key из файла (убедитесь что сервер использует тот же ключ): " + resolvedTaKey, "warning");
            } else {
                addLogMessage("❌ ta.key не найден — конфиг создан без tls-crypt! Подключение не будет работать.", "error");
            }
        }
    }

    config += "\nverb 3\nmute 10\n";

    QFile file(savePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(config.toUtf8());
        file.close();

        // Регистрируем клиента (под мьютексом)
        {
            QMutexLocker locker(&registryMutex);
            if (!clientRegistry.contains(clientName)) {
                ClientRecord rec;
                rec.displayName = clientName;
                rec.expiryDate  = expiryDate;
                rec.firstSeen   = QDateTime();
                rec.lastSeen    = QDateTime();
                rec.sessionCount = 0;
                rec.totalBytesRx = 0;
                rec.totalBytesTx = 0;
                rec.totalOnlineSeconds = 0;
                rec.isBanned = false;
                clientRegistry[clientName] = rec;
            } else {
                clientRegistry[clientName].expiryDate = expiryDate;
            }
        }
        saveClientRegistry();
        updateRegistryTable();

        QString expiryMsg = expiryDate.isValid()
        ? "\nДействителен до: " + expiryDate.toString("dd.MM.yyyy")
        : "\nСрок действия: без ограничений";

        addLogMessage("Именной конфиг создан: " + clientName + " → " + savePath, "success");
        QMessageBox::information(this, "Клиент создан",
                                 "Конфиг для клиента '" + clientName + "' создан.\n"
                                 "Файл: " + savePath + expiryMsg + "\n\n"
                                 "Отслеживание подключений ведётся автоматически.");
    } else {
        addLogMessage("Ошибка сохранения конфига: " + savePath, "error");
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл конфигурации");
    }
}

// ========== ДЕТАЛЬНАЯ АНАЛИТИКА ПО КЛИЕНТУ ==========

void MainWindow::showClientAnalytics()
{
    QList<QTableWidgetItem*> selected = clientsTable->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    QString cn = clientsTable->item(row, 0)->data(Qt::UserRole + 1).toString();
    if (cn.isEmpty()) cn = clientsTable->item(row, 0)->text().remove(QRegularExpression("^[🟢🔴🚫]\\s"));

    auto fmtBytes = [](qint64 b) -> QString {
        if (b < 1024) return QString::number(b) + " B";
        if (b < 1024*1024) return QString::number(b/1024) + " KB";
        if (b < 1024LL*1024*1024) return QString::number(b/(1024*1024.0), 'f', 1) + " MB";
        return QString::number(b/(1024*1024*1024.0), 'f', 2) + " GB";
    };

    auto fmtDuration = [](qint64 secs) -> QString {
        if (secs < 0) secs = 0;
        if (secs < 60) return QString("%1 сек").arg(secs);
        if (secs < 3600) return QString("%1 мин %2 сек").arg(secs/60).arg(secs%60);
        if (secs < 86400) return QString("%1 ч %2 мин").arg(secs/3600).arg((secs%3600)/60);
        return QString("%1 дн. %2 ч %3 мин").arg(secs/86400).arg((secs%86400)/3600).arg((secs%3600)/60);
    };

    // Текущая сессия
    QString sessionInfo;
    if (clientsCache.contains(cn)) {
        const ClientInfo &cur = clientsCache[cn];
        QDateTime now = QDateTime::currentDateTime();
        qint64 onlineSecs = 0;
        if (cur.connectedSince.isValid()) {
            onlineSecs = cur.connectedSince.secsTo(now);
            if (onlineSecs < 0) onlineSecs = 0;
        }
        sessionInfo = QString(
            "<tr><td colspan='2'><b>— Текущая сессия —</b></td></tr>"
            "<tr><td>VPN адрес:</td><td><b>%1</b></td></tr>"
            "<tr><td>Подключён с:</td><td>%2</td></tr>"
            "<tr><td>Длительность:</td><td><b style='color:green;'>%3</b></td></tr>"
            "<tr><td>↓ Получено (сессия):</td><td>%4</td></tr>"
            "<tr><td>↑ Отправлено (сессия):</td><td>%5</td></tr>"
        ).arg(cur.virtualAddress)
        .arg(cur.connectedSince.isValid() ? cur.connectedSince.toString("dd.MM.yyyy HH:mm:ss") : "—")
        .arg(fmtDuration(onlineSecs))
        .arg(fmtBytes(cur.bytesReceived))
        .arg(fmtBytes(cur.bytesSent));
    } else {
        sessionInfo = "<tr><td colspan='2'><i style='color:gray;'>Клиент сейчас не подключён</i></td></tr>";
    }

    // Копируем данные из реестра под мьютексом
    ClientRecord recCopy;
    bool hasRec = false;
    {
        QMutexLocker locker(&registryMutex);
        if (clientRegistry.contains(cn)) {
            recCopy = clientRegistry[cn];
            hasRec = true;
        }
    }

    // Исторические данные
    QString historyInfo;
    QDate today = QDate::currentDate();
    if (hasRec) {
        const ClientRecord &rec = recCopy;

        QString expiryStr;
        if (rec.expiryDate.isValid()) {
            int daysLeft = today.daysTo(rec.expiryDate);
            if (daysLeft < 0)
                expiryStr = "<span style='color:red;'>❌ Истёк " + rec.expiryDate.toString("dd.MM.yyyy") + "</span>";
            else if (daysLeft <= 7)
                expiryStr = "<span style='color:orange;'>⚠️ " + rec.expiryDate.toString("dd.MM.yyyy") + " (осталось " + QString::number(daysLeft) + " дн.)</span>";
            else
                expiryStr = "<span style='color:green;'>✅ " + rec.expiryDate.toString("dd.MM.yyyy") + " (осталось " + QString::number(daysLeft) + " дн.)</span>";
        } else {
            expiryStr = "Без ограничений";
        }

        double avgSessionMin = rec.sessionCount > 0 ? (rec.totalOnlineSeconds / 60.0 / rec.sessionCount) : 0;

        historyInfo = QString(
            "<tr><td colspan='2'><b>— Статистика клиента —</b></td></tr>"
            "<tr><td>Доступ действителен до:</td><td>%1</td></tr>"
            "<tr><td>Первое подключение:</td><td>%2</td></tr>"
            "<tr><td>Последнее подключение:</td><td>%3</td></tr>"
            "<tr><td>Всего сессий:</td><td>%4</td></tr>"
            "<tr><td>Суммарно онлайн:</td><td>%5</td></tr>"
            "<tr><td>Средняя сессия:</td><td>%6 мин</td></tr>"
            "<tr><td>↓ Получено (всего):</td><td>%7</td></tr>"
            "<tr><td>↑ Отправлено (всего):</td><td>%8</td></tr>"
            "<tr><td>Суммарный трафик:</td><td>%9</td></tr>"
        ).arg(expiryStr)
        .arg(rec.firstSeen.isValid() ? rec.firstSeen.toString("dd.MM.yyyy HH:mm") : "—")
        .arg(rec.lastSeen.isValid() ? rec.lastSeen.toString("dd.MM.yyyy HH:mm") : "—")
        .arg(rec.sessionCount)
        .arg(fmtDuration(rec.totalOnlineSeconds))
        .arg(QString::number(avgSessionMin, 'f', 1))
        .arg(fmtBytes(rec.totalBytesRx))
        .arg(fmtBytes(rec.totalBytesTx))
        .arg(fmtBytes(rec.totalBytesRx + rec.totalBytesTx));
    } else {
        historyInfo = "<tr><td colspan='2'><i>Нет исторических данных</i></td></tr>";
    }

    QString html = QString(
        "<h3>📊 Аналитика клиента: <b>%1</b></h3>"
        "<table cellspacing='4' width='100%%'>"
        "%2"
        "<tr><td colspan='2'>&nbsp;</td></tr>"
        "%3"
        "</table>"
    ).arg(cn).arg(sessionInfo).arg(historyInfo);

    QMessageBox *dlg = new QMessageBox(this);
    dlg->setWindowTitle("Аналитика клиента: " + cn);
    dlg->setTextFormat(Qt::RichText);
    dlg->setText(html);
    dlg->setStandardButtons(QMessageBox::Ok);
    dlg->exec();
    delete dlg;
}

void MainWindow::disconnectSelectedClient()
{
    QList<QTableWidgetItem*> selected = clientsTable->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    QString cn       = clientsTable->item(row, 0)->text();
    QString realAddr = clientsTable->item(row, 0)->data(Qt::UserRole).toString(); // IP только здесь

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Отключение клиента",
                                                              QString("Отключить клиента '%1'?").arg(cn),
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        addLogMessage(QString("Принудительное отключение: %1 [%2]").arg(cn).arg(realAddr), "warning");

        QString timestamp = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
        txtClientsLog->append(QString("<span style='color:red;'>[%1] 🔴 Принудительное отключение: <b>%2</b> | IP: %3</span>")
        .arg(timestamp).arg(cn).arg(realAddr));
    }
}

void MainWindow::disconnectAllClients()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Отключение всех клиентов",
                                                              "Отключить ВСЕХ подключённых клиентов?",
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        addLogMessage("Запрос на отключение всех клиентов", "warning");

        QString timestamp = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
        txtClientsLog->append(QString("<span style='color:red;'>[%1] Отключение ВСЕХ клиентов</span>")
        .arg(timestamp));

        // Перезапуск сервера отключит всех клиентов
        // Можно также использовать OpenVPN management interface
    }
}

void MainWindow::showClientDetails()
{
    QList<QTableWidgetItem*> selected = clientsTable->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    QString cn       = clientsTable->item(row, 0)->text();
    QString vpnAddr  = clientsTable->item(row, 1)->text();
    QString rx       = clientsTable->item(row, 2)->text();
    QString tx       = clientsTable->item(row, 3)->text();
    QString since    = clientsTable->item(row, 4)->text();
    // Реальный IP хранится в UserRole — показываем в деталях (не в таблице)
    QString realAddr = clientsTable->item(row, 0)->data(Qt::UserRole).toString();

    QString details = QString(
        "<h3>Информация о клиенте</h3>"
        "<table cellspacing='4'>"
        "<tr><td><b>Имя (CN):</b></td><td>%1</td></tr>"
        "<tr><td><b>VPN адрес:</b></td><td>%2</td></tr>"
        "<tr><td><b>Реальный адрес:</b></td><td><code>%3</code></td></tr>"
        "<tr><td><b>↓ Получено:</b></td><td>%4</td></tr>"
        "<tr><td><b>↑ Отправлено:</b></td><td>%5</td></tr>"
        "<tr><td><b>Активен с:</b></td><td>%6</td></tr>"
        "</table>"
    ).arg(cn).arg(vpnAddr).arg(realAddr).arg(rx).arg(tx).arg(since);

    QMessageBox::information(this, "Детали клиента", details);
}

void MainWindow::applySpeedLimit(const QString &cn, int kbps)
{
    // OpenVPN использует директиву shader в CCD-файле (байты/сек)
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString ccdDir  = appData + "/ccd";
    QDir().mkpath(ccdDir);

    // Санируем имя для файловой системы
    QString safeKey = QString(cn).replace('/', '_').replace('\\', '_').replace(' ', '_');
    QString ccdPath = ccdDir + "/" + safeKey;

    QFile ccdFile(ccdPath);
    // Читаем существующий CCD, убираем старые shader директивы
    QStringList existingLines;
    if (ccdFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!ccdFile.atEnd()) {
            QString line = QString::fromUtf8(ccdFile.readLine()).trimmed();
            if (!line.startsWith("shader ") && !line.startsWith("# speed-limit"))
                existingLines << line;
        }
        ccdFile.close();
    }

    if (!ccdFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        addLogMessage("Ошибка записи CCD для " + cn, "error");
        return;
    }
    QTextStream out(&ccdFile);
    for (const QString &l : existingLines) out << l << "\n";

    if (kbps > 0) {
        // shader принимает байты/сек = kbps * 1000 / 8
        int bytesPerSec = kbps * 1000 / 8;
        out << "# speed-limit " << kbps << " kbps\n";
        out << "shader " << bytesPerSec << "\n";
        addLogMessage(QString("Лимит скорости для %1: %2 КБит/с (%3 Б/с)")
        .arg(cn).arg(kbps).arg(bytesPerSec), "info");
    } else {
        out << "# speed-limit unlimited\n";
        addLogMessage("Лимит скорости для " + cn + " снят", "info");
    }
    ccdFile.close();

    // Сохраняем в реестр (под мьютексом)
    {
        QMutexLocker locker(&registryMutex);
        if (clientRegistry.contains(cn)) {
            clientRegistry[cn].speedLimitKbps = kbps;
        }
    }
    saveClientRegistry();
    updateRegistryTable();
}

void MainWindow::banClient()
{
    // Проверяем выбор в реестре или в активной таблице
    QString cn;
    QList<QTableWidgetItem*> regSel = registryTable->selectedItems();
    QList<QTableWidgetItem*> actSel = clientsTable->selectedItems();

    if (!regSel.isEmpty()) {
        cn = registryTable->item(regSel.first()->row(), 0)->text();
        cn.remove(QRegularExpression("^[🟢🚫]\\s"));
    } else if (!actSel.isEmpty()) {
        cn = clientsTable->item(actSel.first()->row(), 0)->data(Qt::UserRole + 1).toString();
        if (cn.isEmpty()) cn = clientsTable->item(actSel.first()->row(), 0)->text();
    } else {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::warning(this, "Блокировка клиента",
                                                             QString("Заблокировать клиента '%1'?\n\n"
                                                             "Это добавит сертификат в CRL и клиент больше не сможет подключаться.").arg(cn),
                                                             QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        addLogMessage(QString("Блокировка клиента: %1").arg(cn), "warning");

        {
            QMutexLocker locker(&registryMutex);
            if (clientRegistry.contains(cn)) {
                clientRegistry[cn].isBanned = true;
            }
        }
        saveClientRegistry();
        updateRegistryTable();

        QString timestamp = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
        addLogMessage(QString("🚫 Заблокирован: %1").arg(cn), "warning");
    }
}

void MainWindow::onClientTableContextMenu(const QPoint &pos)
{
    QMenu contextMenu(this);

    contextMenu.addAction("🔍 Детали", this, &MainWindow::showClientDetails);
    contextMenu.addAction("📊 Аналитика", this, &MainWindow::showClientAnalytics);
    contextMenu.addAction("📜 История сессий", this, &MainWindow::showClientSessionHistory);
    contextMenu.addSeparator();
    contextMenu.addAction("❌ Отключить", this, &MainWindow::disconnectSelectedClient);
    contextMenu.addAction("🚫 Заблокировать", this, &MainWindow::banClient);
    contextMenu.addSeparator();
    contextMenu.addAction("📋 Копировать VPN IP", [this]() {
        auto items = clientsTable->selectedItems();
        if (!items.isEmpty()) {
            QString vpnIP = clientsTable->item(items.first()->row(), 1)->text();
            QGuiApplication::clipboard()->setText(vpnIP);
        }
    });

    contextMenu.exec(clientsTable->mapToGlobal(pos));
}

void MainWindow::exportClientsLog()
{
    QString filename = QFileDialog::getSaveFileName(this, "Экспорт журнала клиентов",
                                                    QDir::homePath() + "/clients_log_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".txt",
                                                    "Текстовые файлы (*.txt)");

    if (filename.isEmpty()) return;

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "=== ЖУРНАЛ ПОДКЛЮЧЕНИЙ КЛИЕНТОВ ===\n";
        out << "Дата экспорта: " << QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss") << "\n\n";
        out << txtClientsLog->toPlainText();
        file.close();

        addLogMessage("Журнал клиентов экспортирован: " + filename, "success");
        QMessageBox::information(this, "Экспорт завершён",
                                 "Журнал клиентов сохранён:\n" + filename);
    } else {
        addLogMessage("Ошибка экспорта журнала клиентов", "error");
    }
}

void MainWindow::clearClientsLog()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Очистка журнала",
                                                              "Очистить журнал подключений клиентов?",
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        txtClientsLog->clear();
        addLogMessage("Журнал клиентов очищен", "info");
    }
}

void MainWindow::createServerConfig()
{
    QFile configFile(serverConfigPath);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        addLogMessage("Не удалось создать конфигурацию сервера", "error");
        return;
    }

    // Определяем версию OpenVPN для совместимости директив
    int ovpnMajor = 2, ovpnMinor = 4;
    {
        QProcess vp;
        vp.start(openVPNExecutablePath, {"--version"});
        vp.waitForFinished(3000);
        QString vout = QString::fromUtf8(vp.readAllStandardOutput())
        + QString::fromUtf8(vp.readAllStandardError());
        QRegularExpression re("OpenVPN (\\d+)\\.(\\d+)");
        auto m = re.match(vout);
        if (m.hasMatch()) {
            ovpnMajor = m.captured(1).toInt();
            ovpnMinor = m.captured(2).toInt();
        }
        addLogMessage(QString("OpenVPN версия: %1.%2").arg(ovpnMajor).arg(ovpnMinor), "info");
    }
    bool ovpn25 = (ovpnMajor > 2) || (ovpnMajor == 2 && ovpnMinor >= 5);

    QTextStream out(&configFile);

    out << "# OpenVPN Server Configuration\n";
    out << "# Generated by Tor Manager\n";
    out << "# Date: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n\n";

    // ── Базовые параметры ─────────────────────────────────────────────────
    out << "port " << spinServerPort->value() << "\n";
    QString proto = (cboServerProto && cboServerProto->currentText() == "UDP") ? "udp" : "tcp";
    out << "proto " << proto << "\n";
    out << "dev tun\n";
    out << "local 0.0.0.0\n";  // Слушаем на всех интерфейсах
    out << "\n";

    // ── Сертификаты ───────────────────────────────────────────────────────
    out << "ca "   << caCertPath    << "\n";
    out << "cert " << serverCertPath << "\n";
    out << "key "  << serverKeyPath  << "\n";

    // DH: используем ECDH (dh none) — современный и безопасный вариант
    out << "dh none\n";
    if (!ovpn25) out << "ecdh-curve prime256v1\n";
    out << "\n";

    // ── Сеть ──────────────────────────────────────────────────────────────
    QStringList network = txtServerNetwork->text().split(' ');
    if (network.size() >= 2)
        out << "server " << network[0] << " " << network[1] << "\n";
    else
        out << "server 10.8.0.0 255.255.255.0\n";
    out << "topology subnet\n\n";

    // ── Параметры клиентов ────────────────────────────────────────────────
    bool dupCN = (chkDuplicateCN    && chkDuplicateCN->isChecked());
    bool c2c   = (chkClientToClient && chkClientToClient->isChecked());
    int maxCli = (spinMaxClients ? spinMaxClients->value() : 10);
    if (c2c)   out << "client-to-client\n";
    if (dupCN) out << "duplicate-cn\n";
    out << "keepalive 10 120\n";
    out << "max-clients " << maxCli << "\n\n";

    // client-config-dir
    QString ccdDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ccd";
    QDir().mkpath(ccdDir);
    out << "client-config-dir " << ccdDir << "\n\n";

    // ── Шифрование ────────────────────────────────────────────────────────
    out << "# Encryption Settings\n";
    out << "tls-server\n";
    out << "tls-version-min 1.2\n";

    if (ovpn25) {
        // OpenVPN 2.5+ синтаксис
        out << "data-ciphers AES-256-GCM:AES-128-GCM:CHACHA20-POLY1305:AES-256-CBC\n";
        out << "data-ciphers-fallback AES-256-CBC\n";
    } else {
        // OpenVPN 2.4 синтаксис
        out << "ncp-ciphers AES-256-GCM:AES-128-GCM:AES-256-CBC:AES-128-CBC\n";
        out << "cipher AES-256-CBC\n";
    }

    out << "auth SHA256\n";
    out << "auth-nocache\n";

    // ═══════════════════════════════════════════════════════════════════════
    // ИСПРАВЛЕНИЕ: Полностью убрано "compress", оставлено только allow-compression no
    // Это решает проблему "Bad compression stub decompression header byte"
    // ═══════════════════════════════════════════════════════════════════════
    out << "\n# Compression: explicitly disabled for security and compatibility\n";
    out << "# (prevents VORACLE attack and client/server compression mismatch)\n";
    out << "allow-compression no\n";
    out << "\n";

    // ── MTU ──────────────────────────────────────────────────────────────
    int mtu = (spinMtu ? spinMtu->value() : 1500);
    out << "tun-mtu " << mtu << "\n";
    out << "mssfix " << (mtu - 150) << "\n\n";

    // Хранение выданных IP
    out << "ifconfig-pool-persist /tmp/ipp.txt\n\n";

    // ── TLS-Crypt (решает проблему со сжатием) ───────────────────────────
    {
        QString resolvedTaKey = resolveTaKeyPath();
        if (QFile::exists(resolvedTaKey)) {
            out << "# TLS-Crypt encrypts and authenticates all control channel packets\n";
            out << "# This also disables compression negotiation automatically\n";
            out << "<tls-crypt>\n";
            QFile taFile(resolvedTaKey);
            if (taFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                out << taFile.readAll();
                taFile.close();
            }
            out << "</tls-crypt>\n\n";
        }
    }

    // ── Push-опции для клиентов ───────────────────────────────────────────
    if (chkRouteThroughTor && chkRouteThroughTor->isChecked()) {
        out << "push \"redirect-gateway def1 bypass-dhcp\"\n";
        out << "push \"dhcp-option DNS 10.8.0.1\"\n";
        out << "push \"dhcp-option DNS 208.67.222.222\"\n";
        out << "push \"topology subnet\"\n";

        // УБРАНО: push "allow-compression no" — не нужно, сервер уже запрещает
        // out << "push \"allow-compression no\"\n";

        if (mtu != 1500)
            out << "push \"tun-mtu " << mtu << "\"\n";
        out << "\n";
    }

    // ── Скрипты маршрутизации ─────────────────────────────────────────────
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString upScript   = appData + "/scripts/tor-route-up.sh";
    QString downScript = appData + "/scripts/tor-route-down.sh";
    createTorRoutingScripts();
    out << "script-security 2\n";
    if (QFile::exists(upScript))   out << "up "   << upScript   << "\n";
    if (QFile::exists(downScript)) { out << "down " << downScript << "\n"; out << "down-pre\n"; }
    out << "\n";

    // ── Прочее ────────────────────────────────────────────────────────────
    out << "persist-key\n";
    out << "persist-tun\n\n";
    out << "status /tmp/openvpn-status.log 5\n";
    out << "status-version 2\n";
    out << "verb 3\n";
    out << "mute 20\n\n";
    out << "float\n";

    configFile.close();
    addLogMessage("Конфигурация сервера создана: " + serverConfigPath, "success");
}

void MainWindow::generateClientConfig()
{
    QString clientName = QInputDialog::getText(this, "Имя клиента",
                                               "Введите имя для клиентского сертификата:",
                                               QLineEdit::Normal, "client1");

    if (clientName.isEmpty()) return;

    QString savePath = QFileDialog::getSaveFileName(this, "Сохранить конфигурацию клиента",
                                                    QDir::homePath() + "/" + clientName + ".ovpn",
                                                    "OpenVPN Config (*.ovpn)");

    if (savePath.isEmpty()) return;

    // Проверяем/создаем сертификаты для клиента
    // Используем resolve-хелперы: ищем в certsDir и easy-rsa/pki/
    QString clientCertFile = resolveClientCertPath(clientName);
    QString clientKeyFile  = resolveClientKeyPath(clientName);

    if (!QFile::exists(clientCertFile) || !QFile::exists(clientKeyFile)) {
        addLogMessage("Создание сертификатов для клиента: " + clientName, "info");
        generateClientCertificate(clientName);

        QMessageBox::information(this, "Сертификаты созданы",
                                 "Сертификаты для клиента " + clientName + " созданы.\n"
                                 "Повторно нажмите кнопку создания .ovpn файла.");
        return;
    }

    QString serverAddress = "wwcat.duckdns.org";
    // Используем адрес из настроек, если задан
    if (txtServerAddress && !txtServerAddress->text().trimmed().isEmpty())
        serverAddress = txtServerAddress->text().trimmed();
    int serverPort = spinServerPort->value();

    QString config;
    config += "# OpenVPN Client Configuration\n";
    config += "# Generated by Tor Manager\n";
    config += "# Client: " + clientName + "\n";
    config += "# Server: " + serverAddress + ":" + QString::number(serverPort) + "\n\n";

    config += "client\n";
    config += "dev tun\n";
    config += "proto tcp\n";
    config += "remote " + serverAddress + " " + QString::number(serverPort) + "\n";
    config += "resolv-retry infinite\n";
    config += "nobind\n";
    config += "persist-key\n";
    config += "persist-tun\n\n";

    // Настройки безопасности — строго соответствуют server.conf
    config += "tls-client\n";
    config += "remote-cert-tls server\n";
    config += "tls-version-min 1.2\n";
    config += "data-ciphers AES-256-GCM:AES-128-GCM:CHACHA20-POLY1305:AES-256-CBC\n";
    config += "data-ciphers-fallback AES-256-CBC\n";
    config += "auth SHA256\n";
    config += "auth-nocache\n";
    config += "allow-compression no\n";
    config += "\n";

    // CA сертификат
    if (QFile::exists(caCertPath)) {
        config += "<ca>\n";
        QFile caFile(caCertPath);
        if (caFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            config += caFile.readAll();
            caFile.close();
        }
        config += "</ca>\n\n";
    }

    // Клиентский сертификат
    if (QFile::exists(clientCertFile)) {
        config += "<cert>\n";
        QFile certFile(clientCertFile);
        if (certFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString certContent = certFile.readAll();
            certFile.close();

            QRegularExpression pemRegex("-----BEGIN CERTIFICATE-----[\\s\\S]*?-----END CERTIFICATE-----");
            QRegularExpressionMatch match = pemRegex.match(certContent);
            if (match.hasMatch()) {
                config += match.captured(0) + "\n";
            } else {
                config += certContent;
            }
        }
        config += "</cert>\n\n";
    } else {
        QMessageBox::critical(this, "Ошибка", "Клиентский сертификат не найден: " + clientCertFile);
        return;
    }

    // Клиентский ключ
    if (QFile::exists(clientKeyFile)) {
        config += "<key>\n";
        QFile keyFile(clientKeyFile);
        if (keyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            config += keyFile.readAll();
            keyFile.close();
        }
        config += "</key>\n\n";
    } else {
        QMessageBox::critical(this, "Ошибка", "Клиентский ключ не найден: " + clientKeyFile);
        return;
    }

    // tls-crypt — КРИТИЧНО: должен совпадать с ключом в server.conf
    {
        QString taKeyContent = extractTaKeyFromServerConf();
        if (!taKeyContent.isEmpty()) {
            config += "<tls-crypt>\n";
            config += taKeyContent + "\n";
            config += "</tls-crypt>\n\n";
            addLogMessage("✅ ta.key извлечён из server.conf (гарантированное совпадение)", "success");
        } else if (QFile::exists(taKeyPath)) {
            config += "<tls-crypt>\n";
            QFile taFile(taKeyPath);
            if (taFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                config += taFile.readAll();
                taFile.close();
            }
            config += "</tls-crypt>\n\n";
            addLogMessage("⚠️ ta.key из файла (fallback): " + taKeyPath, "warning");
        } else {
            addLogMessage("❌ ta.key не найден — подключение клиента не будет работать!", "error");
        }
    }

    config += "\n# Verbose level\n";
    config += "verb 3\n";
    config += "mute 10\n";

    QFile file(savePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(config.toUtf8());
        file.close();
        addLogMessage("Конфигурация клиента сохранена: " + savePath, "success");

        QMessageBox::information(this, "Успех",
                                 "Конфигурация клиента сохранена.\n"
                                 "Файл: " + savePath + "\n\n"
                                 "Импортируйте этот файл в OpenVPN для Android.");
    } else {
        addLogMessage("Ошибка сохранения файла: " + savePath, "error");
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл конфигурации");
    }
}

void MainWindow::generateClientCertificate(const QString &clientName)
{
    addLogMessage("Генерация сертификатов для клиента: " + clientName, "info");

    QString easyRSAPath = findEasyRSA();
    if (!easyRSAPath.isEmpty()) {
        QString workDir = certsDir + "/easy-rsa";
        QDir().mkpath(workDir);

        QStringList args;
        args << "build-client-full" << clientName << "nopass";

        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("EASYRSA_BATCH", "1");

        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(workDir);
        process->setProcessEnvironment(env);

        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, process, clientName](int code, QProcess::ExitStatus) {
                    if (code == 0) {
                        QString pkiDir = certsDir + "/easy-rsa/pki";
                        QFile::copy(pkiDir + "/issued/" + clientName + ".crt",
                                    certsDir + "/" + clientName + ".crt");
                        QFile::copy(pkiDir + "/private/" + clientName + ".key",
                                    certsDir + "/" + clientName + ".key");

                        addLogMessage("✅ Сертификаты для клиента " + clientName + " созданы успешно", "success");
                    } else {
                        addLogMessage("Ошибка создания сертификатов для клиента " + clientName, "error");
                    }
                    process->deleteLater();
                });

        process->start(easyRSAPath, args);
    } else {
        addLogMessage("Используется OpenSSL для генерации клиентских сертификатов", "info");

        QProcess *genKey = new QProcess(this);
        genKey->setWorkingDirectory(certsDir);
        connect(genKey, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, genKey, clientName](int code, QProcess::ExitStatus) {
                    if (code == 0) {
                        QProcess *genCsr = new QProcess(this);
                        genCsr->setWorkingDirectory(certsDir);
                        connect(genCsr, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                                [this, genCsr, clientName](int code, QProcess::ExitStatus) {
                                    if (code == 0) {
                                        QProcess *signCert = new QProcess(this);
                                        signCert->setWorkingDirectory(certsDir);
                                        connect(signCert, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                                                [this, signCert, clientName](int code, QProcess::ExitStatus) {
                                                    if (code == 0) {
                                                        addLogMessage("✅ Сертификаты для клиента " + clientName + " созданы успешно", "success");
                                                    } else {
                                                        addLogMessage("Ошибка подписи сертификата клиента", "error");
                                                    }
                                                    signCert->deleteLater();
                                                });

                                        signCert->start("openssl", QStringList()
                                        << "x509" << "-req"
                                        << "-in" << (certsDir + "/" + clientName + ".csr")
                                        << "-CA" << caCertPath
                                        << "-CAkey" << (certsDir + "/ca.key")
                                        << "-CAcreateserial"
                                        << "-out" << (certsDir + "/" + clientName + ".crt")
                                        << "-days" << "365"
                                        << "-outform" << "PEM");
                                    } else {
                                        addLogMessage("Ошибка генерации CSR клиента", "error");
                                    }
                                    genCsr->deleteLater();
                                });

                        // SECURITY FIX: санитизируем clientName — запрещаем / \ " ' и пробелы
                        // которые могут сломать Distinguished Name или инъецировать параметры
                        QString safeCN = clientName;
                        safeCN.remove(QRegularExpression(R"([/\\"\s<>])"));
                        safeCN.remove(QChar('\'' ));  // одинарная кавычка отдельно
                        if (safeCN.isEmpty()) safeCN = "client";
                        genCsr->start("openssl", QStringList()
                        << "req" << "-new"
                        << "-key" << (certsDir + "/" + clientName + ".key")
                        << "-out" << (certsDir + "/" + clientName + ".csr")
                        << "-subj" << ("/C=RU/ST=Moscow/L=Moscow/O=TorManager/CN=" + safeCN));
                    } else {
                        addLogMessage("Ошибка генерации ключа клиента", "error");
                    }
                    genKey->deleteLater();
                });

        genKey->start("openssl", QStringList()
        << "genrsa" << "-out"
        << (certsDir + "/" + clientName + ".key") << "2048");
    }
}

void MainWindow::createTorRoutingScripts()
{
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData + "/scripts");
    QDir().mkpath(appData + "/ccd");

    QString extIf = getExternalInterface();
    QString tunNet = txtServerNetwork->text().split(' ')[0];

    // Скрипт UP
    QString upScript = appData + "/scripts/tor-route-up.sh";
    QFile upFile(upScript);
    if (upFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&upFile);
        s << "#!/bin/bash\n";
        s << "# tor-route-up.sh — Auto-generated by Tor Manager\n\n";
        s << "EXT_IF=\"" << extIf << "\"\n";
        s << "[ -z \"$EXT_IF\" ] && EXT_IF=$(ip route | awk '/default/{print $5; exit}')\n";
        s << "TUN_NET=\"" << tunNet << "/24\"\n";
        s << "TOR_TRANS_PORT=9040\n";
        s << "TOR_DNS_PORT=5353\n\n";

        s << "echo \"[up] EXT_IF=$EXT_IF TUN_NET=$TUN_NET\"\n\n";

        s << "# 1. IP forwarding\n";
        s << "sysctl -w net.ipv4.ip_forward=1 >/dev/null 2>&1\n\n";

        s << "# 2. Очистка старых правил\n";
        s << "iptables -t nat -D POSTROUTING -s $TUN_NET -o $EXT_IF -j MASQUERADE 2>/dev/null || true\n";
        s << "iptables -t nat -D PREROUTING -i tun+ -p tcp --syn -j REDIRECT --to-ports $TOR_TRANS_PORT 2>/dev/null || true\n";
        s << "iptables -t nat -D PREROUTING -i tun+ -p udp --dport 53 -j REDIRECT --to-ports $TOR_DNS_PORT 2>/dev/null || true\n";
        s << "iptables -t nat -D PREROUTING -i tun+ -p tcp --dport 53 -j REDIRECT --to-ports $TOR_DNS_PORT 2>/dev/null || true\n\n";

        s << "# 3. NAT\n";
        s << "iptables -t nat -A POSTROUTING -s $TUN_NET -o $EXT_IF -j MASQUERADE\n";
        s << "echo \"✓ NAT rule added successfully\"\n\n";

        s << "# 4. FORWARD правила\n";
        s << "iptables -C FORWARD -i tun+ -o $EXT_IF -j ACCEPT 2>/dev/null || \\\n";
        s << "    iptables -I FORWARD -i tun+ -o $EXT_IF -j ACCEPT\n";
        s << "iptables -C FORWARD -i $EXT_IF -o tun+ -m state --state ESTABLISHED,RELATED -j ACCEPT 2>/dev/null || \\\n";
        s << "    iptables -I FORWARD -i $EXT_IF -o tun+ -m state --state ESTABLISHED,RELATED -j ACCEPT\n";
        s << "iptables -C FORWARD -s $TUN_NET -j ACCEPT 2>/dev/null || \\\n";
        s << "    iptables -I FORWARD -s $TUN_NET -j ACCEPT\n\n";

        s << "# 5. DNS → Tor DNSPort\n";
        s << "iptables -t nat -I PREROUTING -i tun+ -p udp --dport 53 -j REDIRECT --to-ports $TOR_DNS_PORT\n";
        s << "iptables -t nat -I PREROUTING -i tun+ -p tcp --dport 53 -j REDIRECT --to-ports $TOR_DNS_PORT\n";
        s << "echo \"✓ DNS redirected to Tor DNSPort ($TOR_DNS_PORT)\"\n\n";

        s << "# 6. TCP → Tor TransPort\n";
        s << "iptables -t nat -A PREROUTING -i tun+ -d 127.0.0.0/8    -j RETURN\n";
        s << "iptables -t nat -A PREROUTING -i tun+ -d 10.0.0.0/8     -j RETURN\n";
        s << "iptables -t nat -A PREROUTING -i tun+ -d 172.16.0.0/12  -j RETURN\n";
        s << "iptables -t nat -A PREROUTING -i tun+ -d 192.168.0.0/16 -j RETURN\n";
        s << "SERVER_TUN_IP=$(ip -4 addr show tun0 2>/dev/null | awk '/inet /{split($2,a,\"/\"); print a[1]; exit}')\n";
        s << "[ -n \"$SERVER_TUN_IP\" ] && iptables -t nat -A PREROUTING -i tun+ -d $SERVER_TUN_IP -j RETURN 2>/dev/null || true\n";
        s << "iptables -t nat -A PREROUTING -i tun+ -p tcp --syn -j REDIRECT --to-ports $TOR_TRANS_PORT\n";
        s << "echo \"✓ TCP traffic redirected to Tor TransPort ($TOR_TRANS_PORT)\"\n\n";

        s << "echo \"✓ Tor routing enabled. VPN=$TUN_NET EXT=$EXT_IF\"\n";
        upFile.close();
        QFile::setPermissions(upScript,
                              QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|
                              QFile::ReadGroup|QFile::ExeGroup|
                              QFile::ReadOther|QFile::ExeOther);
    }

    // Скрипт DOWN
    QString downScript = appData + "/scripts/tor-route-down.sh";
    QFile downFile(downScript);
    if (downFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&downFile);
        s << "#!/bin/bash\n";
        s << "# tor-route-down.sh — Auto-generated by Tor Manager\n\n";
        s << "EXT_IF=\"" << extIf << "\"\n";
        s << "[ -z \"$EXT_IF\" ] && EXT_IF=$(ip route | awk '/default/{print $5; exit}')\n";
        s << "TUN_NET=\"" << tunNet << "/24\"\n";
        s << "TOR_TRANS_PORT=9040\n";
        s << "TOR_DNS_PORT=5353\n\n";

        s << "iptables -t nat -D POSTROUTING -s $TUN_NET -o $EXT_IF -j MASQUERADE 2>/dev/null || true\n";
        s << "iptables -D FORWARD -i tun+ -o $EXT_IF -j ACCEPT 2>/dev/null || true\n";
        s << "iptables -D FORWARD -i $EXT_IF -o tun+ -m state --state ESTABLISHED,RELATED -j ACCEPT 2>/dev/null || true\n";
        s << "iptables -D FORWARD -s $TUN_NET -j ACCEPT 2>/dev/null || true\n\n";

        s << "# DNS редиректы\n";
        s << "iptables -t nat -D PREROUTING -i tun+ -p udp --dport 53 -j REDIRECT --to-ports $TOR_DNS_PORT 2>/dev/null || true\n";
        s << "iptables -t nat -D PREROUTING -i tun+ -p tcp --dport 53 -j REDIRECT --to-ports $TOR_DNS_PORT 2>/dev/null || true\n\n";

        s << "# TCP-исключения\n";
        s << "iptables -t nat -D PREROUTING -i tun+ -d 127.0.0.0/8    -j RETURN 2>/dev/null || true\n";
        s << "iptables -t nat -D PREROUTING -i tun+ -d 10.0.0.0/8     -j RETURN 2>/dev/null || true\n";
        s << "iptables -t nat -D PREROUTING -i tun+ -d 172.16.0.0/12  -j RETURN 2>/dev/null || true\n";
        s << "iptables -t nat -D PREROUTING -i tun+ -d 192.168.0.0/16 -j RETURN 2>/dev/null || true\n";
        s << "iptables -t nat -D PREROUTING -i tun+ -p tcp --syn -j REDIRECT --to-ports $TOR_TRANS_PORT 2>/dev/null || true\n\n";

        s << "echo \"✓ Tor routing disabled\"\n";
        downFile.close();
        QFile::setPermissions(downScript,
                              QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|
                              QFile::ReadGroup|QFile::ExeGroup|
                              QFile::ReadOther|QFile::ExeOther);
    }

    // CCD default
    QString ccdDir = appData + "/ccd";
    QFile ccdDefault(ccdDir + "/DEFAULT");
    if (ccdDefault.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&ccdDefault);
        s << "# Default client config\n";
        s << "# Разрешаем трафик с реальных LAN-адресов клиента\n";
        s << "iroute 192.168.0.0 255.255.255.0\n";
        s << "iroute 192.168.1.0 255.255.255.0\n";
        s << "iroute 10.0.0.0 255.0.0.0\n";
        s << "iroute 172.16.0.0 255.240.0.0\n";
        ccdDefault.close();
    }

    addLogMessage("Скрипты маршрутизации созданы в: " + appData + "/scripts", "success");
}

// ========== МАРШРУТИЗАЦИЯ ==========

QString MainWindow::getExternalInterface()
{
    QString interface = "unknown";

    #ifdef Q_OS_LINUX
    QProcess process;
    process.start("sh", QStringList() << "-c" << "ip route | grep default | awk '{print $5}' | head -1");
    if (process.waitForFinished(3000)) {
        interface = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        if (!interface.isEmpty() && interface != "unknown") {
            addLogMessage("Определен внешний интерфейс (ip route): " + interface, "info");
            return interface;
        }
    }

    process.start("sh", QStringList() << "-c" << "route -n | grep '^0.0.0.0' | awk '{print $8}' | head -1");
    if (process.waitForFinished(3000)) {
        interface = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        if (!interface.isEmpty() && interface != "unknown") {
            addLogMessage("Определен внешний интерфейс (route): " + interface, "info");
            return interface;
        }
    }

    QStringList commonIfs = {"eth0", "enp0s3", "ens33", "enp2s0", "wlan0", "wlp2s0", "ens160"};
    for (const QString &iface : commonIfs) {
        process.start("sh", QStringList() << "-c" << "ip link show " + iface + " 2>/dev/null | grep -q UP && echo exists");
        if (process.waitForFinished(2000)) {
            QString result = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            if (!result.isEmpty()) {
                addLogMessage("Найден активный интерфейс: " + iface, "info");
                return iface;
            }
        }
    }
    #endif

    addLogMessage("Не удалось определить внешний интерфейс, используется eth0", "warning");
    return "eth0";
}

bool MainWindow::setupIPTablesRules(bool enable)
{
    #ifdef Q_OS_LINUX
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString scriptPath = appData + "/scripts/" + (enable ? "tor-route-up.sh" : "tor-route-down.sh");

    if (!QFile::exists(scriptPath)) {
        createTorRoutingScripts();
    }

    if (!QFile::exists(scriptPath)) {
        addLogMessage("Скрипт маршрутизации не найден: " + scriptPath, "error");
        return false;
    }

    addLogMessage(enable ? "Настройка маршрутизации через Tor..." : "Отключение маршрутизации...", "info");

    QFile::setPermissions(scriptPath,
                          QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                          QFile::ReadGroup | QFile::ExeGroup |
                          QFile::ReadOther | QFile::ExeOther);

    QProcess process;
    QStringList args;

    bool hasRoot = (geteuid() == 0);

    if (!hasRoot) {
        if (QFile::exists("/usr/bin/pkexec")) {
            args << "pkexec" << "bash" << scriptPath;
            addLogMessage("Используется pkexec для получения root прав", "info");
        } else if (QFile::exists("/usr/bin/sudo")) {
            args << "sudo" << "bash" << scriptPath;
            addLogMessage("Используется sudo для получения root прав", "info");
        } else {
            addLogMessage("Нет root прав и не найдены pkexec/sudo", "error");
            return false;
        }
    } else {
        args << "bash" << scriptPath;
    }

    addLogMessage("Выполнение команды: " + args.join(" "), "info");

    process.start(args.takeFirst(), args);

    if (!process.waitForFinished(30000)) {
        addLogMessage("Таймаут при настройке маршрутизации", "error");
        return false;
    }

    QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    QString error = QString::fromUtf8(process.readAllStandardError()).trimmed();

    if (!output.isEmpty()) {
        QStringList lines = output.split('\n');
        for (const QString &line : lines) {
            if (!line.isEmpty()) {
                if (line.contains("✓")) {
                    addLogMessage(line, "success");
                } else {
                    addLogMessage(line, "info");
                }
            }
        }
    }

    if (!error.isEmpty()) {
        addLogMessage("Ошибки выполнения: " + error, "warning");
    }

    if (process.exitCode() == 0) {
        addLogMessage("✓ Маршрутизация " + QString(enable ? "включена" : "отключена"), "success");

        if (enable) {
            QTimer::singleShot(1000, this, &MainWindow::verifyRouting);
        }

        return true;
    } else {
        addLogMessage("✗ Ошибка настройки маршрутизации (код: " +
        QString::number(process.exitCode()) + ")", "error");

        if (enable && hasRoot) {
            addLogMessage("Попытка применить правила вручную...", "info");
            applyRoutingManually();
        }

        return false;
    }
    #else
    Q_UNUSED(enable);
    return true;
    #endif
}

void MainWindow::applyRoutingManually()
{
    #ifdef Q_OS_LINUX
    QString extIf = getExternalInterface();
    QString vpnNet = txtServerNetwork->text().split(' ')[0] + "/24";

    // Валидация входных данных — защита от инъекций
    static QRegularExpression safeNetRe("^[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}/[0-9]{1,2}$");
    static QRegularExpression safeIfRe("^[a-zA-Z0-9_\\-\\.]{1,20}$");
    if (!safeNetRe.match(vpnNet).hasMatch()) {
        addLogMessage("applyRoutingManually: недопустимый формат сети: " + vpnNet, "error");
        return;
    }
    if (!safeIfRe.match(extIf).hasMatch()) {
        addLogMessage("applyRoutingManually: недопустимый интерфейс: " + extIf, "error");
        return;
    }

    addLogMessage("Ручное применение правил маршрутизации...", "info");

    const QString iptables = "/sbin/iptables";
    const QString sysctl   = "/sbin/sysctl";

    runSafeCommand(sysctl,   {"-w", "net.ipv4.ip_forward=1"});
    runSafeCommand(iptables, {"-F", "FORWARD"});
    runSafeCommand(iptables, {"-t", "nat", "-F", "POSTROUTING"});

    // NAT: MASQUERADE
    runSafeCommand(iptables, {"-t", "nat", "-A", "POSTROUTING",
        "-s", vpnNet, "-o", extIf, "-j", "MASQUERADE"});
    addLogMessage("NAT правило добавлено", "success");

    // FORWARD rules
    runSafeCommand(iptables, {"-A", "FORWARD", "-i", "tun+", "-o", extIf, "-j", "ACCEPT"});
    runSafeCommand(iptables, {"-A", "FORWARD", "-i", extIf, "-o", "tun+",
        "-m", "state", "--state", "ESTABLISHED,RELATED", "-j", "ACCEPT"});
    runSafeCommand(iptables, {"-A", "FORWARD", "-i", "tun+", "-o", "tun+", "-j", "ACCEPT"});
    runSafeCommand(iptables, {"-A", "FORWARD", "-s", vpnNet, "-j", "ACCEPT"});
    runSafeCommand(iptables, {"-A", "FORWARD", "-d", vpnNet, "-j", "ACCEPT"});

    addLogMessage("Ручное применение правил завершено", "info");

    verifyRouting();
    #endif
}

void MainWindow::verifyRouting()
{
    #ifdef Q_OS_LINUX
    addLogMessage("=== ПРОВЕРКА МАРШРУТИЗАЦИИ ===", "info");

    QFile fwdFile("/proc/sys/net/ipv4/ip_forward");
    if (fwdFile.open(QIODevice::ReadOnly)) {
        QString value = QString::fromUtf8(fwdFile.readAll()).trimmed();
        fwdFile.close();
        if (value == "1") {
            addLogMessage("✓ IP forwarding включен", "success");
        } else {
            addLogMessage("✗ IP forwarding выключен", "error");
        }
    }

    QString extIf = getExternalInterface();
    QString vpnNet = txtServerNetwork->text().split(' ')[0] + "/24";

    // SECURITY FIX: vpnNet уже провалидирован regex выше, но executeCommand использует sh -c.
    // Используем runSafeCommand для iptables + grep напрямую без shell.
    QProcess natCheck;
    natCheck.start("sh", {"-c",
        QString("iptables -t nat -L POSTROUTING -n -v 2>/dev/null | grep -F '%1'").arg(vpnNet)});
    natCheck.waitForFinished(5000);
    QString checkNat = QString::fromUtf8(natCheck.readAllStandardOutput()).trimmed();
    if (!checkNat.isEmpty()) {
        addLogMessage("✓ NAT правило присутствует", "success");
        QStringList lines = checkNat.split('\n');
        for (const QString &line : lines) {
            if (!line.trimmed().isEmpty()) {
                addLogMessage("  " + line.trimmed(), "info");
            }
        }
    } else {
        addLogMessage("✗ NAT правило отсутствует", "error");
    }

    QProcess fwdCheck;
    fwdCheck.start("sh", {"-c",
        QString("iptables -L FORWARD -n -v 2>/dev/null | grep -E 'tun|%1'").arg(vpnNet)});
    fwdCheck.waitForFinished(5000);
    QString checkForward = QString::fromUtf8(fwdCheck.readAllStandardOutput()).trimmed();
    if (!checkForward.isEmpty()) {
        addLogMessage("✓ FORWARD правила присутствуют", "success");
        QStringList lines = checkForward.split('\n');
        for (const QString &line : lines) {
            if (!line.trimmed().isEmpty()) {
                addLogMessage("  " + line.trimmed(), "info");
            }
        }
    } else {
        addLogMessage("✗ FORWARD правила отсутствуют", "error");
    }

    // Проверка TUN интерфейса через прямой вызов ip (без shell)
    {
        QProcess ipProc;
        ipProc.start("ip", {"link", "show", "type", "tun"});
        if (ipProc.waitForFinished(3000)) {
            QString tunOut = QString::fromUtf8(ipProc.readAllStandardOutput());
            if (!tunOut.trimmed().isEmpty())
                addLogMessage("✓ TUN интерфейс активен", "success");
        }
    }

    addLogMessage("Выполнение тестового пинга до 8.8.8.8...", "info");
    // Безопасный ping без shell — фиксированные аргументы
    {
        QProcess pingProc;
        pingProc.start("ping", {"-c", "2", "-W", "2", "8.8.8.8"});
        if (pingProc.waitForFinished(8000)) {
            QString pingOut = QString::fromUtf8(pingProc.readAllStandardOutput());
            if (pingOut.contains("2 received") || pingOut.contains("1 received")) {
                addLogMessage("✓ Пинг до внешнего мира работает", "success");
            } else {
                addLogMessage("✗ Пинг до внешнего мира не работает", "error");
                addLogMessage("  Возможно, проблемы с маршрутизацией", "warning");
            }
        } else {
            pingProc.kill();
            addLogMessage("✗ Пинг: таймаут (8 сек)", "warning");
        }
    }

    addLogMessage("=== КОНЕЦ ПРОВЕРКИ ===", "info");
    #endif
}

void MainWindow::enableIPForwarding()
{
    #ifdef Q_OS_LINUX
    if (!checkIPForwarding()) {
        addLogMessage("Включение IP forwarding...", "info");

        QProcess process;
        if (geteuid() != 0) {
            process.start("pkexec", QStringList() << "sysctl" << "-w" << "net.ipv4.ip_forward=1");
        } else {
            process.start("sysctl", QStringList() << "-w" << "net.ipv4.ip_forward=1");
        }
        process.waitForFinished(5000);

        QProcess sedProcess;
        QString sedCmd = "sed -i 's/#net.ipv4.ip_forward=1/net.ipv4.ip_forward=1/g' /etc/sysctl.conf";
        sedCmd += " && grep -q '^net.ipv4.ip_forward=1' /etc/sysctl.conf || ";
        sedCmd += "echo 'net.ipv4.ip_forward=1' >> /etc/sysctl.conf";

        if (geteuid() != 0) {
            sedProcess.start("pkexec", QStringList() << "sh" << "-c" << sedCmd);
        } else {
            sedProcess.start("sh", QStringList() << "-c" << sedCmd);
        }
        sedProcess.waitForFinished(5000);

        if (process.exitCode() == 0) {
            addLogMessage("IP forwarding включен (временно и постоянно)", "success");
        } else {
            addLogMessage("Не удалось включить IP forwarding", "warning");
        }
    }
    #endif
}

bool MainWindow::checkIPForwarding()
{
    #ifdef Q_OS_LINUX
    QFile file("/proc/sys/net/ipv4/ip_forward");
    if (file.open(QIODevice::ReadOnly)) {
        QString value = QString::fromUtf8(file.readAll()).trimmed();
        file.close();
        return value == "1";
    }
    #endif
    return false;
}

// ========== ГЕНЕРАЦИЯ СЕРТИФИКАТОВ ==========

void MainWindow::generateCertificates()
{
    generateCertificatesAsync();
}

void MainWindow::generateCertificatesAsync()
{
    if (!certGenerator) {
        certGenerator = new CertificateGenerator(this);

        connect(certGenerator, &CertificateGenerator::logMessage,
                this, &MainWindow::addLogMessage);
        connect(certGenerator, &CertificateGenerator::finished,
                this, &MainWindow::onCertGenerationFinished);
        connect(certGenerator, &CertificateGenerator::progress,
                [this](int percent) {
                    statusBar()->showMessage(QString("Генерация сертификатов: %1%").arg(percent));
                });
    }

    bool useEasyRSA = !findEasyRSA().isEmpty();

    btnGenerateCerts->setEnabled(false);
    btnGenerateCerts->setText("Генерация...");

    certGenerator->generateCertificates(certsDir, openVPNExecutablePath, useEasyRSA);
}

void MainWindow::onCertGenerationFinished(bool success)
{
    btnGenerateCerts->setEnabled(true);
    btnGenerateCerts->setText("Сгенерировать сертификаты");
    statusBar()->showMessage("Готов");

    if (success) {
        addLogMessage("Сертификаты успешно сгенерированы!", "success");

        if (!serverMode && !serverStopPending) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, "Запуск сервера",
                                                                      "Сертификаты готовы. Запустить сервер сейчас?",
                                                                      QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                startOpenVPNServer();
            }
        }
    } else {
        addLogMessage("Ошибка при генерации сертификатов", "error");
        QMessageBox::critical(this, "Ошибка",
                              "Не удалось сгенерировать сертификаты. Проверьте журнал.");
    }
}

void MainWindow::checkCertificates()
{
    QStringList missing;
    QStringList found;

    if (QFile::exists(caCertPath)) found << "CA сертификат";
    else missing << "CA сертификат";

    if (QFile::exists(serverCertPath)) found << "Сертификат сервера";
    else missing << "Сертификат сервера";

    if (QFile::exists(serverKeyPath)) found << "Ключ сервера";
    else missing << "Ключ сервера";

    if (QFile::exists(dhParamPath)) found << "DH параметры";
    else missing << "DH параметры";

    QString message;
    if (!found.isEmpty()) {
        message += "<b>Найдены сертификаты:</b><br>" + found.join("<br>") + "<br><br>";
    }

    if (!missing.isEmpty()) {
        message += "<b style='color:red;'>Отсутствуют:</b><br>" + missing.join("<br>") + "<br><br>";
    }

    message += "<b>Директория:</b><br>" + certsDir;

    if (missing.isEmpty()) {
        QMessageBox::information(this, "Проверка сертификатов", message);
    } else {
        QMessageBox::warning(this, "Проверка сертификатов", message);
    }
}

// ========== ПРОВЕРКА СЕТИ ==========

void MainWindow::checkIPLeak()
{
    addLogMessage("Проверка текущего IP-адреса...", "info");
    lblCurrentIP->setText("Текущий IP: <i style='color:gray;'>проверка...</i>");

    QProcess *curlProcess = new QProcess(this);
    QStringList args;

    if (torRunning) {
        args << "--socks5-hostname"
        << QString("127.0.0.1:%1").arg(spinTorSocksPort->value())
        << "--max-time" << "15"
        << "--silent"
        << "--connect-timeout" << "10"
        << "https://api.ipify.org";
        addLogMessage("Проверка IP через Tor (порт " +
        QString::number(spinTorSocksPort->value()) + ")...", "info");
    } else {
        args << "--max-time" << "10"
        << "--silent"
        << "https://api.ipify.org";
    }

    connect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, curlProcess](int exitCode, QProcess::ExitStatus) {
                QString ip = QString::fromUtf8(curlProcess->readAllStandardOutput()).trimmed();
                curlProcess->deleteLater();

                if (exitCode == 0 && !ip.isEmpty() &&
                    QRegularExpression("^[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}$").match(ip).hasMatch()) {
                    currentIP = ip;
                torIP = ip;
                QString color = torRunning ? "#00aa00" : "#cc6600";
                QString icon  = torRunning ? "🔒 " : "🌐 ";
                QString mode  = torRunning ? " (Tor)" : " (прямой)";
                lblCurrentIP->setText("Текущий IP: <b style='color:" + color + ";'>" + icon + ip + mode + "</b>");
                lblTorIP->setText("<b style='color:" + color + ";'>" + icon + ip + "</b>");
                addLogMessage("✓ Текущий IP: " + ip + mode, "success");
                    } else {
                        addLogMessage("api.ipify.org недоступен, пробуем icanhazip.com...", "warning");
                        QProcess *fallback = new QProcess(this);
                        QStringList fbArgs;
                        fbArgs << "--max-time" << "8" << "--silent" << "https://icanhazip.com";
                        connect(fallback, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                                this, [this, fallback](int code, QProcess::ExitStatus) {
                                    QString ip2 = QString::fromUtf8(fallback->readAllStandardOutput()).trimmed();
                                    fallback->deleteLater();
                                    if (code == 0 && !ip2.isEmpty()) {
                                        currentIP = ip2;
                                        lblCurrentIP->setText("Текущий IP: <b style='color:#cc6600;'>🌐 " + ip2 + " (fallback)</b>");
                                        lblTorIP->setText("<b>🌐 " + ip2 + "</b>");
                                        addLogMessage("✓ IP (fallback): " + ip2, "info");
                                    } else {
                                        lblCurrentIP->setText("Текущий IP: <b style='color:red;'>Ошибка проверки</b>");
                                        addLogMessage("✗ Не удалось определить IP-адрес", "error");
                                    }
                                });
                        fallback->start("curl", fbArgs);
                    }
            });

    curlProcess->start("curl", args);
}

void MainWindow::onIPCheckFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) reply->deleteLater();
}

void MainWindow::requestExternalIP()
{
    checkIPLeak();
}

// ========== КОНФИГУРАЦИЯ TOR ==========

void MainWindow::createTorConfig()
{
    QFile configFile(torrcPath);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        addLogMessage("Не удалось создать файл конфигурации Tor", "error");
        return;
    }

    QTextStream out(&configFile);

    out << "# Tor Configuration File\n";
    out << "# Generated by Tor Manager\n\n";
    out << "DataDirectory " << torDataDir << "\n";
    // SocksPort: один адрес — либо внешний (0.0.0.0) либо локальный (127.0.0.1)
    // НЕЛЬЗЯ писать два SocksPort на одном порту — Tor не запустится
    bool extSocks = (chkExternalSocks && chkExternalSocks->isChecked());
    out << "SocksPort " << (extSocks ? "0.0.0.0:" : "127.0.0.1:")
        << spinTorSocksPort->value() << "\n";
    out << "ControlPort " << spinTorControlPort->value() << "\n";
    // SECURITY FIX: включаем cookie-аутентификацию для Control Port
    // Без неё любой локальный процесс может управлять Tor (NEWNYM, SHUTDOWN и т.д.)
    out << "CookieAuthentication 1\n";
    out << "CookieAuthFile " << torDataDir << "/control_auth_cookie\n\n";
    out << "Log notice file " << torDataDir << "/tor.log\n";
    out << "Log notice stdout\n\n";
    out << "AvoidDiskWrites 1\n";
    out << "HardwareAccel 1\n\n";

    if (!configuredBridges.isEmpty() && cboBridgeType->currentText() != "Нет") {
        out << "# Bridge Configuration\n";
        out << "UseBridges 1\n\n";

        QSet<QString> usedTransports;
        for (const QString &bridge : configuredBridges) {
            QString type = detectBridgeType(bridge);
            if (!type.isEmpty() && type != "unknown") {
                usedTransports.insert(type);
            }
        }

        QString lyrebirdPath = findLyrebirdPath();
        if (lyrebirdPath.isEmpty()) {
            addLogMessage("ВНИМАНИЕ: lyrebird не найден! Используются отдельные плагины.", "warning");

            if (usedTransports.contains("obfs4")) {
                out << "ClientTransportPlugin obfs4 exec /usr/bin/obfs4proxy\n";
            }
            if (usedTransports.contains("webtunnel")) {
                out << "ClientTransportPlugin webtunnel exec /usr/bin/webtunnel\n";
            }
            if (usedTransports.contains("snowflake")) {
                out << "ClientTransportPlugin snowflake exec /usr/bin/snowflake-client\n";
            }
        } else {
            // lyrebird поддерживает obfs4, webtunnel, snowflake — один плагин для всех
            // Но записываем по одной строке на каждый транспорт (Tor требует явное сопоставление)
            for (const QString &transport : usedTransports) {
                out << "ClientTransportPlugin " << transport << " exec " << lyrebirdPath << "\n";
            }
            addLogMessage("✅ lyrebird найден: " + lyrebirdPath, "info");
        }

        // Проверяем доступность url= доменов для webtunnel мостов
        for (const QString &bridge : configuredBridges) {
            if (detectBridgeType(bridge) == "webtunnel") {
                QRegularExpression urlRe(R"(url=(https?://([^/\s]+)))");
                auto m = urlRe.match(bridge);
                if (m.hasMatch()) {
                    QString domain = m.captured(2);
                    QProcess check;
                    check.start("nslookup", {domain, "8.8.8.8"});
                    check.waitForFinished(3000);
                    bool ok = (check.exitCode() == 0 &&
                               !check.readAllStandardOutput().contains("NXDOMAIN"));
                    if (!ok) {
                        addLogMessage(QString("⚠️ webtunnel мост: домен '%1' не резолвится — "
                            "мост может не работать. Используйте кнопку 🔍 Тест для диагностики.")
                            .arg(domain), "warning");
                    }
                }
            }
        }
        out << "\n";

        out << "# Bridge lines\n";
        for (const QString &bridge : configuredBridges) {
            out << "Bridge " << normalizeBridgeLine(bridge) << "\n";
        }
        out << "\n";
    }

    out << "NumEntryGuards 3\n";
    out << "CircuitBuildTimeout 30\n\n";
    out << "ExitPolicy reject *:*\n";

    // ── TransPort и DNSPort ───────────────────────────────────────────────
    if (chkRouteThroughTor && chkRouteThroughTor->isChecked()) {
        out << "\n# Transparent Proxy for VPN server clients\n";

        bool useTrans = chkExternalTrans && chkExternalTrans->isChecked();
        bool useDns   = chkExternalDns   && chkExternalDns->isChecked();
        int  transPort = (spinTransPort ? spinTransPort->value() : 9040);
        int  dnsPort   = (spinDnsPort   ? spinDnsPort->value()   : 5353);

        if (useTrans) {
            out << "TransPort 0.0.0.0:" << transPort << "\n";
        } else {
            out << "TransPort 127.0.0.1:" << transPort << "\n";
        }

        if (useDns) {
            out << "DNSPort 0.0.0.0:" << dnsPort << "\n";
        } else {
            out << "DNSPort 127.0.0.1:" << dnsPort << "\n";
        }

        out << "AutomapHostsOnResolve 1\n";
        out << "VirtualAddrNetworkIPv4 10.192.0.0/10\n";
    } else if (chkExternalTrans && chkExternalTrans->isChecked()) {
        // TransPort включён отдельно (без маршрутизации через Tor)
        out << "\n# TransPort (внешний прозрачный прокси)\n";
        out << "TransPort 0.0.0.0:" << (spinTransPort ? spinTransPort->value() : 9040) << "\n";
        out << "AutomapHostsOnResolve 1\n";
        out << "VirtualAddrNetworkIPv4 10.192.0.0/10\n";
    }

    if (chkExternalDns && chkExternalDns->isChecked() &&
        !(chkRouteThroughTor && chkRouteThroughTor->isChecked())) {
        out << "\n# DNSPort (внешний DNS через Tor)\n";
        out << "DNSPort 0.0.0.0:" << (spinDnsPort ? spinDnsPort->value() : 5353) << "\n";
    }

    configFile.close();
    addLogMessage("Конфигурация Tor создана: " + torrcPath, "info");
}

QString MainWindow::findLyrebirdPath()
{
    QStringList possiblePaths = {
        "/usr/bin/lyrebird",
        "/usr/local/bin/lyrebird",
        "/usr/sbin/lyrebird",
        "/snap/bin/lyrebird"
    };

    QStringList fallbackPaths = {
        "/usr/bin/obfs4proxy",
        "/usr/local/bin/obfs4proxy"
    };

    for (const QString &path : possiblePaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }

    QProcess which;
    which.start("which", QStringList() << "lyrebird");
    which.waitForFinished(2000);
    if (which.exitCode() == 0) {
        QString result = QString::fromUtf8(which.readAllStandardOutput()).trimmed();
        if (!result.isEmpty() && QFile::exists(result)) {
            return result;
        }
    }

    for (const QString &path : fallbackPaths) {
        if (QFile::exists(path)) {
            addLogMessage("lyrebird не найден, используется obfs4proxy", "warning");
            return path;
        }
    }

    return QString();
}

bool MainWindow::checkTorInstalled()
{
    if (torExecutablePath.isEmpty()) {
        QStringList possiblePaths = {
            "/usr/bin/tor",
            "/usr/local/bin/tor",
            "/usr/sbin/tor"
        };

        for (const QString &path : possiblePaths) {
            if (QFile::exists(path)) {
                torExecutablePath = path;
                txtTorPath->setText(path);
                return true;
            }
        }
        return false;
    }

    return QFile::exists(torExecutablePath);
}

bool MainWindow::checkOpenVPNInstalled()
{
    if (openVPNExecutablePath.isEmpty()) {
        QStringList possiblePaths = {
            "/usr/sbin/openvpn",
            "/usr/bin/openvpn",
            "/usr/local/sbin/openvpn"
        };

        for (const QString &path : possiblePaths) {
            if (QFile::exists(path)) {
                openVPNExecutablePath = path;
                txtOpenVPNPath->setText(path);
                return true;
            }
        }
        return false;
    }

    return QFile::exists(openVPNExecutablePath);
}

// ========== УПРАВЛЕНИЕ МОСТАМИ ==========

void MainWindow::addBridge()
{
    QString bridgeType = cboBridgeType->currentText();
    if (bridgeType == "Нет") {
        QMessageBox::information(this, "Информация", "Пожалуйста, выберите тип моста сначала.");
        return;
    }

    QString actualType = bridgeType;
    if (bridgeType == "obfs4 (lyrebird)") actualType = "obfs4";
    else if (bridgeType == "Автоопределение") actualType = "auto";

    QString hint;
    if (actualType == "obfs4") {
        hint = "Пример: obfs4 192.95.36.142:443 CDF2E852BF539B82BD10E27E9115A31734E378C2 cert=... iat-mode=0";
    } else if (actualType == "webtunnel") {
        hint = "Пример: webtunnel [2001:db8::1]:443 2852538D49D7D73C1A6694FC492104983A9C4FA2 url=https://... ver=0.0.3";
    } else if (actualType == "snowflake") {
        hint = "Пример: snowflake 192.0.2.3:80 2B280B23E1107BB62ABFC40DDCC8824814F80A72";
    } else {
        hint = "Введите строку моста (формат определится автоматически)";
    }

    bool ok;
    QString bridge = QInputDialog::getMultiLineText(this, "Добавить мост",
                                                    "Введите строку моста:\n\n" + hint,
                                                    "", &ok);

    if (ok && !bridge.isEmpty()) {
        QStringList lines = bridge.split('\n', Qt::SkipEmptyParts);
        int added = 0;

        for (QString line : lines) {
            line = line.trimmed();
            if (line.isEmpty()) continue;

            QString detectedType = detectBridgeType(line);
            bool valid = false;

            if (detectedType == "obfs4") {
                valid = validateObfs4Bridge(line);
            } else if (detectedType == "webtunnel") {
                valid = validateWebtunnelBridge(line);
            } else if (detectedType != "unknown") {
                valid = true;
            }

            if (valid || detectedType != "unknown") {
                QString normalized = normalizeBridgeLine(line);
                if (!configuredBridges.contains(normalized)) {
                    configuredBridges.append(normalized);
                    lstBridges->addItem(normalized);
                    added++;
                }
            } else {
                addLogMessage("Неверный формат моста: " + line.left(50) + "...", "error");
            }
        }

        updateBridgeStats();
        saveBridgesToSettings();

        if (added > 0) {
            addLogMessage(QString("Добавлено мостов: %1").arg(added), "info");

            if (torRunning) {
                if (QMessageBox::question(this, "Перезапуск Tor",
                    "Мосты добавлены. Перезапустить Tor для применения изменений?") == QMessageBox::Yes) {
                    restartTor();
                    }
            }
        }
    }
}

void MainWindow::removeBridge()
{
    QList<QListWidgetItem*> items = lstBridges->selectedItems();
    if (items.isEmpty()) {
        QMessageBox::information(this, "Информация", "Выберите мост для удаления.");
        return;
    }

    for (QListWidgetItem *item : items) {
        QString bridge = item->text();
        configuredBridges.removeAll(bridge);
        delete item;
    }

    updateBridgeStats();
    saveBridgesToSettings();

    if (torRunning) {
        if (QMessageBox::question(this, "Перезапуск Tor",
            "Мосты удалены. Перезапустить Tor для применения изменений?") == QMessageBox::Yes) {
            restartTor();
            }
    }
}

void MainWindow::importBridgesFromText()
{
    bool ok;
    QString text = QInputDialog::getMultiLineText(this, "Импорт мостов",
                                                  "Вставьте строки мостов (по одной на строку):\n\n"
                                                  "Примеры:\n"
                                                  "obfs4 192.95.36.142:443 CDF2E852BF539B82BD10E27E9115A31734E378C2 cert=... iat-mode=0\n"
                                                  "webtunnel [2001:db8::1]:443 2852538D49D7D73C1A6694FC492104983A9C4FA2 url=https://... ver=0.0.3\n"
                                                  "snowflake 192.0.2.3:80 2B280B23E1107BB62ABFC40DDCC8824814F80A72",
                                                  "", &ok);

    if (!ok || text.isEmpty()) return;

    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    int added = 0;

    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        QString bridgeType = detectBridgeType(line);
        if (bridgeType == "unknown") continue;

        bool valid = false;
        if (bridgeType == "obfs4") {
            valid = validateObfs4Bridge(line);
        } else if (bridgeType == "webtunnel") {
            valid = validateWebtunnelBridge(line);
        } else {
            valid = true;
        }

        if (valid) {
            QString normalized = normalizeBridgeLine(line);
            if (!configuredBridges.contains(normalized)) {
                configuredBridges.append(normalized);
                lstBridges->addItem(normalized);
                added++;
            }
        }
    }

    updateBridgeStats();
    saveBridgesToSettings();

    if (added > 0 && torRunning) {
        if (QMessageBox::question(this, "Перезапуск Tor",
            "Мосты добавлены. Перезапустить Tor для применения изменений?") == QMessageBox::Yes) {
            restartTor();
            }
    }
}

void MainWindow::validateBridgeFormat()
{
    auto item = lstBridges->currentItem();
    if (!item) {
        QMessageBox::information(this, "Информация", "Выберите мост для проверки.");
        return;
    }

    QString bridge = item->text();
    QString bridgeType = detectBridgeType(bridge);
    bool valid = false;
    QString message;

    if (bridgeType == "obfs4") {
        valid = validateObfs4Bridge(bridge);
        message = valid ? "✓ obfs4 мост имеет правильный формат" : "✗ obfs4 мост имеет неверный формат";
    } else if (bridgeType == "webtunnel") {
        valid = validateWebtunnelBridge(bridge);
        message = valid ? "✓ webtunnel мост имеет правильный формат" : "✗ webtunnel мост имеет неверный формат";
    } else if (bridgeType == "snowflake") {
        message = "✓ snowflake мост (формат не проверялся)";
        valid = true;
    } else {
        message = "✗ Неизвестный тип моста";
    }

    QMessageBox::information(this, "Результат проверки", message);

    if (valid) {
        item->setBackground(QColor(200, 255, 200));
    } else {
        item->setBackground(QColor(255, 200, 200));
    }
}

void MainWindow::testBridgeConnection(const QString &bridge)
{
    addLogMessage("Тестирование моста: " + bridge.left(50) + "...", "info");

    QString bridgeType = detectBridgeType(bridge);
    if (bridgeType == "unknown") {
        addLogMessage("Неизвестный тип моста", "error");
        return;
    }

    if (!checkTransportPluginInstalled(bridgeType)) {
        QString message = QString("Плагин транспорта %1 не найден.\n").arg(bridgeType);
        message += "Установите lyrebird для поддержки всех транспортов.";
        QMessageBox::warning(this, "Транспорт не найден", message);
        return;
    }

    // ── Диагностика webtunnel ─────────────────────────────────────────────
    if (bridgeType == "webtunnel") {
        addLogMessage("🔍 Диагностика webtunnel моста...", "info");

        // Извлекаем url= параметр
        QRegularExpression urlRe(R"(url=(https?://[^\s]+))");
        auto urlMatch = urlRe.match(bridge);
        if (!urlMatch.hasMatch()) {
            addLogMessage("❌ webtunnel мост не содержит url= параметр", "error");
            QMessageBox::warning(this, "Ошибка моста",
                "webtunnel мост должен содержать параметр url=https://...\n\n"
                "Пример корректного формата:\n"
                "webtunnel [IP]:443 FINGERPRINT url=https://domain.example/path ver=0.0.3");
            return;
        }
        QString url = urlMatch.captured(1);
        addLogMessage("   URL: " + url, "info");

        // Извлекаем домен из URL
        QRegularExpression domainRe(R"(https?://([^/:]+))");
        auto domainMatch = domainRe.match(url);
        if (domainMatch.hasMatch()) {
            QString domain = domainMatch.captured(1);
            addLogMessage("   Проверяю DNS для домена: " + domain, "info");

            // DNS резолвинг
            QProcess nslookup;
            nslookup.start("nslookup", {domain, "8.8.8.8"});
            nslookup.waitForFinished(5000);
            QString dnsOut = nslookup.readAllStandardOutput() + nslookup.readAllStandardError();

            if (nslookup.exitCode() == 0 && !dnsOut.contains("NXDOMAIN") && !dnsOut.contains("no such")) {
                addLogMessage("   ✅ DNS: домен " + domain + " резолвится", "success");
            } else {
                addLogMessage("   ❌ DNS: домен " + domain + " НЕ резолвится!", "error");
                addLogMessage("   Возможные причины: домен заблокирован, не существует или DNS недоступен", "warning");
                QMessageBox::warning(this, "DNS недоступен",
                    QString("Домен '%1' не резолвится через DNS.\n\n"
                    "webtunnel требует доступа к домену в url=.\n\n"
                    "Проверьте:\n"
                    "• Домен существует и не заблокирован\n"
                    "• Интернет работает\n"
                    "• Попробуйте другой мост с рабочим url=").arg(domain));
                return;
            }

            // Проверяем TCP соединение к серверу (порт из адреса моста)
            QStringList parts = bridge.split(' ', Qt::SkipEmptyParts);
            // Адрес — второй токен (первый — "webtunnel")
            int addrIdx = (parts[0].toLower() == "webtunnel") ? 1 : 0;
            if (parts.size() > addrIdx) {
                QString addr = parts[addrIdx];
                // Извлекаем порт
                QRegularExpression portRe(R"(]:(\d+)$|:(\d+)$)");
                auto portMatch = portRe.match(addr);
                if (portMatch.hasMatch()) {
                    int port = portMatch.captured(1).isEmpty()
                               ? portMatch.captured(2).toInt()
                               : portMatch.captured(1).toInt();
                    addLogMessage(QString("   Проверяю TCP %1:%2...").arg(domain).arg(port), "info");

                    QTcpSocket sock;
                    sock.connectToHost(domain, port);
                    if (sock.waitForConnected(5000)) {
                        sock.close();
                        addLogMessage(QString("   ✅ TCP %1:%2 — доступен").arg(domain).arg(port), "success");
                    } else {
                        addLogMessage(QString("   ❌ TCP %1:%2 — недоступен: %3")
                            .arg(domain).arg(port).arg(sock.errorString()), "warning");
                    }
                }
            }
        }

        // Проверяем ver= параметр
        QRegularExpression verRe(R"(ver=(\S+))");
        auto verMatch = verRe.match(bridge);
        if (verMatch.hasMatch()) {
            addLogMessage("   Версия протокола: " + verMatch.captured(1), "info");
        } else {
            addLogMessage("   ⚠️ Не указан ver= параметр (рекомендуется ver=0.0.3)", "warning");
        }

        addLogMessage("🔍 Диагностика webtunnel завершена", "info");
        return;
    }

    // ── Диагностика обычных мостов (obfs4 и др.) ─────────────────────────
    QStringList parts = bridge.split(' ');
    int addrIdx = (parts.size() > 1 && parts[0].toLower() == bridgeType) ? 1 : 0;
    if (parts.size() > addrIdx) {
        QString address = parts[addrIdx];
        if (address.contains(':')) {
            QString host = address;
            // Убираем порт
            int lastColon = address.lastIndexOf(':');
            if (address.startsWith('[')) {
                // IPv6: [addr]:port
                int closeBracket = address.indexOf(']');
                host = address.mid(1, closeBracket - 1);
            } else {
                host = address.left(lastColon);
            }

            #ifdef Q_OS_LINUX
            QProcess ping;
            bool isIPv6 = host.contains(':');
            if (isIPv6) {
                ping.start("ping6", {"-c", "2", "-W", "2", host});
            } else {
                ping.start("ping",  {"-c", "2", "-W", "2", host});
            }
            ping.waitForFinished(5000);
            if (ping.exitCode() == 0) {
                addLogMessage("✅ Хост " + host + " доступен", "success");
            } else {
                addLogMessage("❌ Хост " + host + " недоступен (ping)", "warning");
                addLogMessage("   Примечание: Tor скрывает реальные IP в логах (2001:db8::), "
                              "реальный IP может быть другим", "info");
            }
            #endif
        }
    }
}

void MainWindow::updateBridgeConfig()
{
    if (torRunning) {
        if (QMessageBox::question(this, "Требуется перезапуск",
            "Конфигурация мостов изменена. Перезапустить Tor для применения изменений?") == QMessageBox::Yes) {
            restartTor();
            }
    } else {
        createTorConfig();
    }
}

void MainWindow::updateRegistryFromTelegram(const QString &cn, const ClientRecord &record)
{
    QMutexLocker locker(&registryMutex);

    if (clientRegistry.contains(cn)) {
        // Обновляем существующую запись
        ClientRecord &existing = clientRegistry[cn];
        existing.telegramId = record.telegramId;
        existing.registeredAt = record.registeredAt;
        existing.expiryDate = record.expiryDate;
        existing.displayName = record.displayName;
        existing.isBanned = record.isBanned;
    } else {
        // Добавляем новую запись
        clientRegistry[cn] = record;
    }

    // Сохраняем на диск
    saveClientRegistry();

    // Уведомляем всех подписчиков (включая сам бот)
    emit registryUpdated(clientRegistry);
}

QString MainWindow::detectBridgeType(const QString &bridgeLine)
{
    QString line = bridgeLine.trimmed();

    if (line.startsWith("obfs4", Qt::CaseInsensitive)) {
        return "obfs4";
    } else if (line.startsWith("webtunnel", Qt::CaseInsensitive)) {
        return "webtunnel";
    } else if (line.startsWith("snowflake", Qt::CaseInsensitive)) {
        return "snowflake";
    }

    if (line.contains("cert=") && line.contains("iat-mode=")) {
        return "obfs4";
    } else if (line.contains("url=") && line.contains("ver=")) {
        return "webtunnel";
    }

    return "unknown";
}

bool MainWindow::validateWebtunnelBridge(const QString &bridge)
{
    QString line = bridge.trimmed();
    if (line.startsWith("webtunnel ", Qt::CaseInsensitive)) {
        line = line.mid(10).trimmed();
    }

    QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 3) return false;

    QString address = parts[0];
    QRegularExpression ipv6Regex("^\\[[0-9a-fA-F:]+\\]:\\d+$");
    QRegularExpression ipv4Regex("^\\d+\\.\\d+\\.\\d+\\.\\d+:\\d+$");

    if (!ipv6Regex.match(address).hasMatch() && !ipv4Regex.match(address).hasMatch()) {
        return false;
    }

    bool hasUrl = false;
    bool hasVer = false;

    for (int i = 2; i < parts.size(); i++) {
        if (parts[i].startsWith("url=")) hasUrl = true;
        if (parts[i].startsWith("ver=")) hasVer = true;
    }

    return hasUrl && hasVer;
}

bool MainWindow::validateObfs4Bridge(const QString &bridge)
{
    QString line = bridge.trimmed();
    if (line.startsWith("obfs4 ", Qt::CaseInsensitive)) {
        line = line.mid(6).trimmed();
    }

    QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 3) return false;

    bool hasCert = false;
    for (int i = 2; i < parts.size(); i++) {
        if (parts[i].startsWith("cert=")) {
            hasCert = true;
            break;
        }
    }

    return hasCert;
}

QString MainWindow::normalizeBridgeLine(const QString &bridge)
{
    QString normalized = bridge.trimmed().simplified();
    QString detectedType = detectBridgeType(normalized);

    // Добавляем тип транспорта если отсутствует
    if (detectedType == "webtunnel" && !normalized.startsWith("webtunnel", Qt::CaseInsensitive)) {
        normalized = "webtunnel " + normalized;
    } else if (detectedType == "obfs4" && !normalized.startsWith("obfs4", Qt::CaseInsensitive)) {
        normalized = "obfs4 " + normalized;
    } else if (detectedType == "snowflake" && !normalized.startsWith("snowflake", Qt::CaseInsensitive)) {
        normalized = "snowflake " + normalized;
    }

    // Для webtunnel: проверяем наличие ver= и добавляем если нет
    if (detectedType == "webtunnel" && !normalized.contains("ver=")) {
        normalized += " ver=0.0.3";
        addLogMessage("ℹ️ webtunnel: добавлен параметр ver=0.0.3 по умолчанию", "info");
    }

    return normalized;
}

void MainWindow::updateBridgeStats()
{
    int count = configuredBridges.size();
    lblBridgeStats->setText(QString("Мосты: %1 настроено").arg(count));
}

void MainWindow::saveBridgesToSettings()
{
    settings->setValue("tor/bridges", configuredBridges);
    settings->setValue("tor/bridgeType", cboBridgeType->currentText());
    settings->sync();
}

void MainWindow::loadBridgesFromSettings()
{
    configuredBridges = settings->value("tor/bridges").toStringList();
    lstBridges->clear();
    lstBridges->addItems(configuredBridges);

    QString bridgeType = settings->value("tor/bridgeType", "Нет").toString();
    int index = cboBridgeType->findText(bridgeType);
    if (index >= 0) {
        cboBridgeType->setCurrentIndex(index);
    }

    updateBridgeStats();
}

bool MainWindow::checkTransportPluginInstalled(const QString &transport)
{
    QString lyrebirdPath = findLyrebirdPath();
    if (!lyrebirdPath.isEmpty()) {
        transportPluginPaths[transport] = lyrebirdPath;
        return true;
    }

    QStringList possiblePaths;
    if (transport == "obfs4") {
        possiblePaths << "/usr/bin/obfs4proxy" << "/usr/local/bin/obfs4proxy";
    } else if (transport == "webtunnel") {
        possiblePaths << "/usr/bin/webtunnel" << "/usr/local/bin/webtunnel";
    } else if (transport == "snowflake") {
        possiblePaths << "/usr/bin/snowflake-client" << "/usr/local/bin/snowflake-client";
    }

    for (const QString &path : possiblePaths) {
        if (QFile::exists(path)) {
            transportPluginPaths[transport] = path;
            return true;
        }
    }

    return false;
}

QString MainWindow::getTransportPluginPath(const QString &transport)
{
    return transportPluginPaths.value(transport);
}

// ========== KILL SWITCH ==========

void MainWindow::enableKillSwitch()
{
    addLogMessage("Включение kill switch...", "info");
    setupFirewallRules(true);
    killSwitchEnabled = true;

    trayIcon->showMessage("Kill Switch",
                          "Kill switch включен. Весь трафик кроме Tor будет заблокирован.",
                          QSystemTrayIcon::Information, 3000);
}

void MainWindow::disableKillSwitch()
{
    addLogMessage("Отключение kill switch...", "info");
    setupFirewallRules(false);
    killSwitchEnabled = false;
}

void MainWindow::setupFirewallRules(bool enable)
{
    #ifdef Q_OS_LINUX
    // Вспомогательная лямбда — запускает iptables напрямую через аргументы (без sh -c)
    auto runIptables = [this](const QStringList &args) {
        QProcess p;
        QStringList fullArgs;
        if (geteuid() != 0) {
            if (QFile::exists("/usr/bin/pkexec")) {
                p.start("pkexec", QStringList() << "iptables" << args);
            } else {
                p.start("sudo", QStringList() << "-n" << "iptables" << args);
            }
        } else {
            p.start("iptables", args);
        }
        if (!p.waitForFinished(5000)) {
            p.kill();
            addLogMessage("setupFirewallRules: таймаут iptables " + args.join(' '), "warning");
        }
        return p.exitCode();
    };

    auto runIp6tables = [this](const QStringList &args) {
        QProcess p;
        if (geteuid() != 0) {
            if (QFile::exists("/usr/bin/pkexec")) {
                p.start("pkexec", QStringList() << "ip6tables" << args);
            } else {
                p.start("sudo", QStringList() << "-n" << "ip6tables" << args);
            }
        } else {
            p.start("ip6tables", args);
        }
        if (!p.waitForFinished(5000)) { p.kill(); }
        return p.exitCode();
    };

    if (enable) {
        runIptables({"-F", "OUTPUT"});
        runIptables({"-P", "OUTPUT", "DROP"});
        runIptables({"-A", "OUTPUT", "-o", "lo", "-j", "ACCEPT"});
        runIptables({"-A", "OUTPUT", "-p", "tcp", "--dport",
            QString::number(spinTorSocksPort->value()), "-j", "ACCEPT"});
        runIptables({"-A", "OUTPUT", "-p", "tcp", "--dport",
            QString::number(spinTorControlPort->value()), "-j", "ACCEPT"});
        runIptables({"-A", "OUTPUT", "-o", "tun+", "-j", "ACCEPT"});
        runIptables({"-A", "OUTPUT", "-m", "state", "--state", "ESTABLISHED,RELATED", "-j", "ACCEPT"});

        if (chkBlockIPv6 && chkBlockIPv6->isChecked()) {
            runIp6tables({"-P", "OUTPUT", "DROP"});
        }
    } else {
        runIptables({"-F", "OUTPUT"});
        runIptables({"-P", "OUTPUT", "ACCEPT"});
        runIp6tables({"-P", "OUTPUT", "ACCEPT"});
    }
    #else
    Q_UNUSED(enable);
    #endif
}

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========

QString MainWindow::findEasyRSA()
{
    QStringList paths = {
        "/usr/share/easy-rsa/easyrsa",
        "/usr/local/share/easy-rsa/easyrsa",
        "/usr/bin/easyrsa"
    };

    for (const QString &path : paths) {
        if (QFile::exists(path)) {
            return path;
        }
    }

    return QString();
}

QString MainWindow::getLocalIP()
{
    QString ip;
    #ifdef Q_OS_LINUX
    ip = executeCommand("hostname -I | awk '{print $1}'").trimmed();
    #endif

    if (ip.isEmpty()) {
        QTcpSocket socket;
        socket.connectToHost("8.8.8.8", 53);
        if (socket.waitForConnected(3000)) {
            ip = socket.localAddress().toString();
            socket.disconnectFromHost();
        }
    }

    return ip;
}

void MainWindow::updateClientStats()
{
    // Устаревший метод, заменён на updateClientsTable
    updateClientsTable();
}

bool MainWindow::isProcessRunning(const QString &processName)
{
    #ifdef Q_OS_LINUX
    // Валидация: имя процесса не должно содержать опасных символов
    static QRegularExpression safeNameRe("^[a-zA-Z0-9_\\-\\.]+$");
    if (!safeNameRe.match(processName).hasMatch()) {
        return false;
    }
    QProcess process;
    process.start("pgrep", QStringList() << "-x" << processName);
    if (!process.waitForFinished(3000)) {
        process.kill();
        return false;
    }
    return process.exitCode() == 0;
    #else
    Q_UNUSED(processName);
    return false;
    #endif
}

QString MainWindow::executeCommand(const QString &command)
{
    QProcess process;
    #ifdef Q_OS_LINUX
    if (geteuid() != 0 && !command.startsWith("pkexec") && !command.startsWith("sudo")) {
        if (QFile::exists("/usr/bin/pkexec")) {
            process.start("pkexec", QStringList() << "sh" << "-c" << command);
        } else if (QFile::exists("/usr/bin/sudo")) {
            process.start("sudo", QStringList() << "sh" << "-c" << command);
        } else {
            process.start("sh", QStringList() << "-c" << command);
        }
    } else {
        process.start("sh", QStringList() << "-c" << command);
    }
    #else
    process.start("sh", QStringList() << "-c" << command);
    #endif

    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished(2000);
        addLogMessage("executeCommand: timeout для команды: " + command.left(80), "warning");
        return QString();
    }

    // Логируем ненулевые коды завершения
    int code = process.exitCode();
    if (code != 0 && process.exitStatus() == QProcess::NormalExit) {
        // Не логируем grep (exitCode=1 = «не найдено», это нормально)
        if (!command.contains("grep") && !command.contains("| grep")) {
            addLogMessage(QString("executeCommand: код %1 для: %2").arg(code).arg(command.left(80)), "warning");
        }
    }

    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

// Безопасный запуск системных команд через список аргументов — без shell-интерпретации.
// Использовать вместо executeCommand для команд с пользовательскими данными (iptables, sysctl и т.д.)
QString MainWindow::runSafeCommand(const QString &program, const QStringList &args)
{
    if (program.isEmpty() || !QFile::exists(program)) {
        addLogMessage("runSafeCommand: программа не найдена: " + program, "warning");
        return QString();
    }

    QProcess process;
    const int TIMEOUT_MS = 10000;

    #ifdef Q_OS_LINUX
    if (geteuid() != 0) {
        QStringList fullArgs;
        if (QFile::exists("/usr/bin/pkexec")) {
            fullArgs << program << args;
            process.start("/usr/bin/pkexec", fullArgs);
        } else if (QFile::exists("/usr/bin/sudo")) {
            fullArgs << program << args;
            process.start("/usr/bin/sudo", fullArgs);
        } else {
            process.start(program, args);
        }
    } else {
        process.start(program, args);
    }
    #else
    process.start(program, args);
    #endif

    if (!process.waitForFinished(TIMEOUT_MS)) {
        addLogMessage("runSafeCommand timeout: " + program, "warning");
        process.kill();
        return QString();
    }

    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

void MainWindow::setConnectionState(const QString &state)
{
    currentConnectionState = state;

    // Вычисляем uptime сервера если он запущен
    QString uptimeStr;
    if (state == "server_mode" && serverStartTime.isValid()) {
        qint64 secs = serverStartTime.secsTo(QDateTime::currentDateTime());
        if (secs < 60)
            uptimeStr = QString("  ↑ %1 сек").arg(secs);
        else if (secs < 3600)
            uptimeStr = QString("  ↑ %1 мин").arg(secs / 60);
        else
            uptimeStr = QString("  ↑ %1 ч %2 мин").arg(secs / 3600).arg((secs % 3600) / 60);
    }

    if (state == "disconnected") {
        statusBar()->showMessage("● Tor: отключён  |  Сервер: остановлен");
        statusBar()->setStyleSheet("color: #888;");
    } else if (state == "tor_only") {
        statusBar()->showMessage("🟢 Tor: активен  |  Сервер: остановлен");
        statusBar()->setStyleSheet("background-color: #e8f5e9; color: #2e7d32; font-weight: bold;");
    } else if (state == "server_mode") {
        statusBar()->showMessage("🟢 Tor: активен  |  🟢 Сервер: запущен" + uptimeStr);
        statusBar()->setStyleSheet("background-color: #e3f2fd; color: #1565c0; font-weight: bold;");
    }
}

bool MainWindow::verifyTorConnection()
{
    if (!torRunning) return false;

    QTcpSocket testSocket;
    testSocket.connectToHost("127.0.0.1", spinTorSocksPort->value());

    if (testSocket.waitForConnected(3000)) {
        testSocket.disconnectFromHost();
        return true;
    }

    return false;
}

void MainWindow::updateStatus()
{
    if (torRunning && verifyTorConnection()) {
        lblTorStatus->setText("Статус: <b style='color:green;'>Подключен</b>");
    } else if (torRunning) {
        lblTorStatus->setText("Статус: <b style='color:orange;'>Запуск...</b>");
    } else {
        lblTorStatus->setText("Статус: <b style='color:red;'>Отключен</b>");
    }

    // Переобновляем строку состояния (с uptime если сервер запущен)
    if (serverMode) {
        setConnectionState("server_mode");
    } else if (torRunning) {
        setConnectionState("tor_only");
    } else {
        setConnectionState("disconnected");
    }
}

void MainWindow::updateTrafficStats()
{
    if (!controlSocketConnected) return;

    sendTorCommand("GETINFO traffic/read");
    sendTorCommand("GETINFO traffic/written");

    QString readableRx = QString::number(bytesReceived / 1024.0, 'f', 2) + " КБ";
    QString readableTx = QString::number(bytesSent / 1024.0, 'f', 2) + " КБ";

    if (bytesReceived > 1024 * 1024) {
        readableRx = QString::number(bytesReceived / (1024.0 * 1024.0), 'f', 2) + " МБ";
    }
    if (bytesSent > 1024 * 1024) {
        readableTx = QString::number(bytesSent / (1024.0 * 1024.0), 'f', 2) + " МБ";
    }

    lblTrafficStats->setText("Трафик: ↓ " + readableRx + " ↑ " + readableTx);
}

void MainWindow::addLogMessage(const QString &message, const QString &type)
{
    // Фильтрация по уровню
    if (type != "success") {
        auto levelRank = [](const QString &t) -> int {
            if (t == "error")   return 3;
            if (t == "warning") return 2;
            if (t == "info")    return 1;
            return 0;
        };
        int minRank = levelRank(logLevel);
        int msgRank = levelRank(type);
        if (msgRank < minRank) return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
    QString typeStr;

    if (type == "error") typeStr = "ОШИБКА";
    else if (type == "warning") typeStr = "ПРЕДУПР";
    else if (type == "info") typeStr = "ИНФО";
    else if (type == "success") typeStr = "УСПЕХ";
    else typeStr = "ЛОГ";

    QString formattedMsg = QString("[%1] [%2] %3").arg(timestamp, typeStr, message);

    // Добавляем в буфер (это потокобезопасно)
    {
        QMutexLocker locker(&logMutex);
        logBuffer.append(formattedMsg + "|" + type);
    }

    // Статус-бар (только из главного потока)
    if (QThread::currentThread() == this->thread()) {
        if (type == "error") {
            statusBar()->showMessage("⛔ " + message.left(120), 5000);
        } else if (type == "warning") {
            statusBar()->showMessage("⚠️ " + message.left(120), 3000);
        }
    } else {
        // Если не в главном потоке - отправляем сигнал
        QMetaObject::invokeMethod(this, [this, message, type]() {
            if (type == "error") {
                statusBar()->showMessage("⛔ " + message.left(120), 5000);
            } else if (type == "warning") {
                statusBar()->showMessage("⚠️ " + message.left(120), 3000);
            }
        }, Qt::QueuedConnection);
    }

    // Запись в файл (можно из любого потока)
    static const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    bool writeAll = chkWriteAllLogs ? chkWriteAllLogs->isChecked() : false;
    bool shouldWrite = (type == "error" || type == "warning") || writeAll;

    if (shouldWrite) {
        QString mainLogPath = appData + "/tormanager.log";
        QFile mainLog(mainLogPath);
        if (mainLog.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&mainLog);
            out << "[" << timestamp << "] [" << typeStr << "] " << message << "\n";
            mainLog.close();
        }
    }

    if (type == "error") {
        QString errLogPath = appData + "/tormanager_errors.log";
        QFile errLog(errLogPath);
        if (errLog.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&errLog);
            out << "[" << timestamp << "] " << message << "\n";
            errLog.close();
        }
    }

    // Уведомление в трее (только из главного потока)
    bool trayNotify = chkShowTrayNotifications ? chkShowTrayNotifications->isChecked() : true;
    if (trayNotify && trayIcon && type == "error") {
        if (QThread::currentThread() == this->thread()) {
            trayIcon->showMessage("Tor Manager — Ошибка", message.left(200),
                                  QSystemTrayIcon::Critical, 4000);
        } else {
            QMetaObject::invokeMethod(this, [this, message]() {
                trayIcon->showMessage("Tor Manager — Ошибка", message.left(200),
                                      QSystemTrayIcon::Critical, 4000);
            }, Qt::QueuedConnection);
        }
    }
}

void MainWindow::flushLogs()
{
    if (!txtAllLogs) return;

    QStringList bufferCopy;
    {
        QMutexLocker locker(&logMutex);
        if (logBuffer.isEmpty()) return;
        bufferCopy = logBuffer;
        logBuffer.clear();
    }

    for (const QString &entry : bufferCopy) {
        QStringList parts = entry.split('|');
        if (parts.size() == 2) {
            QString message = parts[0];
            QString type = parts[1];

            QString color;
            QString typeStr;

            if (type == "error") {
                color = "#cc0000";
                typeStr = "ОШИБКА";
            } else if (type == "warning") {
                color = "#e07000";
                typeStr = "ПРЕДУПР";
            } else if (type == "info") {
                color = "#1a5fa8";
                typeStr = "ИНФО";
            } else if (type == "success") {
                color = "#1a7a1a";
                typeStr = "УСПЕХ";
            } else {
                color = "#444444";
                typeStr = "ЛОГ";
            }

            // Извлекаем timestamp из начала сообщения
            QString timestamp;
            QString cleanMessage = message;

            QRegularExpression timestampRe(R"(^\[(\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}:\d{2})\])");
            auto match = timestampRe.match(message);
            if (match.hasMatch()) {
                timestamp = match.captured(1);
                cleanMessage = message.mid(match.capturedLength(0)).trimmed();
            } else {
                timestamp = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
            }

            QString formattedMsg = QString(
                "<span style='color:%1;'>[%2] [<b>%3</b>] %4</span><br>")
                .arg(color, timestamp, typeStr, cleanMessage.toHtmlEscaped());

            txtAllLogs->append(formattedMsg);
        }
    }

    // Ротация логов
    if (txtAllLogs->document()->lineCount() > MAX_LOG_LINES) {
        QTextCursor removeCursor(txtAllLogs->document());
        removeCursor.movePosition(QTextCursor::Start);
        removeCursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 1000);
        removeCursor.removeSelectedText();
    }

    // Автоскролл
    QTextCursor cursor = txtAllLogs->textCursor();
    cursor.movePosition(QTextCursor::End);
    txtAllLogs->setTextCursor(cursor);
}

// ========== НАСТРОЙКИ ==========

void MainWindow::loadSettings()
{
    if (!settings) return;

    spinTorSocksPort->setValue(settings->value("tor/socksPort", DEFAULT_TOR_SOCKS_PORT).toInt());
    spinTorControlPort->setValue(settings->value("tor/controlPort", DEFAULT_TOR_CONTROL_PORT).toInt());

    // Внешний доступ
    if (chkExternalSocks) chkExternalSocks->setChecked(settings->value("tor/externalSocks", false).toBool());
    if (chkExternalTrans) {
        bool trans = settings->value("tor/externalTrans", true).toBool();
        chkExternalTrans->setChecked(trans);
        if (spinTransPort) spinTransPort->setEnabled(trans);
    }
    if (chkExternalDns) {
        bool dns = settings->value("tor/externalDns", true).toBool();
        chkExternalDns->setChecked(dns);
        if (spinDnsPort) spinDnsPort->setEnabled(dns);
    }
    if (spinTransPort) spinTransPort->setValue(settings->value("tor/transPort", 9040).toInt());
    if (spinDnsPort)   spinDnsPort->setValue(settings->value("tor/dnsPort", 5353).toInt());

    torExecutablePath = settings->value("tor/executablePath", "/usr/bin/tor").toString();
    txtTorPath->setText(torExecutablePath);

    openVPNExecutablePath = settings->value("vpn/executablePath", "/usr/sbin/openvpn").toString();
    txtOpenVPNPath->setText(openVPNExecutablePath);

    chkKillSwitch->setChecked(settings->value("security/killSwitch", false).toBool());
    chkBlockIPv6->setChecked(settings->value("security/blockIPv6", true).toBool());
    chkDNSLeakProtection->setChecked(settings->value("security/dnsProtection", true).toBool());

    chkAutoStart->setChecked(settings->value("general/autoStart", false).toBool());
    chkStartMinimized->setChecked(settings->value("general/startMinimized", false).toBool());

    spinServerPort->setValue(settings->value("server/port", DEFAULT_VPN_SERVER_PORT).toInt());
    txtServerNetwork->setText(settings->value("server/network", "10.8.0.0 255.255.255.0").toString());
    chkRouteThroughTor->setChecked(settings->value("server/routeThroughTor", true).toBool());

    // Security Settings
    if (cboServerProto)    cboServerProto->setCurrentText(settings->value("server/proto", "TCP").toString());
    if (cboDataCipher)     cboDataCipher->setCurrentText(settings->value("server/dataCipher", "AES-256-GCM").toString());
    if (cboTlsCipher)      cboTlsCipher->setCurrentIndex(settings->value("server/tlsCipherIdx", 0).toInt());
    if (cboHmacAuth)       cboHmacAuth->setCurrentText(settings->value("server/hmacAuth", "SHA256").toString());
    if (cboTlsMinVer)      cboTlsMinVer->setCurrentText(settings->value("server/tlsMinVer", "1.2").toString());
    if (cboTlsMode)        cboTlsMode->setCurrentIndex(settings->value("server/tlsMode", 1).toInt());
    if (chkPfs)            chkPfs->setChecked(settings->value("server/pfs", true).toBool());
    if (chkCompression)    chkCompression->setChecked(settings->value("server/compression", false).toBool());
    if (chkDuplicateCN)    chkDuplicateCN->setChecked(settings->value("server/duplicateCN", true).toBool());
    if (chkClientToClient) chkClientToClient->setChecked(settings->value("server/clientToClient", false).toBool());
    if (spinMaxClients)    spinMaxClients->setValue(settings->value("server/maxClients", 10).toInt());
    if (spinMtu)           spinMtu->setValue(settings->value("server/mtu", 1500).toInt());

    // Новые настройки
    if (txtServerAddress)
        txtServerAddress->setText(settings->value("server/publicAddress", "").toString());
    if (spinRefreshInterval)
        spinRefreshInterval->setValue(settings->value("general/refreshInterval", 3).toInt());
    if (chkShowTrayNotifications)
        chkShowTrayNotifications->setChecked(settings->value("general/trayNotifications", true).toBool());
    if (chkWriteAllLogs)
        chkWriteAllLogs->setChecked(settings->value("logging/writeAllLogs", false).toBool());
    if (cboLogLevelSetting) {
        QString lvl = settings->value("logging/level", "all").toString();
        logLevel = lvl;
        int idx = 0;
        if      (lvl == "info")    idx = 1;
        else if (lvl == "warning") idx = 2;
        else if (lvl == "error")   idx = 3;
        cboLogLevelSetting->setCurrentIndex(idx);
    }

    // Применяем интервал обновления к таймеру
    if (clientsRefreshTimer && spinRefreshInterval)
        clientsRefreshTimer->setInterval(spinRefreshInterval->value() * 1000);

    loadBridgesFromSettings();
}

void MainWindow::saveSettings()
{
    settings->setValue("tor/socksPort", spinTorSocksPort->value());
    settings->setValue("tor/controlPort", spinTorControlPort->value());
    settings->setValue("tor/executablePath", txtTorPath->text());

    // Внешний доступ
    if (chkExternalSocks) settings->setValue("tor/externalSocks", chkExternalSocks->isChecked());
    if (chkExternalTrans) settings->setValue("tor/externalTrans", chkExternalTrans->isChecked());
    if (chkExternalDns)   settings->setValue("tor/externalDns",   chkExternalDns->isChecked());
    if (spinTransPort)    settings->setValue("tor/transPort",      spinTransPort->value());
    if (spinDnsPort)      settings->setValue("tor/dnsPort",        spinDnsPort->value());

    settings->setValue("vpn/executablePath", txtOpenVPNPath->text());

    settings->setValue("security/killSwitch", chkKillSwitch->isChecked());
    settings->setValue("security/blockIPv6", chkBlockIPv6->isChecked());
    settings->setValue("security/dnsProtection", chkDNSLeakProtection->isChecked());

    settings->setValue("general/autoStart", chkAutoStart->isChecked());
    settings->setValue("general/startMinimized", chkStartMinimized->isChecked());

    settings->setValue("server/port",           spinServerPort->value());
    settings->setValue("server/network",         txtServerNetwork->text());
    settings->setValue("server/routeThroughTor", chkRouteThroughTor->isChecked());

    // Security Settings
    if (cboServerProto)    settings->setValue("server/proto",          cboServerProto->currentText());
    if (cboDataCipher)     settings->setValue("server/dataCipher",     cboDataCipher->currentText());
    if (cboTlsCipher)      settings->setValue("server/tlsCipherIdx",   cboTlsCipher->currentIndex());
    if (cboHmacAuth)       settings->setValue("server/hmacAuth",       cboHmacAuth->currentText());
    if (cboTlsMinVer)      settings->setValue("server/tlsMinVer",      cboTlsMinVer->currentText());
    if (cboTlsMode)        settings->setValue("server/tlsMode",        cboTlsMode->currentIndex());
    if (chkPfs)            settings->setValue("server/pfs",            chkPfs->isChecked());
    if (chkCompression)    settings->setValue("server/compression",    chkCompression->isChecked());
    if (chkDuplicateCN)    settings->setValue("server/duplicateCN",    chkDuplicateCN->isChecked());
    if (chkClientToClient) settings->setValue("server/clientToClient", chkClientToClient->isChecked());
    if (spinMaxClients)    settings->setValue("server/maxClients",     spinMaxClients->value());
    if (spinMtu)           settings->setValue("server/mtu",            spinMtu->value());

    // Новые настройки
    if (txtServerAddress)
        settings->setValue("server/publicAddress", txtServerAddress->text().trimmed());
    if (spinRefreshInterval)
        settings->setValue("general/refreshInterval", spinRefreshInterval->value());
    if (chkShowTrayNotifications)
        settings->setValue("general/trayNotifications", chkShowTrayNotifications->isChecked());
    if (chkWriteAllLogs)
        settings->setValue("logging/writeAllLogs", chkWriteAllLogs->isChecked());
    if (cboLogLevelSetting) {
        static const QStringList levels = {"all", "info", "warning", "error"};
        int idx = cboLogLevelSetting->currentIndex();
        logLevel = (idx >= 0 && idx < levels.size()) ? levels[idx] : "all";
        settings->setValue("logging/level", logLevel);
    }

    saveBridgesToSettings();
    settings->sync();
}

void MainWindow::applySettings()
{
    saveSettings();

    torExecutablePath = txtTorPath->text();
    openVPNExecutablePath = txtOpenVPNPath->text();

    // Применяем новый интервал обновления клиентов
    if (clientsRefreshTimer && spinRefreshInterval) {
        int ms = spinRefreshInterval->value() * 1000;
        clientsRefreshTimer->setInterval(ms);
        addLogMessage(QString("Интервал обновления клиентов: %1 сек").arg(spinRefreshInterval->value()), "info");
    }

    // Синхронизация с Telegram ботом
    syncBotWithSettings();

    QMessageBox::information(this, "Настройки", "Настройки успешно применены.");

    if (chkKillSwitch->isChecked() && !killSwitchEnabled) {
        enableKillSwitch();
    } else if (!chkKillSwitch->isChecked() && killSwitchEnabled) {
        disableKillSwitch();
    }

    if (torRunning) {
        if (QMessageBox::question(this, "Перезапуск Tor",
            "Некоторые настройки требуют перезапуска Tor. Перезапустить сейчас?") == QMessageBox::Yes) {
            restartTor();
            }
    } else {
        createTorConfig();
    }
}

// ========== СЛОТЫ ИНТЕРФЕЙСА ==========

void MainWindow::showSettings()
{
    tabWidget->setCurrentWidget(settingsTab);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "О программе Tor Manager",
                       "<h2>Tor Manager с OpenVPN</h2>"
                       "<p>Версия 1.1</p>"
                       "<p>Приложение на Qt для управления Tor с интегрированной поддержкой OpenVPN сервера.</p>"
                       "<p><b>Возможности:</b></p>"
                       "<ul>"
                       "<li>Запуск/Остановка Tor с пользовательской конфигурацией</li>"
                       "<li>OpenVPN сервер с маршрутизацией трафика через Tor</li>"
                       "<li><b>Управление клиентами — отдельная вкладка с таблицей и журналом</b></li>"
                       "<li>Поддержка мостов (obfs4/lyrebird, webtunnel, snowflake)</li>"
                       "<li>Поддержка IPv6 адресов для webtunnel мостов</li>"
                       "<li>Автоматическая генерация сертификатов</li>"
                       "<li>Kill switch для защиты от утечек</li>"
                       "<li>Мониторинг трафика и клиентов</li>"
                       "<li>Обнаружение утечек IP</li>"
                       "<li>Импорт списков мостов</li>"
                       "<li>Диагностика сервера</li>"
                       "<li>Проверка конфигурации</li>"
                       "<li>Автоматическая настройка маршрутизации</li>"
                       "<li>Проверка и восстановление правил iptables</li>"
                       "</ul>"
                       "<p>Создано с использованием Qt и C++</p>"
                       "<p>© 2026 Проект Tor Manager</p>");
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        setVisible(!isVisible());
        if (isVisible()) {
            activateWindow();
            raise();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible()) {
        hide();
        event->ignore();
        trayIcon->showMessage("Tor Manager",
                              "Приложение продолжает работу в системном трее.",
                              QSystemTrayIcon::Information, 2000);
    }
}

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========

bool MainWindow::copyPath(const QString &src, const QString &dst)
{
    QDir dir(src);
    if (!dir.exists())
        return false;

    QDir().mkpath(dst);

    foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString dst_path = dst + QDir::separator() + d;
        copyPath(src + QDir::separator() + d, dst_path);
    }

    foreach (QString f, dir.entryList(QDir::Files)) {
        QFile::copy(src + QDir::separator() + f, dst + QDir::separator() + f);
    }

    return true;
}

// ========== ДИАГНОСТИКА ==========

void MainWindow::diagnoseConnection()
{
    addLogMessage("=== ДИАГНОСТИКА ПОДКЛЮЧЕНИЯ ===", "info");

    // SECURITY FIX: заменяем shell-конкатенацию на безопасный runSafeCommand
    int diagPort = spinServerPort ? spinServerPort->value() : DEFAULT_VPN_SERVER_PORT;
    QProcess telnetProc;
    telnetProc.start("bash", {"-c",
        QString("timeout 2 bash -c 'cat < /dev/null > /dev/tcp/127.0.0.1/%1' 2>&1 && echo Connected || echo Failed")
        .arg(diagPort)});
    telnetProc.waitForFinished(4000);
    QString telnetTest = QString::fromUtf8(telnetProc.readAllStandardOutput()).trimmed();

    QString portTestResult = telnetTest.contains("Connected") ? "УСПЕХ" : "НЕУДАЧА";
    addLogMessage("Локальный тест порта: " + portTestResult,
                  telnetTest.contains("Connected") ? "success" : "error");

    QString externalIP = executeCommand("curl -s ifconfig.me || curl -s icanhazip.com || echo 'Не удалось определить'");
    addLogMessage("Внешний IP сервера: " + externalIP, "info");

    QString iptables = executeCommand("iptables -L -n | grep -E '(ACCEPT|DROP)' || echo 'Нет правил или нет доступа'");
    addLogMessage("Правила firewall:\n" + iptables, "info");

    // SECURITY FIX: используем QProcess напрямую, не shell
    QProcess portCheckProc;
    portCheckProc.start("sh", {"-c",
        QString("ss -tlnp 2>/dev/null | grep ':%1' || echo 'Порт не прослушивается'")
        .arg(diagPort)});
    portCheckProc.waitForFinished(3000);
    QString portCheck = QString::fromUtf8(portCheckProc.readAllStandardOutput()).trimmed();
    addLogMessage("Прослушивание порта:\n" + portCheck,
                  portCheck.contains("openvpn") ? "success" : "warning");

    QString logFile = "/tmp/openvpn-server.log";
    if (QFile::exists(logFile)) {
        QFile file(logFile);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            QStringList errors;
            QStringList lines = content.split('\n');
            for (const QString &line : lines) {
                if (line.contains("error", Qt::CaseInsensitive) ||
                    line.contains("fail", Qt::CaseInsensitive) ||
                    line.contains("TLS", Qt::CaseInsensitive) ||
                    line.contains("SSL", Qt::CaseInsensitive)) {
                    errors << line;
                    }
            }
            if (!errors.isEmpty()) {
                addLogMessage("Ошибки в логе OpenVPN:", "error");
                for (const QString &err : errors) {
                    addLogMessage("  " + err, "error");
                }
            } else {
                addLogMessage("Ошибок в логе OpenVPN не найдено", "success");
            }
            file.close();
        }
    } else {
        addLogMessage("Лог файл OpenVPN не найден: " + logFile, "warning");
    }

    addLogMessage("Проверка сертификатов:", "info");

    if (QFile::exists(caCertPath)) {
        // SECURITY FIX: используем список аргументов вместо shell-конкатенации путей
        QProcess caInfoProc;
        caInfoProc.start("openssl", {"x509", "-in", caCertPath, "-noout", "-subject", "-issuer", "-dates"});
        caInfoProc.waitForFinished(5000);
        QString caInfo = QString::fromUtf8(caInfoProc.readAllStandardOutput()).trimmed();
        addLogMessage("CA сертификат:\n" + caInfo, "info");
    } else {
        addLogMessage("CA сертификат не найден: " + caCertPath, "error");
    }

    if (QFile::exists(serverCertPath)) {
        // SECURITY FIX: используем список аргументов — пути не интерпретируются shell
        QProcess certVerifyProc;
        certVerifyProc.start("openssl", {"verify", "-CAfile", caCertPath, serverCertPath});
        certVerifyProc.waitForFinished(5000);
        QString certVerify = QString::fromUtf8(certVerifyProc.readAllStandardOutput()).trimmed();
        addLogMessage("Проверка сертификата сервера: " + certVerify,
                      certVerify.contains("OK") ? "success" : "error");

        QProcess certInfoProc;
        certInfoProc.start("openssl", {"x509", "-in", serverCertPath, "-noout", "-subject", "-issuer", "-dates"});
        certInfoProc.waitForFinished(5000);
        QString certInfo = QString::fromUtf8(certInfoProc.readAllStandardOutput()).trimmed();
        addLogMessage("Информация о сертификате сервера:\n" + certInfo, "info");
    } else {
        addLogMessage("Сертификат сервера не найден: " + serverCertPath, "error");
    }

    // Получаем версию OpenVPN безопасно через аргументы (без shell)
    {
        QProcess verProc;
        verProc.start(openVPNExecutablePath, {"--version"});
        if (verProc.waitForFinished(5000)) {
            QString verOut = QString::fromUtf8(verProc.readAllStandardOutput()).trimmed();
            // Берём первые две строки
            QStringList lines = verOut.split('\n');
            QString ovpnVer = lines.mid(0, 2).join('\n').trimmed();
            addLogMessage("Версия OpenVPN:\n" + ovpnVer, "info");
        } else {
            verProc.kill();
            addLogMessage("Не удалось получить версию OpenVPN", "warning");
        }
    }

    if (QFile::exists(serverConfigPath)) {
        QFile cfgFile(serverConfigPath);
        if (cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString cfgContent = cfgFile.readAll();
            cfgFile.close();

            QStringList lines = cfgContent.split('\n');
            QString firstLines;
            for (int i = 0; i < qMin(10, lines.size()); i++) {
                firstLines += lines[i] + "\n";
            }
            addLogMessage("Содержимое server.conf (первые 10 строк):\n" + firstLines, "info");

            if (cfgContent.contains("\r\n")) {
                addLogMessage("ВНИМАНИЕ: Файл содержит Windows-переносы строк (CRLF)", "warning");
            }
        }
    } else {
        addLogMessage("Конфигурационный файл не найден: " + serverConfigPath, "error");
    }

    if (openVPNServerProcess && openVPNServerProcess->state() == QProcess::Running) {
        addLogMessage("Процесс OpenVPN запущен, PID: " +
        QString::number(openVPNServerProcess->processId()), "success");
    } else {
        addLogMessage("Процесс OpenVPN не запущен", "warning");
    }

    verifyRouting();

    addLogMessage("Для проверки доступности извне выполните:", "info");
    addLogMessage("  telnet " + externalIP + " " + QString::number(spinServerPort->value()), "info");
    addLogMessage("  nmap -p " + QString::number(spinServerPort->value()) + " " + externalIP, "info");

    addLogMessage("=== КОНЕЦ ДИАГНОСТИКИ ===", "info");
}

void MainWindow::generateTestAndroidConfig()
{
    QString savePath = QFileDialog::getSaveFileName(this, "Сохранить тестовый конфиг для Android",
                                                    QDir::homePath() + "/android_test.ovpn",
                                                    "OpenVPN Config (*.ovpn)");
    if (savePath.isEmpty()) return;

    QString externalIP = executeCommand("curl -s ifconfig.me || curl -s icanhazip.com || echo '176.51.100.76'");
    externalIP = externalIP.trimmed();

    QString config;
    config += "# OpenVPN Android Test Configuration\n";
    config += "# Generated by Tor Manager\n";
    config += "# Server IP: " + externalIP + "\n";
    config += "# Date: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + "\n";
    config += "\n";

    config += "client\n";
    config += "dev tun\n";
    config += "proto tcp\n";
    config += "remote " + externalIP + " " + QString::number(spinServerPort->value()) + "\n";
    config += "resolv-retry infinite\n";
    config += "nobind\n";
    config += "persist-key\n";
    config += "persist-tun\n";
    config += "\n";

    config += "remote-cert-tls server\n";
    config += "auth SHA256\n";
    config += "auth-nocache\n";
    config += "tls-version-min 1.2\n";
    config += "\n";

    if (QFile::exists(caCertPath)) {
        config += "<ca>\n";
        QFile caFile(caCertPath);
        if (caFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString caContent = caFile.readAll();
            caFile.close();

            if (!caContent.contains("BEGIN CERTIFICATE")) {
                addLogMessage("CA сертификат имеет неверный формат!", "error");
                config += "# ОШИБКА: Неверный формат сертификата\n";
            } else {
                config += caContent;
            }
        }
    } else {
        config += "# CA сертификат не найден\n";
    }
    config += "</ca>\n";
    config += "\n";

    config += "verb 3\n";
    config += "mute 10\n";

    QFile file(savePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(config.toUtf8());
        file.close();
        addLogMessage("Тестовый Android конфиг сохранен: " + savePath, "success");

        QMessageBox::information(this, "Тестовый конфиг создан",
                                 "Создан упрощенный конфигурационный файл для Android.\n\n"
                                 "Путь: " + savePath + "\n\n"
                                 "Инструкция:\n"
                                 "1. Скопируйте файл на Android устройство\n"
                                 "2. В OpenVPN для Android импортируйте этот файл\n"
                                 "3. Попробуйте подключиться\n\n"
                                 "Если подключение не работает, выполните диагностику на сервере.");
    } else {
        addLogMessage("Ошибка сохранения файла: " + savePath, "error");
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл конфигурации");
    }
}

void MainWindow::testServerConfig()
{
    if (!QFile::exists(serverConfigPath)) {
        QMessageBox::warning(this, "Ошибка", "Конфигурация сервера не найдена: " + serverConfigPath);
        return;
    }

    addLogMessage("Тестирование конфигурации сервера...", "info");

    QString command = openVPNExecutablePath + " --config \"" + serverConfigPath + "\" --test";
    addLogMessage("Команда: " + command, "info");

    QProcess testProcess;
    testProcess.start(openVPNExecutablePath, QStringList() << "--config" << serverConfigPath << "--test");

    if (testProcess.waitForFinished(10000)) {
        QString output = QString::fromUtf8(testProcess.readAllStandardOutput());
        QString error = QString::fromUtf8(testProcess.readAllStandardError());

        if (testProcess.exitCode() == 0) {
            addLogMessage("✓ Конфигурация успешно прошла проверку", "success");

            if (!output.isEmpty()) {
                addLogMessage("Вывод:\n" + output, "info");
            }

            QMessageBox::information(this, "Проверка конфигурации",
                                     "Конфигурация сервера валидна!\n\n"
                                     "Путь: " + serverConfigPath);
        } else {
            addLogMessage("✗ Ошибка в конфигурации", "error");
            addLogMessage("Код ошибки: " + QString::number(testProcess.exitCode()), "error");

            if (!error.isEmpty()) {
                addLogMessage("Сообщение об ошибке:\n" + error, "error");
            }

            if (!output.isEmpty()) {
                addLogMessage("Вывод:\n" + output, "info");
            }

            QString userMessage = "Ошибка проверки конфигурации:\n\n";

            if (error.contains("Unrecognized option")) {
                userMessage += "Неизвестная опция в конфигурационном файле.\n";
                userMessage += "Проверьте синтаксис на наличие лишних пробелов или символов.\n\n";

                QRegularExpression re("Unrecognized option or missing.*?:\\s+(\\w+)");
                QRegularExpressionMatch match = re.match(error);
                if (match.hasMatch()) {
                    userMessage += "Проблемная опция: " + match.captured(1) + "\n";
                }
            } else if (error.contains("no such file")) {
                userMessage += "Файл сертификата или ключа не найден.\n";
                userMessage += "Проверьте пути к файлам в конфигурации.\n";
            } else if (error.contains("permission denied")) {
                userMessage += "Нет прав доступа к файлам сертификатов.\n";
                // SECURITY FIX: 644 сделает приватный ключ читаемым всем. Нужно 600 для ключей.
                userMessage += "Запустите: sudo chmod 600 " + certsDir + "/*.key\n";
                userMessage += "           sudo chmod 644 " + certsDir + "/*.crt " + certsDir + "/dh.pem\n";
            } else {
                userMessage += error;
            }

            QMessageBox::critical(this, "Ошибка конфигурации", userMessage);
        }
    } else {
        addLogMessage("Таймаут при проверке конфигурации", "error");
        QMessageBox::critical(this, "Ошибка", "Таймаут при проверке конфигурации");
    }
}

bool MainWindow::validateServerConfig()
{
    if (!QFile::exists(serverConfigPath)) {
        addLogMessage("Файл конфигурации не найден: " + serverConfigPath, "error");
        return false;
    }

    QFile file(serverConfigPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addLogMessage("Не удалось прочитать файл конфигурации", "error");
        return false;
    }
    QString content = file.readAll();
    file.close();

    // Логируем полный конфиг
    QStringList meaningful;
    for (const QString &l : content.split('\n'))
        if (!l.trimmed().startsWith('#') && !l.trimmed().isEmpty())
            meaningful << l.trimmed();
    addLogMessage("=== server.conf ===\n" + meaningful.join('\n') + "\n==================", "info");

    // dh none — PFS режим
    bool dhNone = content.contains(QRegularExpression("^dh\\s+none",
                                                      QRegularExpression::MultilineOption));

    // Обязательные директивы
    QStringList requiredKeys = {"port", "proto", "dev", "ca", "cert", "key"};
    if (!dhNone) requiredKeys << "dh";

    QStringList errors;
    for (const QString &opt : requiredKeys) {
        if (!content.contains(QRegularExpression("^" + opt + "\\s",
            QRegularExpression::MultilineOption)))
            errors << "Отсутствует директива: " + opt;
    }

    // Вспомогательная: извлечь путь из директивы
    auto extractPath = [&](const QString &directive) -> QString {
        QRegularExpression re("^" + directive + "\\s+(.+)$",
                              QRegularExpression::MultilineOption);
        auto m = re.match(content);
        if (!m.hasMatch()) return QString();
        return m.captured(1).trimmed();
    };

    // Список файлов для проверки
    struct FileCheck { QString directive; QString label; };
    QList<FileCheck> checks = {
        {"ca",   "CA сертификат"},
        {"cert", "Сертификат сервера"},
        {"key",  "Ключ сервера"},
    };
    if (!dhNone) checks << FileCheck{"dh", "DH параметры"};

    // ИСПРАВЛЕНО: используем правильную конкатенацию
    const char* const tlsOptions[] = {"tls-auth", "tls-crypt"};
    for (const char* d : tlsOptions) {
        QString p = extractPath(d);
        if (!p.isEmpty() && p != "none") {
            FileCheck fc;
            fc.directive = d;
            fc.label = QString(d) + " ключ";
            checks << fc;
        }
    }

    QString diagLines;
    bool allFilesOk = true;
    for (const auto &chk : checks) {
        QString path = extractPath(chk.directive);
        if (path.isEmpty()) {
            errors << "Директива '" + chk.directive + "' не содержит пути";
            diagLines += "❌ " + chk.label + ": путь пустой\n";
            allFilesOk = false;
            continue;
        }
        bool exists = QFile::exists(path);
        diagLines += (exists ? "✅ " : "❌ ") + chk.label + ":\n    " + path + "\n";
        if (!exists) {
            errors << chk.label + " не найден:\n    " + path;
            addLogMessage(chk.label + " не найден: " + path, "error");
            allFilesOk = false;
        } else {
            addLogMessage(chk.label + " OK: " + path, "info");
        }
    }

    if (!errors.isEmpty() || !allFilesOk) {
        QString msg = "Конфигурация сервера содержит ошибки:\n\n"
        + diagLines + "\n"
        + (errors.isEmpty() ? "" : "Ошибки:\n• " + errors.join("\n• ") + "\n\n")
        + "Конфиг: " + serverConfigPath + "\n"
        + "Сертификаты: " + certsDir + "\n\n"
        + "Для генерации: Настройки → 🔒 VPN/Сертификаты";
        QMessageBox dlg(QMessageBox::Critical, "Ошибка конфигурации", msg, QMessageBox::Ok, this);
        dlg.setDetailedText("server.conf:\n" + meaningful.join('\n'));
        dlg.exec();
        return false;
    }

    addLogMessage("✅ Конфигурация сервера валидна", "success");
    return true;
}


// ========== МЕТОДЫ ДЛЯ ТЕЛЕГРАМ БОТА ==========

void MainWindow::createTelegramTab()
{
    // Панель управления ботом встроена в вкладку «Клиенты»
    // Этот метод можно использовать для дополнительной инициализации при необходимости
    if (tgBotManager) {
        addLogMessage("Вкладка Telegram бота инициализирована", "info");
    }
}

void MainWindow::onBotClientCreated(const QString &cn)
{
    addLogMessage(QString("[Telegram Bot] ✅ Создан клиент: %1").arg(cn), "info");
    addLogMessage(QString("[DEBUG] Время обработки: %1 мс")
    .arg(QDateTime::currentMSecsSinceEpoch()), "info");
    // Перечитываем реестр с диска — бот пишет в тот же файл напрямую
    loadClientRegistry();
    updateRegistryTable();

    if (trayIcon && chkShowTrayNotifications && chkShowTrayNotifications->isChecked()) {
        trayIcon->showMessage(
            "Telegram Bot",
            QString("Создан клиент: %1").arg(cn),
                              QSystemTrayIcon::Information, 3000
        );
    }
}

void MainWindow::onBotClientRevoked(const QString &cn)
{
    addLogMessage(QString("[Telegram Bot] Отозван доступ: %1").arg(cn), "warning");
    loadClientRegistry();
    updateRegistryTable();

    // Показываем уведомление в трее
    if (trayIcon && chkShowTrayNotifications && chkShowTrayNotifications->isChecked()) {
        trayIcon->showMessage(
            "Telegram Bot",
            QString("Отозван доступ: %1").arg(cn),
                              QSystemTrayIcon::Warning,
                              3000
        );
    }
}

void MainWindow::syncBotWithSettings()
{
    if (!tgBotManager) return;

    // Синхронизируем пути ВСЕГДА — независимо от наличия токена
    // (certsDir должен быть установлен до первого запроса на генерацию)
    tgBotManager->setCertsDir(certsDir);
    tgBotManager->setServerConfPath(serverConfigPath);

    QString registryPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
    + "/client_registry.ini";
    tgBotManager->setRegistryIni(registryPath);

    // Обновляем адрес сервера
    if (txtServerAddress && spinServerPort) {
        QString proto = (cboServerProto && cboServerProto->currentText() == "UDP") ? "udp" : "tcp";
        tgBotManager->setServerAddress(
            txtServerAddress->text().trimmed(),
            spinServerPort->value(),
            proto
        );
    }

    // Запускаем бота только если токен задан
    QString botToken = settings->value("telegram/botToken").toString();
    if (botToken.isEmpty()) {
        addLogMessage("⚠️ Токен Telegram бота не настроен!", "error");
        return;
    }
    tgBotManager->setBotToken(botToken);

    if (!tgBotManager->isRunning()) {
        addLogMessage("🤖 Запуск Telegram бота...", "info");
        tgBotManager->startPolling();
    }

    addLogMessage("Настройки Telegram бота синхронизированы", "info");
}

// ==================== MTProxy SLOTS ====================

void MainWindow::onMTProxyStarted()
{
    addLogMessage("🚀 MTProxy успешно запущен на порту " + 
                  QString::number(mtproxyManager->getPort()), "success");
    if (trayIcon && chkShowTrayNotifications && chkShowTrayNotifications->isChecked()) {
        trayIcon->showMessage("MTProxy", 
                             "Прокси-сервер запущен\n" + mtproxyManager->getProxyLink(),
                             QSystemTrayIcon::Information, 5000);
    }
}

void MainWindow::onMTProxyStopped()
{
    addLogMessage("MTProxy остановлен", "info");
}

void MainWindow::onMTProxyLog(const QString &msg, const QString &level)
{
    addLogMessage("[MTProxy] " + msg, level);
}

void MainWindow::startMTProxy()
{
    if (!mtproxyManager) {
        addLogMessage("MTProxy менеджер не инициализирован", "error");
        return;
    }
    
    QSettings settings("TorManager", "MTProxy");
    int port = settings.value("port", 443).toInt();
    QString secret = settings.value("secret").toString();

    // FIX: если сохранён старый 64-символьный секрет — сбрасываем
    // mtproto-proxy принимает ровно 32 hex символа
    bool isDD = secret.startsWith("dd") || secret.startsWith("DD");
    int expectedLen = isDD ? 34 : 32;
    if (!secret.isEmpty() && secret.length() != expectedLen) {
        addLogMessage(QString("Старый секрет неверной длины (%1 символов), генерируем новый").arg(secret.length()), "warning");
        secret.clear();
    }

    if (secret.isEmpty()) {
        secret = MTProxyManager::generateSecureSecret();
        settings.setValue("secret", secret);
        settings.sync();
        addLogMessage("Сгенерирован новый секрет для MTProxy", "info");
    }
    
    addLogMessage(QString("Запуск MTProxy на порту %1...").arg(port), "info");
    mtproxyManager->startProxy(port, secret);
}

void MainWindow::stopMTProxy()
{
    if (mtproxyManager && mtproxyManager->isRunning()) {
        addLogMessage("Остановка MTProxy...", "info");
        mtproxyManager->stopProxy();
    }
}

// ==================== END MTProxy SLOTS ====================
