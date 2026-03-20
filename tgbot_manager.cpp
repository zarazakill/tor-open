/**
 * tgbot_manager.cpp
 *
 * Полная реализация Telegram бота на C++/Qt для управления OpenVPN клиентами.
 * Функциональный эквивалент Python-бота (version_9/tgbot/bot.py):
 *   - /start, /help, /instruction
 *   - /status         — статус сервера (только admin)
 *   - /list           — список клиентов (только admin)
 *   - /newuser        — диалог создания клиента + генерация .ovpn (только admin)
 *   - /revoke <name>  — отзыв доступа (только admin)
 *   - /getconfig <n>  — повторная отправка .ovpn (только admin)
 *   - /cancel         — прерывание текущего диалога
 *
 * Транспорт: Telegram Bot API (Long Polling через QNetworkAccessManager).
 * Без сторонних библиотек — только Qt5/Qt6.
 */

#include "tgbot_manager.h"
#include <QNetworkProxy>
#include "mainwindow.h"

#include <QApplication>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QFileInfo>
#include <QProcess>
#include <QHttpMultiPart>
#include <QMimeDatabase>
#include <QScrollBar>
#include <QColor>
#include <QFont>
#include <cmath>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QPointer>
#include "dns_monitor.h"  // для DnsClientStats и DnsMonitor
#include "client_stats.h" // для ClientStats (класс)

// ─── Константы стилей ─────────────────────────────────────────────────────────
const QString TelegramBotManager::STYLE_RUNNING = "color: #27ae60; font-weight: bold;";
const QString TelegramBotManager::STYLE_STOPPED = "color: #7f8c8d; font-weight: bold;";
const QString TelegramBotManager::STYLE_ERROR = "color: #e74c3c; font-weight: bold;";

static const QString TG_API_BASE = "https://api.telegram.org/bot";

// ─── Конструктор / деструктор ─────────────────────────────────────────────────

TelegramBotManager::TelegramBotManager(QSettings *settings, QObject *parent)
: QObject(parent)
, settings(settings)
{
    nam = new QNetworkAccessManager(this);

    // Маршрутизируем весь трафик бота через Tor SOCKS5.
    // Без этого трафик root-процесса обходит transparent proxy iptables
    // и идёт напрямую — соединение с api.telegram.org сбрасывается.
    {
        int socksPort = settings ? settings->value("tor/socksPort", 9050).toInt() : 9050;
        QNetworkProxy torProxy;
        torProxy.setType(QNetworkProxy::Socks5Proxy);
        torProxy.setHostName("127.0.0.1");
        torProxy.setPort(static_cast<quint16>(socksPort));
        nam->setProxy(torProxy);
    }

    // Отключаем HTTP/2 глобально — он вызывает "device not open" и обрывы
    // соединения с api.telegram.org. HTTP/1.1 стабильнее для long polling.
    QNetworkRequest defaultReq;
    defaultReq.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    nam->setStrictTransportSecurityEnabled(false);

    reconnectTimer = new QTimer(this);
    reconnectTimer->setSingleShot(true);
    connect(reconnectTimer, &QTimer::timeout, this, &TelegramBotManager::doPoll);

    // Пути по умолчанию
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    certsDir        = dataDir + "/certs";
    registryIniPath = dataDir + "/client_registry.ini";
    ovpnOutDir      = QDir::tempPath() + "/tormanager_ovpn";

    QDir().mkpath(ovpnOutDir);
    QDir().mkpath(certsDir);

    loadSettings();
}

TelegramBotManager::~TelegramBotManager()
{
    stopPolling();
    // Очищаем все pending cert запросы чтобы lambda'ы таймеров не обращались
    // к уже удалённым объектам после уничтожения TelegramBotManager
    pendingCertChats.clear();
}

// ─── Настройки ────────────────────────────────────────────────────────────────

void TelegramBotManager::loadSettings()
{
    settings->beginGroup("TelegramBot");

    // Пробуем прочитать зашифрованный токен
    QString encryptedToken = settings->value("token", "").toString();

    if (encryptedToken.isEmpty()) {
        // Если нет зашифрованного, пробуем старый открытый токен
        botToken = settings->value("plainToken", "").toString();
        if (!botToken.isEmpty()) {
            // Мигрируем на новый формат
            saveSettings();
        }
    } else {
        // Расшифровываем токен
        botToken = simpleXorDecrypt(encryptedToken, "TorManagerKey2026");

        // Проверяем что расшифровка удалась (токен должен содержать ':')
        if (!botToken.contains(':') && botToken.length() < 40) {
            qWarning() << "Возможно ошибка расшифровки токена, используем как есть";
            botToken = encryptedToken; // fallback
        }
    }

    serverAddress  = settings->value("serverAddress", "").toString();
    serverPort     = settings->value("serverPort", 1194).toInt();
    serverProto    = settings->value("serverProto", "tcp").toString();
    ovpnOutDir     = settings->value("ovpnOutDir", ovpnOutDir).toString();
    pollTimeoutSec = settings->value("pollTimeout", 25).toInt();

    // certsDir и registryIniPath
    QString savedCerts = settings->value("certsDir", "").toString();
    if (!savedCerts.isEmpty()) {
        certsDir = savedCerts;
    }

    QString savedRegistry = settings->value("registryIni", "").toString();
    if (!savedRegistry.isEmpty()) {
        registryIniPath = savedRegistry;
    }

    QStringList adminStrs = settings->value("adminIds", QStringList()).toStringList();
    adminIds.clear();
    for (const QString &s : adminStrs) {
        bool ok;
        qint64 id = s.toLongLong(&ok);
        if (ok && id > 0) adminIds.append(id);
    }
    settings->endGroup();
}

void TelegramBotManager::saveSettings()
{
    settings->beginGroup("TelegramBot");

    // Шифруем токен перед сохранением
    if (!botToken.isEmpty()) {
        QString encryptedToken = simpleXorEncrypt(botToken, "TorManagerKey2026");
        settings->setValue("token", encryptedToken);
        // Сохраняем также plain token для обратной совместимости
        settings->setValue("plainToken", botToken);
    }

    settings->setValue("serverAddress", serverAddress);
    settings->setValue("serverPort", serverPort);
    settings->setValue("serverProto", serverProto);
    if (!certsDir.isEmpty()) {
        settings->setValue("certsDir", certsDir);
    }
    if (!registryIniPath.isEmpty()) {
        settings->setValue("registryIni", registryIniPath);
    }
    settings->setValue("ovpnOutDir", ovpnOutDir);
    settings->setValue("pollTimeout", pollTimeoutSec);

    QStringList adminStrs;
    for (qint64 id : adminIds) adminStrs << QString::number(id);
    settings->setValue("adminIds", adminStrs);
    settings->endGroup();
    settings->sync();
}

// ─── UI ───────────────────────────────────────────────────────────────────────

// Панель управления ботом — встраивается в верх вкладки «Клиенты»
QWidget* TelegramBotManager::buildBotControlPanel(QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setStyleSheet("QFrame { background: #2c3e50; border-radius: 6px; }");
    auto *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(12, 8, 12, 8);

    auto *botIcon = new QLabel("🤖");
    botIcon->setStyleSheet("font-size: 22px;");
    layout->addWidget(botIcon);

    auto *statusInfo = new QVBoxLayout;
    lblBotInfo = new QLabel("Telegram Bot");
    lblBotInfo->setStyleSheet("color: #ecf0f1; font-weight: bold; font-size: 13px;");
    lblBotStatus = new QLabel("● Остановлен");
    lblBotStatus->setStyleSheet("color: #7f8c8d; font-size: 11px;");
    statusInfo->addWidget(lblBotInfo);
    statusInfo->addWidget(lblBotStatus);
    layout->addLayout(statusInfo);
    layout->addStretch();

    pbPollActivity = new QProgressBar;
    pbPollActivity->setFixedSize(100, 8);
    pbPollActivity->setRange(0, 0);
    pbPollActivity->setVisible(false);
    pbPollActivity->setStyleSheet(
        "QProgressBar { border: none; background: #34495e; border-radius: 4px; }"
        "QProgressBar::chunk { background: #2ecc71; border-radius: 4px; }"
    );
    layout->addWidget(pbPollActivity);

    btnStartStop = new QPushButton("▶  Запустить бота");
    btnStartStop->setFixedHeight(32);
    btnStartStop->setStyleSheet(
        "QPushButton { background: #27ae60; color: white; border: none; "
        "border-radius: 5px; padding: 0 16px; font-weight: bold; }"
        "QPushButton:hover { background: #2ecc71; }"
        "QPushButton:pressed { background: #1e8449; }"
    );
    connect(btnStartStop, &QPushButton::clicked, this, &TelegramBotManager::onBtnStartStop);
    layout->addWidget(btnStartStop);

    tabWidget = frame;
    updateStatusLabel();
    return frame;
}

// Секция настроек бота — встраивается в «Настройки» MainWindow
QWidget* TelegramBotManager::buildSettingsSection(QWidget *parent)
{
    auto *container = new QWidget(parent);
    auto *vl = new QVBoxLayout(container);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(8);

    // ── Токен бота ────────────────────────────────────────────────────────
    {
        auto *grp = new QGroupBox("🤖 Telegram Bot — Токен");
        auto *fl  = new QFormLayout(grp);

        auto *tokenRow = new QHBoxLayout;
        edtToken = new QLineEdit;
        edtToken->setEchoMode(QLineEdit::Password);
        edtToken->setPlaceholderText("123456789:ABCDEFGHIJKLMNOPabcdefghijklmnop");
        edtToken->setText(botToken);
        tokenRow->addWidget(edtToken);

        btnToggleToken = new QPushButton("👁");
        btnToggleToken->setFixedWidth(32);
        connect(btnToggleToken, &QPushButton::clicked,
                this, &TelegramBotManager::onTokenVisibilityToggle);
        tokenRow->addWidget(btnToggleToken);

        btnTestToken = new QPushButton("Проверить");
        connect(btnTestToken, &QPushButton::clicked,
                this, &TelegramBotManager::onBtnTestToken);
        tokenRow->addWidget(btnTestToken);

        fl->addRow("Токен:", tokenRow);
        auto *hint = new QLabel("Получить токен: написать @BotFather → /newbot");
        hint->setStyleSheet("color: #7f8c8d; font-size: 10px;");
        fl->addRow("", hint);
        vl->addWidget(grp);
    }

    // ── Администраторы ────────────────────────────────────────────────────
    {
        auto *grp = new QGroupBox("👮 Telegram Bot — Администраторы (User ID)");
        auto *gvl = new QVBoxLayout(grp);

        lstAdmins = new QListWidget;
        lstAdmins->setFixedHeight(100);
        gvl->addWidget(lstAdmins);

        auto *addRow = new QHBoxLayout;
        edtNewAdminId = new QLineEdit;
        edtNewAdminId->setPlaceholderText("Telegram User ID...");
        edtNewAdminId->setValidator(new QRegularExpressionValidator(
            QRegularExpression("\\d{1,15}"), edtNewAdminId));
        addRow->addWidget(edtNewAdminId);

        btnAddAdmin = new QPushButton("➕ Добавить");
        connect(btnAddAdmin, &QPushButton::clicked,
                this, &TelegramBotManager::onBtnAddAdmin);
        addRow->addWidget(btnAddAdmin);

        btnRemoveAdmin = new QPushButton("➖ Удалить");
        connect(btnRemoveAdmin, &QPushButton::clicked,
                this, &TelegramBotManager::onBtnRemoveAdmin);
        addRow->addWidget(btnRemoveAdmin);
        gvl->addLayout(addRow);

        auto *hint = new QLabel("Узнать свой ID: написать @userinfobot в Telegram");
        hint->setStyleSheet("color: #7f8c8d; font-size: 10px;");
        gvl->addWidget(hint);
        vl->addWidget(grp);
    }

    // ── Параметры сервера ─────────────────────────────────────────────────
    {
        auto *grp = new QGroupBox("🌐 Telegram Bot — Параметры сервера");
        auto *fl  = new QFormLayout(grp);

        edtServerAddr = new QLineEdit;
        edtServerAddr->setPlaceholderText("1.2.3.4 или vpn.example.com");
        edtServerAddr->setText(serverAddress);
        fl->addRow("Публичный адрес:", edtServerAddr);

        spinPort = new QSpinBox;
        spinPort->setRange(1, 65535);
        spinPort->setValue(serverPort);
        fl->addRow("Порт OpenVPN:", spinPort);

        cboProto = new QComboBox;
        cboProto->addItems({"udp", "tcp"});
        cboProto->setCurrentText(serverProto);
        fl->addRow("Протокол:", cboProto);
        vl->addWidget(grp);
    }

    // ── Пути к файлам ─────────────────────────────────────────────────────
    {
        auto *grp = new QGroupBox("📁 Telegram Bot — Пути к файлам");
        auto *fl  = new QFormLayout(grp);

        auto *infoLabel = new QLabel(
            "✅ Пути к сертификатам и реестру клиентов берутся автоматически\n"
            "из основных настроек приложения (вкладка «Настройки»).\n\n"
            "Текущие пути:"
        );
        infoLabel->setStyleSheet("color: #bdc3c7; font-size: 11px;");
        infoLabel->setWordWrap(true);
        fl->addRow(infoLabel);

        lblCertsDirInfo = new QLabel(certsDir);
        lblCertsDirInfo->setStyleSheet(
            "color: #2ecc71; font-size: 10px; font-family: monospace; "
            "background: #1a252f; padding: 4px 6px; border-radius: 3px;"
        );
        lblCertsDirInfo->setWordWrap(true);
        fl->addRow("📂 Сертификаты:", lblCertsDirInfo);

        lblRegistryInfo = new QLabel(registryIniPath);
        lblRegistryInfo->setStyleSheet(
            "color: #2ecc71; font-size: 10px; font-family: monospace; "
            "background: #1a252f; padding: 4px 6px; border-radius: 3px;"
        );
        lblRegistryInfo->setWordWrap(true);
        fl->addRow("📄 Реестр:", lblRegistryInfo);

        auto *ovpnRow = new QHBoxLayout;
        edtOvpnOutDir = new QLineEdit;
        edtOvpnOutDir->setText(ovpnOutDir);
        edtOvpnOutDir->setPlaceholderText("Директория для временных .ovpn файлов");
        ovpnRow->addWidget(edtOvpnOutDir);
        btnBrowseOvpnOut = new QPushButton("📂");
        btnBrowseOvpnOut->setFixedWidth(32);
        connect(btnBrowseOvpnOut, &QPushButton::clicked, this, [this]{
            QString d = QFileDialog::getExistingDirectory(nullptr,
                                                          "Выберите директорию для .ovpn", edtOvpnOutDir->text());
            if (!d.isEmpty()) edtOvpnOutDir->setText(d);
        });
            ovpnRow->addWidget(btnBrowseOvpnOut);
            fl->addRow(".ovpn вывод:", ovpnRow);

            vl->addWidget(grp);
    }

    // ── Long Polling ──────────────────────────────────────────────────────
    {
        auto *grp = new QGroupBox("📡 Telegram Bot — Long Polling");
        auto *fl  = new QFormLayout(grp);
        spinPollTimeout = new QSpinBox;
        spinPollTimeout->setRange(5, 60);
        spinPollTimeout->setValue(pollTimeoutSec);
        spinPollTimeout->setSuffix(" сек");
        fl->addRow("Таймаут запроса:", spinPollTimeout);
        vl->addWidget(grp);
    }

    // ── Кнопка сохранения ─────────────────────────────────────────────────
    {
        auto *row = new QHBoxLayout;
        row->addStretch();
        btnSaveSettings = new QPushButton("💾  Сохранить настройки бота");
        btnSaveSettings->setFixedHeight(32);
        btnSaveSettings->setStyleSheet(
            "QPushButton { background: #2980b9; color: white; border: none; "
            "border-radius: 5px; padding: 0 20px; font-weight: bold; }"
            "QPushButton:hover { background: #3498db; }"
        );
        connect(btnSaveSettings, &QPushButton::clicked,
                this, &TelegramBotManager::onBtnSaveSettings);
        row->addWidget(btnSaveSettings);
        vl->addLayout(row);
    }

    refreshAdminsList();
    return container;
}

// Секция журнала бота — встраивается в «Журналы» MainWindow
QWidget* TelegramBotManager::buildLogSection(QWidget *parent)
{
    auto *grp = new QGroupBox("🤖 Журнал Telegram Бота", parent);
    auto *vl  = new QVBoxLayout(grp);
    vl->setSpacing(4);

    auto *row = new QHBoxLayout;
    row->addWidget(new QLabel("Фильтр:"));
    cboLogFilter = new QComboBox;
    cboLogFilter->addItems({"Все", "Info", "Warning", "Error"});
    row->addWidget(cboLogFilter);
    row->addStretch();
    btnClearLog = new QPushButton("🗑  Очистить");
    connect(btnClearLog, &QPushButton::clicked,
            this, &TelegramBotManager::onBtnClearLog);
    row->addWidget(btnClearLog);
    vl->addLayout(row);

    txtBotLog = new QTextEdit;
    txtBotLog->setReadOnly(true);
    txtBotLog->setFont(QFont("Courier", 9));
    txtBotLog->document()->setMaximumBlockCount(5000);
    vl->addWidget(txtBotLog);

    auto *bcastRow = new QHBoxLayout;
    edtBroadcast = new QTextEdit;
    edtBroadcast->setFixedHeight(52);
    edtBroadcast->setPlaceholderText("Сообщение всем администраторам...");
    bcastRow->addWidget(edtBroadcast);
    btnSendBroadcast = new QPushButton("📤 Отправить\nвсем");
    btnSendBroadcast->setFixedWidth(90);
    connect(btnSendBroadcast, &QPushButton::clicked,
            this, &TelegramBotManager::onBtnSendBroadcast);
    bcastRow->addWidget(btnSendBroadcast);
    vl->addLayout(bcastRow);

    return grp;
}

// ─── Вспомогательные методы ──────────────────────────────────────────────────

QNetworkRequest TelegramBotManager::createTelegramRequest(const QString &url, int timeoutMs)
{
    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // В Qt6 SpdyAllowedAttribute удален, используем только Http2AllowedAttribute
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    req.setTransferTimeout(timeoutMs);
    return req;
}

bool TelegramBotManager::checkOpenSSLInstalled()
{
    QProcess which;
    which.start("which", {"openssl"});
    which.waitForFinished(3000);
    return which.exitCode() == 0;
}

bool TelegramBotManager::generateClientCert(const QString &cn)
{
    CertGenerationResult result = generateClientCertDetailed(cn);
    if (!result.success) {
        logCertGenerationError(cn, result);
        return false;
    }
    return true;
}

// ─── Детальная генерация сертификата ─────────────────────────────────────────

CertGenerationResult TelegramBotManager::generateClientCertDetailed(const QString &cn)
{
    CertGenerationResult result;
    result.success = false;

    QString certsPath = certsDir;
    QString caCrt = certsPath + "/ca.crt";
    QString caKey = certsPath + "/ca.key";

    addLog(QString("🔐 Генерация сертификата для %1").arg(cn), "info");

    // 1. Проверка наличия openssl
    result.step = "check_openssl";
    QProcess whichOpenSSL;
    whichOpenSSL.start("which", {"openssl"});
    if (!whichOpenSSL.waitForFinished(3000) || whichOpenSSL.exitCode() != 0) {
        result.errorDetails = "OpenSSL не найден в системе. Установите: sudo apt install openssl";
        result.opensslError = whichOpenSSL.readAllStandardError();
        addLog("❌ " + result.errorDetails, "error");
        return result;
    }

    // 2. Проверка наличия CA файлов
    result.step = "check_ca_files";
    if (!QFile::exists(caCrt)) {
        result.errorDetails = "CA сертификат не найден: " + caCrt;
        addLog("❌ " + result.errorDetails, "error");
        return result;
    }
    if (!QFile::exists(caKey)) {
        result.errorDetails = "CA ключ не найден: " + caKey;
        addLog("❌ " + result.errorDetails, "error");
        return result;
    }

    // 3. Проверка прав на запись
    result.step = "check_permissions";
    QFileInfo certsDirInfo(certsPath);
    if (!certsDirInfo.isWritable()) {
        result.errorDetails = "Нет прав на запись в директорию: " + certsPath;
        addLog("❌ " + result.errorDetails, "error");
        return result;
    }

    QString cliKey = certsPath + "/" + cn + ".key";
    QString cliCsr = certsPath + "/" + cn + ".csr";
    QString cliCrt = certsPath + "/" + cn + ".crt";

    // Удаляем старые файлы
    QFile::remove(cliKey);
    QFile::remove(cliCsr);
    QFile::remove(cliCrt);

    // 4. Генерация ключа
    result.step = "generate_key";
    addLog("   Шаг 1/4: Генерация ключа...", "info");
    {
        QProcess genKey;
        genKey.start("openssl", {"genrsa", "-out", cliKey, "2048"});
        if (!genKey.waitForFinished(15000) || genKey.exitCode() != 0) {
            result.opensslError = QString::fromUtf8(genKey.readAllStandardError());
            result.opensslOutput = QString::fromUtf8(genKey.readAllStandardOutput());
            result.errorDetails = "Ошибка генерации ключа: " + result.opensslError;
            addLog("❌ " + result.errorDetails, "error");
            return result;
        }
    }

    // 5. Создание CSR
    result.step = "create_csr";
    addLog("   Шаг 2/4: Создание CSR...", "info");
    {
        QString subj = QString("/C=RU/ST=Moscow/L=Moscow/O=TorManager/CN=%1").arg(cn);
        QProcess genCsr;
        genCsr.start("openssl", {"req", "-new", "-key", cliKey, "-out", cliCsr, "-subj", subj});
        if (!genCsr.waitForFinished(15000) || genCsr.exitCode() != 0) {
            result.opensslError = QString::fromUtf8(genCsr.readAllStandardError());
            result.opensslOutput = QString::fromUtf8(genCsr.readAllStandardOutput());
            result.errorDetails = "Ошибка создания CSR: " + result.opensslError;
            addLog("❌ " + result.errorDetails, "error");
            return result;
        }
    }

    // 6. Подпись сертификата
    result.step = "sign_certificate";
    addLog("   Шаг 3/4: Подпись сертификата CA...", "info");
    {
        QProcess signCert;
        signCert.start("openssl", {
            "x509", "-req",
            "-in", cliCsr,
            "-CA", caCrt,
            "-CAkey", caKey,
            "-CAcreateserial",
            "-out", cliCrt,
            "-days", "365"
        });

        if (!signCert.waitForFinished(15000) || signCert.exitCode() != 0) {
            result.opensslError = QString::fromUtf8(signCert.readAllStandardError());
            result.opensslOutput = QString::fromUtf8(signCert.readAllStandardOutput());

            QProcess diagCA;
            diagCA.start("openssl", {"x509", "-in", caCrt, "-subject", "-noout"});
            diagCA.waitForFinished(2000);
            QString caSubject = QString::fromUtf8(diagCA.readAllStandardOutput()).trimmed();

            result.errorDetails = QString(
                "Ошибка подписи сертификата:\n"
                "  OpenSSL: %1\n"
                "  CA Subject: %2\n"
                "  Возможные причины:\n"
                "  • Неверный пароль CA ключа\n"
                "  • Повреждённый CA сертификат\n"
                "  • Недостаточно прав для записи")
            .arg(result.opensslError, caSubject);

            addLog("❌ " + result.errorDetails, "error");
            return result;
        }
    }

    // 7. Проверка сертификата
    result.step = "verify_certificate";
    addLog("   Шаг 4/4: Проверка сертификата...", "info");
    {
        QProcess verifyProc;
        verifyProc.start("openssl", {"verify", "-CAfile", caCrt, cliCrt});
        verifyProc.waitForFinished(5000);

        if (verifyProc.exitCode() != 0) {
            result.opensslError = QString::fromUtf8(verifyProc.readAllStandardError());
            result.opensslOutput = QString::fromUtf8(verifyProc.readAllStandardOutput());

            QProcess diagProc;
            diagProc.start("openssl", {"x509", "-in", cliCrt, "-issuer", "-noout"});
            diagProc.waitForFinished(2000);
            QString issuer = QString::fromUtf8(diagProc.readAllStandardOutput()).trimmed();

            QProcess diagCA;
            diagCA.start("openssl", {"x509", "-in", caCrt, "-subject", "-noout"});
            diagCA.waitForFinished(2000);
            QString caSubject = QString::fromUtf8(diagCA.readAllStandardOutput()).trimmed();

            result.errorDetails = QString(
                "Ошибка проверки сертификата:\n"
                "  OpenSSL: %1\n"
                "  Сертификат подписан: %2\n"
                "  Ожидаемый CA: %3\n"
                "  Возможные причины:\n"
                "  • Несоответствие CA при подписи\n"
                "  • Повреждённый сертификат\n"
                "  • Проблемы с датами (проверьте системное время)")
            .arg(result.opensslError, issuer, caSubject);

            addLog("❌ " + result.errorDetails, "error");
            return result;
        }
    }

    // Устанавливаем права
    QFile::setPermissions(cliKey, QFile::ReadOwner | QFile::WriteOwner);
    QFile::setPermissions(cliCrt, QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther);

    // Удаляем CSR
    QFile::remove(cliCsr);

    result.success = true;
    result.step = "completed";
    addLog(QString("✅ Сертификат для %1 успешно создан").arg(cn), "success");
    return result;
}

void TelegramBotManager::logCertGenerationError(const QString &cn, const CertGenerationResult &result)
{
    QString logMsg = QString(
        "❌ Ошибка генерации сертификата для %1\n"
        "   Шаг: %2\n"
        "   Ошибка: %3")
    .arg(cn, result.step, result.errorDetails);

    addLog(logMsg, "error");

    if (!result.opensslOutput.isEmpty()) {
        addLog("   OpenSSL stdout: " + result.opensslOutput, "info");
    }
    if (!result.opensslError.isEmpty()) {
        addLog("   OpenSSL stderr: " + result.opensslError, "error");
    }
}

// ─── Асинхронная генерация сертификата ───────────────────────────────────────

void TelegramBotManager::generateClientCertAsync(const QString &cn,
                                                 const QDate &expiry,
                                                 bool hasExpiry,
                                                 qint64 chatId,
                                                 int editMsgId,
                                                 bool isAdminFlow,
                                                 const TgMessage &originalMsg)
{
    // Защита от двойных запросов
    if (pendingCertChats.contains(chatId)) {
        sendMessage(chatId, "⏳ Предыдущий запрос ещё выполняется, пожалуйста, подождите...");
        return;
    }

    // Проверяем что certsDir установлен
    if (certsDir.isEmpty()) {
        addLog("❌ certsDir не настроен! Путь к сертификатам не передан боту.", "error");
        sendMessage(chatId,
                    "❌ <b>Ошибка конфигурации бота</b>\n\n"
                    "Директория сертификатов не настроена.\n"
                    "Перезапустите приложение — пути синхронизируются автоматически.");
        return;
    }

    pendingCertChats.insert(chatId);

    QString cnCopy = cn;
    QString certsDirCopy = certsDir;
    bool alreadyHas = certExists(cn);

    // Запускаем в пуле потоков
    QFuture<CertGenerationResult> future = QtConcurrent::run([=]() -> CertGenerationResult {
        if (alreadyHas) {
            CertGenerationResult res;
            res.success = true;
            res.step = "already_exists";
            return res;
        }
        return generateClientCertDetailed(cnCopy);
    });

    auto *watcher = new QFutureWatcher<CertGenerationResult>(this);

    // Таймаут с QPointer для защиты от use-after-free
    QPointer<QFutureWatcher<CertGenerationResult>> watcherPtr(watcher);
    QTimer::singleShot(60000, this, [this, watcherPtr, chatId, editMsgId, cnCopy]() {
        if (watcherPtr.isNull() || !watcherPtr.data()) {
            return;
        }
        if (!watcherPtr->isFinished()) {
            watcherPtr->cancel();
            if (pendingCertChats.contains(chatId)) {
                pendingCertChats.remove(chatId);
            }
            QString errorMsg = "❌ <b>Таймаут генерации сертификата</b>\n\n...";
            if (editMsgId > 0) {
                editMessage(chatId, editMsgId, errorMsg);
            } else {
                sendMessage(chatId, errorMsg);
            }
            addLog("❌ Таймаут генерации сертификата для " + cnCopy, "error");
            watcherPtr->deleteLater();
        }
    });

    connect(watcher, &QFutureWatcher<CertGenerationResult>::finished,
            this, [=]() mutable
            {
                // Защита: watcher может быть уже удален
                if (!watcher) {
                    return;
                }

                watcher->deleteLater();
                pendingCertChats.remove(chatId);

                CertGenerationResult result = watcher->result();

                if (!result.success) {
                    // Детальное сообщение об ошибке
                    QString errorMsg;

                    if (result.step == "check_openssl") {
                        errorMsg = QString(
                            "❌ <b>OpenSSL не найден</b>\n\n"
                            "Для работы бота необходим OpenSSL.\n\n"
                            "📦 <b>Установка:</b>\n"
                            "```bash\nsudo apt update\nsudo apt install openssl\n```\n\n"
                            "После установки перезапустите бота.");
                    } else if (result.step == "check_ca_files") {
                        errorMsg = QString(
                            "❌ <b>CA сертификаты не найдены</b>\n\n"
                            "%1\n\n"
                            "🔧 <b>Решение:</b> Сгенерируйте CA через интерфейс приложения "
                            "(Настройки → VPN → Сгенерировать сертификаты) или выполните:\n"
                            "```bash\ncd %2\nopenssl genrsa -out ca.key 4096\n"
                            "openssl req -new -x509 -key ca.key -out ca.crt -days 3650\n```")
                        .arg(result.errorDetails, certsDirCopy);
                    } else if (result.step == "check_permissions") {
                        errorMsg = QString(
                            "❌ <b>Нет прав на запись</b>\n\n"
                            "%1\n\n"
                            "🔧 <b>Решение:</b>\n"
                            "```bash\nsudo chown -R $(whoami) %2\n```")
                        .arg(result.errorDetails, certsDirCopy);
                    } else {
                        errorMsg = QString(
                            "❌ <b>Ошибка создания сертификата</b>\n\n"
                            "```\n%1\n```\n\n"
                            "📋 <b>Детали:</b>\n"
                            "• Шаг: %2\n"
                            "• OpenSSL stderr: %3")
                        .arg(result.errorDetails, result.step, result.opensslError);
                    }

                    if (editMsgId > 0) {
                        editMessage(chatId, editMsgId, errorMsg);
                    } else {
                        sendMessage(chatId, errorMsg);
                    }

                    logCertGenerationError(cnCopy, result);
                    return;
                }

                // Ищем ta.key
                QStringList taKeySearchPaths = {
                    certsDirCopy + "/ta.key",
                    certsDirCopy + "/easy-rsa/pki/ta.key",
                    certsDirCopy + "/easy-rsa/pki/private/ta.key"
                };
                QString foundTaKey;
                for (const QString &p : taKeySearchPaths) {
                    if (QFile::exists(p)) { foundTaKey = p; break; }
                }

                if (foundTaKey.isEmpty()) {
                    addLog("⚠️ ta.key не найден, генерирую...", "warning");

                    QProcess genTa;
                    genTa.start("openvpn", {"--genkey", "--secret", certsDirCopy + "/ta.key"});
                    if (!genTa.waitForFinished(10000) || genTa.exitCode() != 0) {
                        addLog("❌ Не удалось сгенерировать ta.key", "error");
                        sendMessage(chatId, "❌ Ошибка: не удалось создать ключ шифрования.");
                        return;
                    }
                    addLog("✅ ta.key сгенерирован в " + certsDirCopy, "success");
                } else {
                    addLog("✅ ta.key найден: " + foundTaKey, "info");
                }

                // Успешная генерация
                QString ovpnContent = buildOvpnConfig(cnCopy, hasExpiry ? expiry : QDate());
                if (ovpnContent.isEmpty()) {
                    sendMessage(chatId, QString("❌ Ошибка создания .ovpn для <code>%1</code>.").arg(cnCopy));
                    return;
                }

                QString tmpPath = ovpnOutDir + "/" + cnCopy + ".ovpn";
                QDir().mkpath(ovpnOutDir);
                QFile f(tmpPath);
                if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    sendMessage(chatId, "❌ Не удалось записать .ovpn файл.");
                    return;
                }
                f.write(ovpnContent.toUtf8());
                f.close();

                // Запись в реестр
                if (isAdminFlow) {
                    writeRegistryClient(cnCopy, expiry);
                } else {
                    TgClientRecord rec;
                    rec.displayName = originalMsg.firstName +
                    (originalMsg.lastName.isEmpty() ? "" : " " + originalMsg.lastName);
                    rec.telegramId = originalMsg.userId;
                    rec.expiryDate = expiry;
                    rec.registeredAt = QDate::currentDate();
                    rec.isBanned = false;
                    saveClient(cnCopy, rec);
                }

                // Отправка результата
                QString expiryStr = hasExpiry ? expiry.toString("dd.MM.yyyy") : "без ограничений";

                if (isAdminFlow) {
                    QString confirmMsg = QString(
                        "✅ <b>Пользователь создан:</b> <code>%1</code>\n"
                        "📅 Срок действия: %2\n\n"
                        "📎 Файл конфига готов к отправке.")
                    .arg(cnCopy, expiryStr);

                    if (editMsgId > 0) {
                        editMessage(chatId, editMsgId, "✅ Готово! Отправляю файл конфигурации...");
                    }
                    sendMessage(chatId, confirmMsg);
                    sendDocument(chatId, tmpPath, "");
                } else {
                    sendMessage(chatId, QString(
                        "✅ <b>Ваш VPN-конфиг готов!</b>\n\n"
                        "👤 Имя профиля: <code>%1</code>\n"
                        "📅 Действует до: <b>%2</b>\n\n"
                        "📎 Файл ниже — импортируйте его в OpenVPN клиент.\n"
                        "Инструкция: /instruction")
                    .arg(cnCopy, expiry.toString("dd.MM.yyyy")));
                    sendDocument(chatId, tmpPath, "");
                }

                addLog(QString("✅ Создан клиент: %1, срок: %2").arg(cnCopy, expiryStr), "info");
                emit clientCreated(cnCopy);
                emit clientsChanged();
            });

    watcher->setFuture(future);
}

// ─── Отправка сообщений с повторными попытками ───────────────────────────────

void TelegramBotManager::sendMessage(qint64 chatId, const QString &text,
                                     const QJsonArray &inlineKeyboard,
                                     const QString &parseMode)
{
    // Используем HTML — он надёжнее Markdown: не ломается на <>, _, *, спецсимволах в именах.
    // Весь текст бота уже написан с HTML-тегами (<b>, <code>, <i>).
    // Если parseMode явно задан — используем его, иначе HTML.
    QString actualParseMode = parseMode.isEmpty() ? "HTML" : parseMode;

    // Для HTML НЕ применяем toHtmlEscaped() к уже готовому HTML-тексту.
    // Для MarkdownV2 экранируем спецсимволы.
    QString processedText = text;
    if (actualParseMode == "MarkdownV2") {
        processedText = escapeMarkdownV2(text);
    }
    // HTML и Markdown — отправляем как есть (разметка уже встроена в текст)

    QJsonObject body;
    body["chat_id"] = chatId;
    body["text"] = processedText;
    body["parse_mode"] = actualParseMode;
    body["disable_web_page_preview"] = true;

    if (!inlineKeyboard.isEmpty()) {
        QJsonObject replyMarkup;
        replyMarkup["inline_keyboard"] = inlineKeyboard;
        body["reply_markup"] = replyMarkup;
    }

    QNetworkRequest req = createTelegramRequest(TG_API_BASE + botToken + "/sendMessage", 45000);
    QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);

    PendingRequest pending;
    pending.request = req;
    pending.payload = payload;
    pending.retryCount = 0;
    pending.chatId = chatId;
    pending.context = "sendMessage";

    auto *reply = nam->post(req, payload);
    pendingRequests[reply] = pending;

    connect(reply, &QNetworkReply::finished, this, [this, reply, chatId]() {
        handleNetworkError(reply, "sendMessage");
    });
}

void TelegramBotManager::editMessage(qint64 chatId, int messageId, const QString &text,
                                     const QJsonArray &inlineKeyboard,
                                     const QString &parseMode)
{
    QJsonObject body;
    body["chat_id"]    = chatId;
    body["message_id"] = messageId;
    body["text"]       = text;
    body["parse_mode"] = parseMode;

    if (!inlineKeyboard.isEmpty()) {
        QJsonObject replyMarkup;
        replyMarkup["inline_keyboard"] = inlineKeyboard;
        body["reply_markup"] = replyMarkup;
    } else {
        body["reply_markup"] = QJsonObject();
    }

    QNetworkRequest req = createTelegramRequest(TG_API_BASE + botToken + "/editMessageText", 15000);
    auto *reply = nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, chatId]() {
        QByteArray data = reply->readAll();
        auto err = reply->error();
        reply->deleteLater();
        if (err != QNetworkReply::NoError) return;
        QJsonObject root = QJsonDocument::fromJson(data).object();
        if (!root["ok"].toBool()) {
            QString desc = root["description"].toString();
            if (!desc.contains("not modified"))
                addLog(QString("⚠️ editMessage (chat %1): %2").arg(chatId).arg(desc), "warning");
        }
    });
}

void TelegramBotManager::answerCallbackQuery(const QString &callbackQueryId,
                                             const QString &text)
{
    QJsonObject body;
    body["callback_query_id"] = callbackQueryId;
    if (!text.isEmpty()) body["text"] = text;

    QNetworkRequest req = createTelegramRequest(TG_API_BASE + botToken + "/answerCallbackQuery", 10000);
    auto *reply = nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply]() {
        reply->readAll();
        reply->deleteLater();
    });
}

void TelegramBotManager::sendDocument(qint64 chatId, const QString &filePath,
                                      const QString &caption,
                                      const QString &parseMode)
{
    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        addLog(QString("❌ Не удалось открыть файл для отправки: %1").arg(filePath), "error");
        delete file;
        return;
    }

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Chat ID
    QHttpPart chatIdPart;
    chatIdPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                         "form-data; name=\"chat_id\"");
    chatIdPart.setBody(QByteArray::number(chatId));
    multiPart->append(chatIdPart);

    // Document
    QHttpPart documentPart;
    documentPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");

    QFileInfo fi(filePath);
    documentPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QString("form-data; name=\"document\"; filename=\"%1\"")
                           .arg(fi.fileName()));

    file->setParent(multiPart);
    documentPart.setBodyDevice(file);
    multiPart->append(documentPart);

    // Caption (опционально)
    if (!caption.isEmpty()) {
        QHttpPart captionPart;
        captionPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                              "form-data; name=\"caption\"");
        captionPart.setBody(caption.toUtf8());
        multiPart->append(captionPart);

        QHttpPart parsePart;
        parsePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                            "form-data; name=\"parse_mode\"");
        parsePart.setBody(parseMode.toUtf8());
        multiPart->append(parsePart);
    }

    QNetworkRequest req = createTelegramRequest(TG_API_BASE + botToken + "/sendDocument", 30000);

    // ВАЖНО: Устанавливаем правильный заголовок Content-Type
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  "multipart/form-data; boundary=" + multiPart->boundary());

    auto *reply = nam->post(req, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::finished, this, [this, reply, cn = fi.baseName(), chatId]{
        QByteArray data = reply->readAll();
        auto err = reply->error();
        QString errStr = reply->errorString();
        reply->deleteLater();

        if (err != QNetworkReply::NoError) {
            addLog(QString("❌ Ошибка отправки файла %1: %2").arg(cn, errStr), "error");

            // Попробуем отправить как обычный текст если файл не работает
            if (errStr.contains("Bad Request")) {
                QFile f(QString(ovpnOutDir + "/" + cn + ".ovpn"));
                if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString content = f.readAll();
                    f.close();
                    sendMessage(chatId, "📎 Содержимое .ovpn файла (сохраните как .ovpn):\n```\n" +
                    content.left(1000) + "...\n```");
                }
            }
            return;
        }

        QJsonObject root = QJsonDocument::fromJson(data).object();
        if (root["ok"].toBool()) {
            addLog(QString("✅ Файл %1.ovpn успешно отправлен").arg(cn), "info");
        } else {
            addLog(QString("❌ sendDocument API ошибка: %1")
            .arg(root["description"].toString()), "error");
        }
    });
}

// ─── Обработка сетевых ошибок ────────────────────────────────────────────────

void TelegramBotManager::handleNetworkError(QNetworkReply *reply, const QString &context)
{
    // Читаем всё ДО deleteLater — после него reply может стать невалидным
    if (!pendingRequests.contains(reply)) {
        reply->deleteLater();
        return;
    }

    PendingRequest pending = pendingRequests.take(reply);

    QNetworkReply::NetworkError err = reply->error();
    QString errStr = reply->errorString();
    QByteArray data;
    if (err == QNetworkReply::NoError && reply->isReadable()) {
        data = reply->readAll();
    }
    reply->deleteLater();

    if (err == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject root = doc.object();

        if (root["ok"].toBool()) {
            addLog(QString("✅ %1 успешно отправлен в чат %2").arg(context).arg(pending.chatId), "info");
        } else {
            addLog(QString("❌ API ошибка %1 (чат %2): %3")
            .arg(context, QString::number(pending.chatId), root["description"].toString()), "error");
        }
        return;
    }

    if (pending.retryCount < 3) {
        int delayMs = 1000 * (1 << pending.retryCount);
        pending.retryCount++;

        addLog(QString("⚠️ Ошибка %1 (попытка %2/3) в чат %3: %4. Повтор через %5 мс...")
        .arg(context)
        .arg(pending.retryCount)
        .arg(pending.chatId)
        .arg(errStr)
        .arg(delayMs), "warning");

        QTimer::singleShot(delayMs, this, [this, pending]() {
            retryFailedRequest(pending.request, pending.payload,
                               pending.retryCount, pending.chatId, pending.context);
        });
    } else {
        addLog(QString("❌ Не удалось отправить %1 после 3 попыток в чат %2: %3")
        .arg(context, QString::number(pending.chatId), errStr), "error");
    }
}

// Вспомогательный метод для безопасного получения MainWindow
MainWindow* TelegramBotManager::getMainWindow() const
{
    // Пробуем через parent()
    QObject *parentObj = parent();
    if (parentObj) {
        MainWindow *mw = qobject_cast<MainWindow*>(parentObj);
        if (mw) return mw;
    }

    // Ищем среди топ-уровневых виджетов
    QWidgetList topWidgets = QApplication::topLevelWidgets();
    for (QWidget *widget : topWidgets) {
        MainWindow *mw = qobject_cast<MainWindow*>(widget);
        if (mw) return mw;
    }

    return nullptr;
}

void TelegramBotManager::retryFailedRequest(QNetworkRequest request, QByteArray payload,
                                            int retryCount, qint64 chatId, const QString &context)
{
    PendingRequest pending;
    pending.request = request;
    pending.payload = payload;
    pending.retryCount = retryCount;
    pending.chatId = chatId;
    pending.context = context;

    auto *reply = nam->post(request, payload);
    pendingRequests[reply] = pending;

    connect(reply, &QNetworkReply::finished, this, [this, reply, chatId]() {
        handleNetworkError(reply, "sendMessage (retry)");
    });
}

QString TelegramBotManager::escapeMarkdownV2(const QString &text) const
{
    QString escaped;
    escaped.reserve(text.size() * 2);

    const QSet<QChar> specialChars = {
        '_', '*', '[', ']', '(', ')', '~', '`', '>', '#',
        '+', '-', '=', '|', '{', '}', '.', '!'
    };

    for (const QChar &ch : text) {
        if (specialChars.contains(ch)) {
            escaped += '\\';
        }
        escaped += ch;
    }

    return escaped;
}

QString TelegramBotManager::htmlEscape(const QString &text) const
{
    // Экранирует пользовательские данные для вставки в HTML-сообщение Telegram
    QString s = text;
    s.replace("&", "&amp;");
    s.replace("<", "&lt;");
    s.replace(">", "&gt;");
    return s;
}

// ─── Извлечение ta.key из server.conf ────────────────────────────────────────
// КРИТИЧНО: клиент обязан использовать тот же ta.key что и сервер.
// Читаем прямо из server.conf чтобы гарантировать совпадение.
// Несовпадение → "tls-crypt unwrap error: packet authentication failed"
QString TelegramBotManager::extractTaKeyFromServerConf()
{
    if (serverConfPath.isEmpty()) {
        addLog("⚠️ Путь к server.conf не задан — ta.key будет взят из файла", "warning");
        return QString();
    }
    QFile f(serverConfPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addLog("⚠️ Не удалось открыть server.conf: " + serverConfPath, "warning");
        return QString();
    }
    QString content = f.readAll();
    f.close();

    int start = content.indexOf("<tls-crypt>");
    int end   = content.indexOf("</tls-crypt>", start);
    if (start == -1 || end == -1) {
        addLog("⚠️ Блок <tls-crypt> не найден в server.conf", "warning");
        return QString();
    }

    QString block = content.mid(start + 11, end - start - 11).trimmed();
    if (!block.contains("-----BEGIN OpenVPN Static key V1-----")) {
        addLog("⚠️ Неверный формат ta.key в server.conf", "warning");
        return QString();
    }

    addLog("✅ ta.key извлечён из server.conf — гарантированное совпадение с сервером", "info");
    return block;
}



// ─── Управление ботом ────────────────────────────────────────────────────────

void TelegramBotManager::startPolling()
{
    if (pollingActive) return;
    if (botToken.isEmpty()) {
        addLog("❌ Токен бота не задан! Укажите токен в настройках.", "error");
        return;
    }

    pollingActive = true;
    pollingOffset = 0;
    reconnectDelay = 1000;

    addLog("🚀 Бот запущен. Начинаю опрос Telegram API...", "info");
    updateStatusLabel();

    if (pbPollActivity) pbPollActivity->setVisible(true);
    doPoll();
    emit statusChanged(true);
}

void TelegramBotManager::stopPolling()
{
    if (!pollingActive) return;
    pollingActive = false;

    if (currentPollReply) {
        currentPollReply->abort();
        currentPollReply = nullptr;
    }
    reconnectTimer->stop();

    if (pbPollActivity) pbPollActivity->setVisible(false);
    addLog("⏹ Бот остановлен.", "info");
    updateStatusLabel();
    emit statusChanged(false);
}

void TelegramBotManager::doPoll()
{
    if (!pollingActive || botToken.isEmpty()) return;
    if (currentPollReply) return;

    QString url = QString("%1%2/getUpdates?timeout=%3&offset=%4")
    .arg(TG_API_BASE, botToken)
    .arg(pollTimeoutSec)
    .arg(pollingOffset);

    QNetworkRequest req = createTelegramRequest(url, (pollTimeoutSec + 15) * 1000);
    QNetworkReply *reply = nam->get(req);
    currentPollReply = reply;

    connect(reply, &QNetworkReply::finished,
            this, [this, reply]{ onPollReply(reply); });
}

void TelegramBotManager::onPollReply(QNetworkReply *reply)
{
    // Сначала сохраняем error и errorString ДО readAll и deleteLater
    QNetworkReply::NetworkError replyError = reply->error();
    QString replyErrorString = reply->errorString();

    // Читаем данные только если устройство открыто и нет ошибки
    QByteArray data;
    if (replyError == QNetworkReply::NoError && reply->isReadable()) {
        data = reply->readAll();
    }

    // Обнуляем currentPollReply ДО deleteLater
    if (reply == currentPollReply) {
        currentPollReply = nullptr;
    }
    reply->deleteLater();

    if (!pollingActive) return;

    if (replyError != QNetworkReply::NoError &&
        replyError != QNetworkReply::OperationCanceledError)
    {
        addLog(QString("⚠️ Ошибка сети: %1. Переподключение через %2 мс...")
        .arg(replyErrorString).arg(reconnectDelay), "warning");
        reconnectTimer->start(reconnectDelay);
        reconnectDelay = qMin(reconnectDelay * 2, 30000);
        return;
    }

    reconnectDelay = 1000;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        reconnectTimer->start(1000);
        return;
    }

    QJsonObject root = doc.object();
    if (!root["ok"].toBool()) {
        addLog(QString("❌ API ошибка: %1").arg(root["description"].toString()), "error");
        reconnectTimer->start(5000);
        return;
    }

    QJsonArray updates = root["result"].toArray();
    for (const QJsonValue &upd : updates) {
        processUpdate(upd.toObject());
    }

    if (!updates.isEmpty()) {
        qint64 lastId = updates.last().toObject()["update_id"].toVariant().toLongLong();
        pollingOffset = static_cast<int>(lastId) + 1;
    }

    QTimer::singleShot(100, this, &TelegramBotManager::doPoll);  // 100мс пауза
}

// ─── Обработка обновлений ────────────────────────────────────────────────────

void TelegramBotManager::processUpdate(const QJsonObject &update)
{
    TgMessage msg;

    if (update.contains("message")) {
        QJsonObject m = update["message"].toObject();
        QJsonObject from = m["from"].toObject();
        msg.messageId = m["message_id"].toVariant().toLongLong();
        msg.chatId    = m["chat"].toObject()["id"].toVariant().toLongLong();
        msg.userId    = from["id"].toVariant().toLongLong();
        msg.firstName = from["first_name"].toString();
        msg.lastName  = from["last_name"].toString();
        msg.username  = from["username"].toString();
        msg.text      = m["text"].toString();
        msg.isCallback = false;
        processMessage(msg);
    }
    else if (update.contains("callback_query")) {
        QJsonObject cq   = update["callback_query"].toObject();
        QJsonObject from = cq["from"].toObject();
        QJsonObject m    = cq["message"].toObject();
        msg.messageId    = cq["id"].toVariant().toLongLong();
        msg.chatId       = m["chat"].toObject()["id"].toVariant().toLongLong();
        msg.userId       = from["id"].toVariant().toLongLong();
        msg.firstName    = from["first_name"].toString();
        msg.lastName     = from["last_name"].toString();
        msg.username     = from["username"].toString();
        msg.callbackData = cq["data"].toString();
        msg.lastBotMessageId = m["message_id"].toInt();
        msg.isCallback   = true;
        processCallback(msg);
    }
}

// В методе TelegramBotManager::processMessage добавьте новые команды:
void TelegramBotManager::processMessage(const TgMessage &msg)
{
    QString text = msg.text.trimmed();

    addLog(QString("📩 Команда от %1 (ID: %2): %3")
    .arg(htmlEscape(msg.firstName)).arg(msg.userId).arg(msg.text), "info");

    // Проверяем наличие активного диалога под мьютексом
    {
        QMutexLocker locker(&conversationsMutex);
        if (conversations.contains(msg.chatId)) {
            ConvContext &ctx = conversations[msg.chatId];
            if (ctx.state != ConvState::None) {
                // Разблокируем для длительных операций
                locker.unlock();

                if (text.startsWith("/")) {
                    handleCancel(msg);
                    QString cmd = text.split(' ').first().toLower();
                    if (cmd.contains('@')) cmd = cmd.split('@').first();
                    if (cmd == "/newuser") {
                        handleNewUser(msg);
                    }
                    return;
                }

                // Снова блокируем для доступа к ctx
                locker.relock();
                // Получаем свежую ссылку после relock
                if (conversations.contains(msg.chatId)) {
                    ConvContext &ctxRelock = conversations[msg.chatId];
                    handleConvMessage(msg, ctxRelock);
                }
                return;
            }
        }
    }

    // Если нет диалога, обрабатываем команду
    QString cmd = text.split(' ').first().toLower();
    if (cmd.contains('@')) cmd = cmd.split('@').first();

    if (cmd == "/start")       handleStart(msg);
    else if (cmd == "/help")   handleHelp(msg);
    else if (cmd == "/instruction") handleInstruction(msg);
    else if (cmd == "/status")      { if (isAdmin(msg.userId)) handleStatus(msg); else handleStart(msg); }
    else if (cmd == "/list")        { if (isAdmin(msg.userId)) handleList(msg); else handleStart(msg); }
    else if (cmd == "/newuser")     { if (isAdmin(msg.userId)) handleNewUser(msg); else handleStart(msg); }
    else if (cmd.startsWith("/revoke"))    { if (isAdmin(msg.userId)) handleRevoke(msg); else handleStart(msg); }
    else if (cmd.startsWith("/getconfig")) { if (isAdmin(msg.userId)) handleGetConfig(msg); else handleStart(msg); }
    else if (cmd == "/cancel")      handleCancel(msg);
    else {
        if (isAdmin(msg.userId)) {
            sendMessage(msg.chatId, "Неизвестная команда. Используйте /help.");
        } else {
            handleStart(msg);
        }
    }
}

void TelegramBotManager::processCallback(const TgMessage &msg)
{
    answerCallbackQuery(QString::number(msg.messageId));

    // Проверяем наличие активного диалога под мьютексом
    {
        QMutexLocker locker(&conversationsMutex);
        if (conversations.contains(msg.chatId)) {
            ConvContext &ctx = conversations[msg.chatId];
            if (ctx.state != ConvState::None) {
                locker.unlock();  // Разблокируем для handleConvCallback
                handleConvCallback(msg, ctx);
                return;
            }
        }
    }
}

// ─── Команды ──────────────────────────────────────────────────────────────────

void TelegramBotManager::handleStart(const TgMessage &msg)
{
    if (isAdmin(msg.userId)) {
        QString text = QString(
            "🔐 <b>Панель администратора TorVPN</b>\n\n"
            "👤 Администратор: %1\n\n"
            "📋 <b>Управление пользователями:</b>\n"
            "• /newuser — создать нового пользователя\n"
            "• /list — список всех пользователей\n"
            "• /revoke <code>&lt;имя&gt;</code> — отозвать доступ\n"
            "• /getconfig <code>&lt;имя&gt;</code> — повторно отправить .ovpn\n\n"
            "📊 <b>Информация:</b>\n"
            "• /status — статус VPN-сервера\n"
            "• /help — эта справка"
        ).arg(htmlEscape(msg.firstName));
        sendMessage(msg.chatId, text);
        addLog(QString("🔐 Вход администратора: %1 (ID: %2)").arg(htmlEscape(msg.firstName)).arg(msg.userId), "info");
        return;
    }

    auto clients = readRegistry();
    QString existingCn;
    for (auto it = clients.cbegin(); it != clients.cend(); ++it) {
        if (it.value().telegramId == msg.userId) {
            existingCn = it.key();
            break;
        }
    }

    if (!existingCn.isEmpty() && !clients[existingCn].isBanned) {
        addLog(QString("📤 Повторная отправка конфига %1 (ID: %2)").arg(existingCn).arg(msg.userId), "info");
        sendMessage(msg.chatId, QString(
            "👋 С возвращением, <b>%1</b>!\n\n"
            "📦 Отправляю ваш VPN-конфиг..."
        ).arg(htmlEscape(msg.firstName)));
        sendExistingConfig(msg, existingCn);
        return;
    }

    if (clients[existingCn].isBanned) {
        sendMessage(msg.chatId, "⛔ Ваш доступ был отозван. Обратитесь к администратору.");
        return;
    }

    if (consentGiven.contains(msg.userId)) {
        handleNewUserForNonAdmin(msg);
        return;
    }

    ConvContext ctx;
    ctx.state = ConvState::WaitingConsent;
    ctx.chatId = msg.chatId;
    ctx.pendingFirstName = msg.firstName;
    ctx.pendingLastName  = msg.lastName;
    ctx.pendingUsername  = msg.username;

    {
        QMutexLocker locker(&conversationsMutex);
        conversations[msg.chatId] = ctx;
    }

    QString displayName = msg.firstName;
    if (!msg.lastName.isEmpty()) displayName += " " + msg.lastName;
    if (!msg.username.isEmpty()) displayName += " (@" + msg.username + ")";

    QString consentText = QString(
        "👋 Добро пожаловать в <b>TorVPN</b>!\n\n"
        "━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
        "🔒 <b>Соглашение об использовании данных</b>\n"
        "━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n"
        "Для предоставления доступа к VPN нам необходимо обработать следующие данные "
        "вашего Telegram-аккаунта:\n\n"
        "👤 <b>Имя:</b> %1\n"
        "🔖 <b>Telegram ID:</b> <code>%2</code>\n\n"
        "📌 <b>Эти данные будут использованы для:</b>\n"
        "• Создания персонального VPN-сертификата\n"
        "• Идентификации вашего подключения\n"
        "• Технической поддержки\n\n"
        "⚠️ Данные хранятся только на сервере администратора и "
        "не передаются третьим лицам.\n\n"
        "Нажмите <b>«Согласен»</b> для получения вашего VPN-конфига."
    ).arg(htmlEscape(displayName)).arg(msg.userId);

    QJsonArray keyboard;
    QJsonArray row;
    QJsonObject btnYes, btnNo;
    btnYes["text"] = "✅ Согласен, получить VPN-конфиг";
    btnYes["callback_data"] = "consent_yes";
    btnNo["text"]  = "❌ Отказаться";
    btnNo["callback_data"] = "consent_no";
    row.append(btnYes);
    QJsonArray row2;
    row2.append(btnNo);
    keyboard.append(row);
    keyboard.append(row2);

    sendMessage(msg.chatId, consentText, keyboard);
    addLog(QString("📝 Запрос согласия для %1 (ID: %2)").arg(htmlEscape(msg.firstName)).arg(msg.userId), "info");
}

void TelegramBotManager::handleHelp(const TgMessage &msg)
{
    if (isAdmin(msg.userId)) {
        handleStart(msg);
    } else {
        sendMessage(msg.chatId,
                    "ℹ️ <b>TorVPN Bot</b>\n\n"
                    "Для получения VPN-конфигурации нажмите /start\n"
                    "Для инструкции по подключению: /instruction");
    }
}

void TelegramBotManager::handleNewUserForNonAdmin(const TgMessage &msg)
{
    QString cn;
    if (!msg.firstName.isEmpty()) {
        cn = msg.firstName;
        if (!msg.lastName.isEmpty()) cn += "_" + msg.lastName;
    } else if (!msg.username.isEmpty()) {
        cn = msg.username;
    } else {
        cn = QString("user%1").arg(msg.userId);
    }
    cn = sanitizeCN(cn);

    auto clients = readRegistry();
    if (clients.contains(cn)) {
        if (!clients[cn].isBanned) {
            sendExistingConfig(msg, cn);
        } else {
            sendMessage(msg.chatId, "⛔ Ваш доступ отозван. Обратитесь к администратору.");
        }
        return;
    }

    QDate expiry = QDate::currentDate().addMonths(1);

    sendMessage(msg.chatId, QString("⏳ Создаю ваш персональный VPN-конфиг, подождите..."));
    addLog(QString("🔐 Создание конфига для нового пользователя %1 (ID: %2)").arg(cn).arg(msg.userId), "info");

    generateClientCertAsync(cn, expiry, /*hasExpiry=*/true, msg.chatId,
                            /*editMsgId=*/0, /*isAdminFlow=*/false, msg);
}

void TelegramBotManager::sendExistingConfig(const TgMessage &msg, const QString &cn)
{
    auto clients = readRegistry();
    if (!clients.contains(cn)) {
        sendMessage(msg.chatId, "❌ Ваш профиль не найден. Используйте /start");
        return;
    }

    const TgClientRecord &rec = clients[cn];
    QString expStr = rec.expiryDate.isValid() ? rec.expiryDate.toString("dd.MM.yyyy") : "∞";

    addLog(QString("📤 Повторная отправка конфига %1").arg(cn), "info");
    sendMessage(msg.chatId, QString(
        "📦 <b>Ваш VPN-конфиг</b>\n\n"
        "👤 Профиль: <code>%1</code>\n"
        "📅 Действует до: <b>%2</b>\n\n"
        "📎 Файл ниже:"
    ).arg(cn, expStr));

    QString ovpnContent = buildOvpnConfig(cn, rec.expiryDate);
    if (ovpnContent.isEmpty()) {
        sendMessage(msg.chatId,
                    "❌ Не удалось сформировать конфиг.\n"
                    "Обратитесь к администратору.");
        return;
    }

    QString tmpPath = ovpnOutDir + "/" + cn + ".ovpn";
    QFile f(tmpPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(ovpnContent.toUtf8());
        f.close();
        sendMessage(msg.chatId, QString("📎 Отправляю файл конфига <code>%1.ovpn</code>:").arg(cn));
        sendDocument(msg.chatId, tmpPath, "");
    }
}

void TelegramBotManager::handleInstruction(const TgMessage &msg)
{
    const QString instr =
    "📋 <b>Инструкция по подключению к VPN</b>\n\n"
    "<b>Шаг 1 — Скачайте OpenVPN клиент</b>\n\n"
    "🤖 <b>Android:</b> [OpenVPN для Android](https://play.google.com/store/apps/details?id=de.blinkt.openvpn)\n"
    "🍎 <b>iOS:</b> [OpenVPN Connect](https://apps.apple.com/app/openvpn-connect/id590379981)\n"
    "🪟 <b>Windows:</b> [OpenVPN GUI](https://openvpn.net/community-downloads/)\n"
    "🐧 <b>Linux:</b> <code>sudo apt install openvpn</code>\n\n"
    "<b>Шаг 2 — Импортируйте конфигурацию</b>\n\n"
    "• Откройте OpenVPN клиент\n"
    "• Нажмите «+» или «Import profile»\n"
    "• Выберите файл <code>.ovpn</code>\n\n"
    "<b>Шаг 3 — Подключитесь</b>\n\n"
    "• Нажмите на профиль и «Connect»\n"
    "• Разрешите создание VPN-подключения\n\n"
    "<b>Готово! 🎉</b>\n\n"
    "❓ <b>Проблемы с подключением?</b>\n"
    "• Убедитесь что интернет работает без VPN\n"
    "• Попробуйте переподключиться\n"
    "• Напишите администратору";

    sendMessage(msg.chatId, instr);
}

void TelegramBotManager::handleStatus(const TgMessage &msg)
{
    if (!isAdmin(msg.userId)) {
        sendMessage(msg.chatId, "⛔ Нет доступа.");
        return;
    }

    auto clients = readRegistry();
    int total  = clients.size();
    int banned = 0;
    for (auto &c : clients) if (c.isBanned) banned++;
    int active = total - banned;

    addLog(QString("📊 /status запрошен admin %1").arg(msg.userId), "info");

    QString srvAddr = serverAddress.isEmpty() ? "не задан" : serverAddress;
    int srvPort = serverPort;
    qint64 chatId = msg.chatId;

    QProcess *p = new QProcess(this);
    connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, p, chatId, total, active, banned, srvAddr, srvPort]
            (int exitCode, QProcess::ExitStatus)
            {
                p->deleteLater();
                QString vpnStatus = (exitCode == 0) ? "🟢 Работает" : "🔴 Не запущен";

                QString text = QString(
                    "📊 <b>Статус сервера</b>\n\n"
                    "🖥 OpenVPN: %1\n"
                    "👥 Всего клиентов: <code>%2</code>\n"
                    "✅ Активных: <code>%3</code>\n"
                    "🚫 Заблокированных: <code>%4</code>\n"
                    "🌐 Адрес сервера: <code>%5:%6</code>"
                ).arg(vpnStatus).arg(total).arg(active).arg(banned)
                .arg(srvAddr).arg(srvPort);

                sendMessage(chatId, text);
            });

    p->start("pgrep", {"-x", "openvpn"});
}

void TelegramBotManager::handleList(const TgMessage &msg)
{
    if (!isAdmin(msg.userId)) {
        sendMessage(msg.chatId, "⛔ Нет доступа.");
        return;
    }

    auto clients = readRegistry();
    if (clients.isEmpty()) {
        sendMessage(msg.chatId, "📭 Реестр пуст — пользователей нет.");
        return;
    }

    QString chunk = "👥 <b>Список пользователей:</b>\n\n";
    QList<QString> keys = clients.keys();
    std::sort(keys.begin(), keys.end());

    for (const QString &cn : keys) {
        const auto &c = clients[cn];
        QString icon = c.isBanned ? "🚫" : "✅";
        QString expiry = c.expiryDate.isNull() ? "∞"
        : c.expiryDate.toString("dd.MM.yyyy");
        QString line = QString("%1 <code>%2</code>\n"
        "   📅 До: %3  •  🔁 Сессий: %4\n"
        "   ↓%5  ↑%6\n\n")
        .arg(icon, cn, expiry)
        .arg(c.sessionCount)
        .arg(formatBytes(c.totalBytesRx),
             formatBytes(c.totalBytesTx));

        if (chunk.length() + line.length() > 3800) {
            sendMessage(msg.chatId, chunk);
            chunk = "";
        }
        chunk += line;
    }
    if (!chunk.isEmpty())
        sendMessage(msg.chatId, chunk);

    addLog(QString("📋 /list запрошен admin %1, %2 записей").arg(msg.userId).arg(clients.size()), "info");
}

void TelegramBotManager::handleNewUser(const TgMessage &msg)
{
    if (!isAdmin(msg.userId)) {
        sendMessage(msg.chatId, "⛔ Нет доступа.");
        return;
    }

    QString rawName = msg.firstName.trimmed();
    if (!msg.lastName.trimmed().isEmpty())
        rawName += "_" + msg.lastName.trimmed();
    if (rawName.isEmpty() && !msg.username.isEmpty())
        rawName = msg.username;
    if (rawName.isEmpty())
        rawName = QString("user%1").arg(msg.userId);

    QString cn = sanitizeCN(rawName);
    if (cn.length() < 2)
        cn = QString("user%1").arg(msg.userId);

    auto clients = readRegistry();
    if (clients.contains(cn)) {
        int suffix = 2;
        while (clients.contains(cn + QString::number(suffix))) ++suffix;
        cn = cn + QString::number(suffix);
    }

    QDate expiry = QDate::currentDate().addMonths(1);

    ConvContext ctx;
    ctx.chatId       = msg.chatId;
    ctx.pendingCN    = cn;
    ctx.hasExpiry    = true;
    ctx.pendingExpiry = expiry;
    ctx.state        = ConvState::WaitingConfirm;

    // Сохраняем контекст под мьютексом
    {
        QMutexLocker locker(&conversationsMutex);
        conversations[msg.chatId] = ctx;
    }

    QJsonArray kb = makeInlineKeyboard({{
        {"✅ Создать и отправить .ovpn", "confirm_yes"},
        {"❌ Отмена",                    "confirm_no"}
    }});

    sendMessage(msg.chatId,
                QString(
                    "👤 <b>Создание пользователя</b>\n\n"
                    "🔤 Имя: <code>%1</code>\n"
                    "📅 Срок доступа: <b>%2</b> (1 месяц)\n\n"
                    "Создать пользователя и отправить <code>.ovpn</code>?"
                ).arg(cn, expiry.toString("dd.MM.yyyy")),
                kb);
}

void TelegramBotManager::handleRevoke(const TgMessage &msg)
{
    if (!isAdmin(msg.userId)) {
        sendMessage(msg.chatId, "⛔ Нет доступа.");
        return;
    }

    QStringList parts = msg.text.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 2) {
        sendMessage(msg.chatId,
                    "Использование: <code>/revoke &lt;имя_пользователя&gt;</code>\n\n"
                    "Например: <code>/revoke Ivan_Petrov</code>");
        return;
    }

    QString cn = sanitizeCN(parts[1]);
    auto clients = readRegistry();

    if (!clients.contains(cn)) {
        sendMessage(msg.chatId, QString("❌ Пользователь <code>%1</code> не найден.").arg(cn));
        return;
    }

    ConvContext ctx;
    ctx.state       = ConvState::WaitingRevokeConfirm;
    ctx.chatId      = msg.chatId;
    ctx.revokeTarget= cn;

    // Сохраняем контекст под мьютексом
    {
        QMutexLocker locker(&conversationsMutex);
        conversations[msg.chatId] = ctx;
    }

    QJsonArray kb = makeInlineKeyboard({{
        {"✅ Да, отозвать", "revoke_yes"},
        {"❌ Отмена",       "revoke_no"}
    }});

    sendMessage(msg.chatId,
                QString("⚠️ <b>Подтверждение</b>\n\n"
                "Отозвать доступ пользователя <code>%1</code>?\n"
                "Сертификат будет удалён.").arg(cn),
                kb);
}

void TelegramBotManager::handleGetConfig(const TgMessage &msg)
{
    if (!isAdmin(msg.userId)) {
        sendMessage(msg.chatId, "⛔ Нет доступа.");
        return;
    }

    QStringList parts = msg.text.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 2) {
        sendMessage(msg.chatId,
                    "Использование: <code>/getconfig &lt;имя_пользователя&gt;</code>");
        return;
    }

    QString cn = sanitizeCN(parts[1]);
    auto clients = readRegistry();

    if (!clients.contains(cn)) {
        sendMessage(msg.chatId, QString("❌ Пользователь <code>%1</code> не найден.").arg(cn));
        return;
    }
    if (clients[cn].isBanned) {
        sendMessage(msg.chatId, QString("⛔ Пользователь <code>%1</code> заблокирован.").arg(cn));
        return;
    }

    addLog(QString("📤 Генерирую конфиг для %1 по запросу admin %2").arg(cn).arg(msg.userId), "info");
    sendMessage(msg.chatId, QString("⏳ Генерирую конфиг для <code>%1</code>...").arg(cn));

    const TgClientRecord &rec = clients[cn];
    QString ovpnContent = buildOvpnConfig(cn, rec.expiryDate);
    if (ovpnContent.isEmpty()) {
        sendMessage(msg.chatId,
                    QString("❌ Не удалось собрать конфиг для <code>%1</code>.\n"
                    "Возможно, сертификаты отсутствуют.").arg(cn));
        return;
    }

    QString tmpPath = ovpnOutDir + "/" + cn + ".ovpn";
    QFile f(tmpPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(ovpnContent.toUtf8());
        f.close();
        sendMessage(msg.chatId, QString("📎 Файл конфига для <code>%1</code>:").arg(cn));
        sendDocument(msg.chatId, tmpPath, "");
    }
}

void TelegramBotManager::handleCancel(const TgMessage &msg)
{
    // Удаляем контекст под мьютексом
    {
        QMutexLocker locker(&conversationsMutex);
        conversations.remove(msg.chatId);
    }
    sendMessage(msg.chatId, "❌ Отменено.");
}

// ─── ConversationHandler ──────────────────────────────────────────────────────

void TelegramBotManager::handleConvMessage(const TgMessage &msg, ConvContext &ctx)
{
    switch (ctx.state) {
        case ConvState::WaitingName:        convGotName(msg, ctx);       break;
        case ConvState::WaitingExpiryDate:  convGotExpiryDate(msg, ctx); break;
        default: break;
    }
}

void TelegramBotManager::handleConvCallback(const TgMessage &msg, ConvContext &ctx)
{
    switch (ctx.state) {
        case ConvState::WaitingConsent:      convConsent(msg, ctx);        break;
        case ConvState::WaitingExpiryChoice: convExpiryChoice(msg, ctx);   break;
        case ConvState::WaitingConfirm:      convConfirm(msg, ctx);        break;
        case ConvState::WaitingRevokeConfirm:convRevokeConfirm(msg, ctx);  break;
        default: break;
    }
}

void TelegramBotManager::convConsent(const TgMessage &msg, ConvContext &ctx)
{
    if (msg.callbackData == "consent_yes") {
        consentGiven.insert(msg.userId);
        {
            QMutexLocker locker(&conversationsMutex);
            conversations.remove(msg.chatId);
        }
        addLog(QString("✅ Согласие принято: %1 (ID: %2)").arg(ctx.pendingFirstName).arg(msg.userId), "info");
        handleNewUserForNonAdmin(msg);
    } else {
        {
            QMutexLocker locker(&conversationsMutex);
            conversations.remove(msg.chatId);
        }
        sendMessage(msg.chatId,
                    "❌ Вы отказались от предоставления данных.\n\n"
                    "Без согласия мы не можем создать ваш VPN-профиль.\n"
                    "Если передумаете — нажмите /start");
        addLog(QString("🚫 Отказ от согласия: %1 (ID: %2)").arg(ctx.pendingFirstName).arg(msg.userId), "info");
    }
}

void TelegramBotManager::convGotName(const TgMessage &msg, ConvContext &ctx)
{
    QString safe = sanitizeCN(msg.text.trimmed());

    if (safe.length() < 2) {
        sendMessage(msg.chatId,
                    "❌ Имя слишком короткое или содержит только спецсимволы. Попробуйте ещё раз:");
        return;
    }

    auto clients = readRegistry();
    if (clients.contains(safe)) {
        sendMessage(msg.chatId,
                    QString("⚠️ Пользователь <code>%1</code> уже существует.\n"
                    "Введите другое имя или /cancel:").arg(safe));
        return;
    }

    {
        QMutexLocker locker(&conversationsMutex);
        ctx.pendingCN = safe;
        ctx.state = ConvState::WaitingExpiryChoice;
    }

    QJsonArray kb = makeInlineKeyboard({{
        {"📅 Установить срок",  "expiry_yes"},
        {"♾️ Без ограничений", "expiry_no"},
        {"❌ Отмена",           "expiry_cancel"}
    }});

    sendMessage(msg.chatId,
                QString("✅ Имя: <code>%1</code>\n\nУстановить срок действия доступа?").arg(safe),
                kb);
}

void TelegramBotManager::convExpiryChoice(const TgMessage &msg, ConvContext &ctx)
{
    if (msg.callbackData == "expiry_cancel") {
        {
            QMutexLocker locker(&conversationsMutex);
            conversations.remove(msg.chatId);
        }
        editMessage(msg.chatId, msg.lastBotMessageId, "❌ Отменено.");
        return;
    }

    if (msg.callbackData == "expiry_no") {
        {
            QMutexLocker locker(&conversationsMutex);
            ctx.hasExpiry = false;
            ctx.pendingExpiry = QDate();
        }
        convDoConfirm(msg, ctx);
        return;
    }

    {
        QMutexLocker locker(&conversationsMutex);
        ctx.state = ConvState::WaitingExpiryDate;
    }
    editMessage(msg.chatId, msg.lastBotMessageId,
                "📅 Введите дату истечения доступа в формате <b>ДД.ММ.ГГГГ</b>:\n"
                "_Например: 31.12.2025_\n\nОтмена: /cancel");
}

void TelegramBotManager::convGotExpiryDate(const TgMessage &msg, ConvContext &ctx)
{
    QDate d = QDate::fromString(msg.text.trimmed(), "dd.MM.yyyy");
    if (!d.isValid()) {
        sendMessage(msg.chatId,
                    "❌ Неверный формат. Введите дату как <b>ДД.ММ.ГГГГ</b>:");
        return;
    }
    if (d <= QDate::currentDate()) {
        sendMessage(msg.chatId, "❌ Дата должна быть в будущем. Введите ещё раз:");
        return;
    }

    {
        QMutexLocker locker(&conversationsMutex);
        ctx.hasExpiry = true;
        ctx.pendingExpiry = d;
    }
    convDoConfirm(msg, ctx);
}

void TelegramBotManager::convDoConfirm(const TgMessage &msg, ConvContext &ctx)
{
    {
        QMutexLocker locker(&conversationsMutex);
        ctx.state = ConvState::WaitingConfirm;
    }

    QString expiryStr = ctx.hasExpiry
    ? ctx.pendingExpiry.toString("dd.MM.yyyy")
    : "без ограничений";

    QJsonArray kb = makeInlineKeyboard({{
        {"✅ Создать и отправить .ovpn", "confirm_yes"},
        {"❌ Отмена",                    "confirm_no"}
    }});

    QString text = QString(
        "📋 <b>Подтверждение</b>\n\n"
        "👤 Имя: <code>%1</code>\n"
        "📅 Срок: %2\n\n"
        "Создать пользователя и сгенерировать <code>.ovpn</code>?"
    ).arg(ctx.pendingCN, expiryStr);

    if (msg.isCallback)
        editMessage(msg.chatId, msg.lastBotMessageId, text, kb);
    else
        sendMessage(msg.chatId, text, kb);
}

// tgbot_manager.cpp, функция convConfirm (строка ~1247)
void TelegramBotManager::convConfirm(const TgMessage &msg, ConvContext &ctx)
{
    if (msg.callbackData == "confirm_no") {
        {
            QMutexLocker locker(&conversationsMutex);
            conversations.remove(msg.chatId);
        }
        editMessage(msg.chatId, msg.lastBotMessageId, "❌ Отменено.");
        return;
    }

    QString cn     = ctx.pendingCN;
    QDate   expiry = ctx.pendingExpiry;
    bool    hasExp = ctx.hasExpiry;
    int     editId = msg.lastBotMessageId;

    {
        QMutexLocker locker(&conversationsMutex);
        conversations.remove(msg.chatId);
    }

    editMessage(msg.chatId, editId,
                QString("⏳ Генерирую сертификат для <code>%1</code>...").arg(cn));

    // generateClientCertAsync сам добавит в pendingCertChats
    generateClientCertAsync(cn, expiry, hasExp, msg.chatId, editId,
                            /*isAdminFlow=*/true, msg);
}

void TelegramBotManager::convRevokeConfirm(const TgMessage &msg, ConvContext &ctx)
{
    QString cn = ctx.revokeTarget;
    {
        QMutexLocker locker(&conversationsMutex);
        conversations.remove(msg.chatId);
    }

    if (msg.callbackData == "revoke_no") {
        editMessage(msg.chatId, msg.lastBotMessageId, "❌ Отменено.");
        return;
    }

    banClientInRegistry(cn, true);
    revokeClientCerts(cn);

    QFile ovpn(ovpnOutDir + "/" + cn + ".ovpn");
    if (ovpn.exists()) ovpn.remove();

    editMessage(msg.chatId, msg.lastBotMessageId,
                QString("✅ Доступ пользователя <code>%1</code> <b>отозван</b>.\n"
                "Сертификат удалён. Активные сессии завершатся при переподключении.").arg(cn));

    addLog(QString("🚫 Отозван доступ: %1").arg(cn), "warning");
    emit clientRevoked(cn);
    updateClientsTable();
}

// ─── Реестр и операции с клиентами ───────────────────────────────────────────

void TelegramBotManager::onRegistryUpdated(const QMap<QString, ClientRecord> &registry)
{
    QMutexLocker locker(&registryMutex);  // ✅ Потокобезопасно
    static int lastSize = -1;
    int currentSize = registry.size();

    cachedRegistry.clear();
    for (auto it = registry.begin(); it != registry.end(); ++it) {
        // ✅ Копирование данных
        const ClientRecord &src = it.value();
        TgClientRecord dst;
        dst.cn = it.key();
        dst.displayName = src.displayName;
        dst.expiryDate = src.expiryDate;
        dst.registeredAt = src.registeredAt;
        dst.telegramId = src.telegramId;
        dst.firstSeen = src.firstSeen;
        dst.lastSeen = src.lastSeen;
        dst.totalBytesRx = src.totalBytesRx;
        dst.totalBytesTx = src.totalBytesTx;
        dst.sessionCount = src.sessionCount;
        dst.totalOnlineSecs = src.totalOnlineSeconds;
        dst.isBanned = src.isBanned;
        dst.knownIPs = src.knownIPs.values();

        cachedRegistry[it.key()] = dst;
    }

    // ✅ Логирование только при изменении размера
    if (currentSize != lastSize) {
        addLog(QString("📋 Реестр обновлён из MainWindow: %1 записей")
        .arg(cachedRegistry.size()), "info");
        lastSize = currentSize;
    }
}

QMap<QString, TgClientRecord> TelegramBotManager::readRegistry()
{
    QMutexLocker locker(&registryMutex);
    return cachedRegistry;
}

bool TelegramBotManager::generateCA()
{
    QDir().mkpath(certsDir);
    QString caCrt = certsDir + "/ca.crt";
    QString caKey = certsDir + "/ca.key";

    {
        QProcess p;
        p.start("openssl", {"genrsa", "-out", caKey, "4096"});
        p.waitForFinished(60000);
        if (p.exitCode() != 0) {
            addLog("❌ CA genrsa: " + p.readAllStandardError(), "error");
            return false;
        }
    }

    {
        QProcess p;
        p.start("openssl", {
            "req", "-new", "-x509",
            "-key",  caKey,
            "-out",  caCrt,
            "-days", "3650",
            "-subj", "/C=RU/ST=Moscow/L=Moscow/O=TorManager/CN=TorManager CA"
        });
        p.waitForFinished(60000);
        if (p.exitCode() != 0) {
            addLog("❌ CA x509: " + p.readAllStandardError(), "error");
            return false;
        }
    }

    addLog("✅ CA сертификат создан в " + certsDir, "info");
    emit caGenerated();
    return true;
}

QString TelegramBotManager::buildOvpnConfig(const QString &cn, const QDate &expiry)
{
    auto readFile = [](const QString &path) -> QString {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }
        QString content = QString::fromUtf8(f.readAll()).trimmed();
        f.close();
        return content;
    };

    // Извлекает только нужный PEM-блок из файла (убирает текстовый заголовок OpenSSL,
    // лишние сертификаты в цепочке и т.д.)
    auto extractPem = [](const QString &content, const QString &pemType) -> QString {
        // pemType: "CERTIFICATE", "PRIVATE KEY", "OpenVPN Static key V1"
        QString beginMarker = "-----BEGIN " + pemType + "-----";
        QString endMarker   = "-----END "   + pemType + "-----";
        int start = content.indexOf(beginMarker);
        int end   = content.indexOf(endMarker, start);
        if (start == -1 || end == -1) return content; // нет маркеров — возвращаем как есть
        return content.mid(start, end + endMarker.length() - start).trimmed();
    };

    // Вспомогательная функция: ищет файл по нескольким путям
    auto readFileMulti = [&readFile](const QStringList &paths, QString *foundPath = nullptr) -> QString {
        for (const QString &p : paths) {
            QString content = readFile(p);
            if (!content.isEmpty()) {
                if (foundPath) *foundPath = p;
                return content;
            }
        }
        return QString();
    };

    QStringList missingFiles;

    // ca.crt — только в certsDir
    QString caCrtRaw = readFile(certsDir + "/ca.crt");
    if (caCrtRaw.isEmpty()) missingFiles << "ca.crt";
    QString caCrt = extractPem(caCrtRaw, "CERTIFICATE");

    // Клиентский сертификат — ищем в certsDir и easy-rsa/pki/
    QString cliCrtPath;
    QString cliCrtRaw = readFileMulti({
        certsDir + "/" + cn + ".crt",
        certsDir + "/easy-rsa/pki/" + cn + ".crt",
        certsDir + "/easy-rsa/pki/issued/" + cn + ".crt"
    }, &cliCrtPath);
    if (cliCrtRaw.isEmpty()) {
        missingFiles << cn + ".crt";
    } else {
        addLog("✅ Сертификат клиента: " + cliCrtPath, "info");
    }
    // Извлекаем ТОЛЬКО клиентский сертификат (первый блок CERTIFICATE)
    // Файлы easy-rsa могут содержать цепочку CA+client
    QString cliCrt = extractPem(cliCrtRaw, "CERTIFICATE");

    // Клиентский ключ — ищем в certsDir и easy-rsa/pki/
    QString cliKeyPath;
    QString cliKeyRaw = readFileMulti({
        certsDir + "/" + cn + ".key",
        certsDir + "/easy-rsa/pki/" + cn + ".key",
        certsDir + "/easy-rsa/pki/private/" + cn + ".key"
    }, &cliKeyPath);
    if (cliKeyRaw.isEmpty()) {
        missingFiles << cn + ".key";
    } else {
        addLog("✅ Ключ клиента: " + cliKeyPath, "info");
    }
    // Извлекаем ключ — может быть RSA PRIVATE KEY или PRIVATE KEY (PKCS8)
    QString cliKey = cliKeyRaw;
    if (cliKeyRaw.contains("-----BEGIN RSA PRIVATE KEY-----"))
        cliKey = extractPem(cliKeyRaw, "RSA PRIVATE KEY");
    else if (cliKeyRaw.contains("-----BEGIN PRIVATE KEY-----"))
        cliKey = extractPem(cliKeyRaw, "PRIVATE KEY");
    else if (cliKeyRaw.contains("-----BEGIN EC PRIVATE KEY-----"))
        cliKey = extractPem(cliKeyRaw, "EC PRIVATE KEY");

    // ta.key — ищем во всех возможных местах
    QString taKeyFoundPath;
    QString taKeyRaw = readFileMulti({
        certsDir + "/ta.key",
        certsDir + "/easy-rsa/pki/ta.key",
        certsDir + "/easy-rsa/pki/private/ta.key"
    }, &taKeyFoundPath);
    if (taKeyRaw.isEmpty()) {
        missingFiles << "ta.key";
        addLog("❌ ta.key не найден ни в одном из путей: "
               + certsDir + "/ta.key, "
               + certsDir + "/easy-rsa/pki/ta.key", "error");
    } else {
        addLog("✅ ta.key найден: " + taKeyFoundPath, "info");
    }
    // ta.key содержит заголовочный комментарий — берём весь блок
    QString taKey = extractPem(taKeyRaw, "OpenVPN Static key V1");

    if (!missingFiles.isEmpty()) {
        addLog("❌ Отсутствуют файлы: " + missingFiles.join(", "), "error");
        return QString();
    }

    QStringList lines;

    lines << "# OpenVPN Client Configuration";
    lines << "# Клиент: " + cn;
    lines << "# Создан: " + QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
    if (expiry.isValid()) {
        lines << "# Действителен до: " + expiry.toString("dd.MM.yyyy");
    } else {
        lines << "# Срок действия: без ограничений";
    }
    lines << "# Создан через: Tor Manager Telegram Bot";
    lines << "";

    lines << "client";
    lines << "dev tun";
    lines << QString("proto %1").arg(serverProto.toLower());
    lines << QString("remote %1 %2").arg(
        serverAddress.isEmpty() ? "176.59.145.225" : serverAddress
    ).arg(serverPort);
    lines << "resolv-retry infinite";
    lines << "nobind";
    lines << "persist-key";
    lines << "persist-tun";
    lines << "";

    // TLS: клиент явно указывает роль (требуется при tls-crypt)
    lines << "tls-client";
    lines << "remote-cert-tls server";
    lines << "tls-version-min 1.2";
    lines << "";

    // Шифры — должны совпадать с сервером
    lines << "data-ciphers AES-256-GCM:AES-128-GCM:CHACHA20-POLY1305:AES-256-CBC";
    lines << "data-ciphers-fallback AES-256-CBC";
    lines << "auth SHA256";
    lines << "auth-nocache";
    lines << "allow-compression no";
    lines << "";

    lines << "<ca>";
    lines << caCrt;
    lines << "</ca>";
    lines << "";

    lines << "<cert>";
    lines << cliCrt;
    lines << "</cert>";
    lines << "";

    lines << "<key>";
    lines << cliKey;
    lines << "</key>";
    lines << "";

    // tls-crypt: КРИТИЧНО — должен совпадать с ta.key сервера
    // Приоритет: server.conf (гарантия) → файл (fallback)
    {
        QString taContent = extractTaKeyFromServerConf();
        if (taContent.isEmpty()) {
            // Fallback: используем ta.key из файла
            taContent = taKey;
            if (taContent.isEmpty()) {
                addLog("❌ ta.key не найден ни в server.conf ни в файлах — клиент не сможет подключиться!", "error");
                return QString();
            }
            addLog("⚠️ ta.key взят из файла (убедитесь что он совпадает с server.conf)", "warning");
        }
        lines << "<tls-crypt>";
        lines << taContent;
        lines << "</tls-crypt>";
        lines << "";
    }

    lines << "verb 3";
    lines << "mute 10";

    return lines.join("\n");
}

bool TelegramBotManager::writeRegistryClient(const QString &cn, const QDate &expiry)
{
    QSettings reg(registryIniPath, QSettings::IniFormat);
    QString key = cn;
    key.replace(QRegularExpression("[/\\\\ ]"), "_");

    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    reg.beginGroup(key);
    reg.setValue("originalName",    cn);
    reg.setValue("displayName",     cn);
    reg.setValue("expiryDate",      expiry.isNull() ? "" : expiry.toString("yyyy-MM-dd"));
    if (!reg.contains("firstSeen")) reg.setValue("firstSeen", now);
    reg.setValue("lastSeen",        now);
    if (!reg.contains("totalBytesRx")) reg.setValue("totalBytesRx", 0);
    if (!reg.contains("totalBytesTx")) reg.setValue("totalBytesTx", 0);
    if (!reg.contains("sessionCount")) reg.setValue("sessionCount", 0);
    if (!reg.contains("totalOnlineSecs")) reg.setValue("totalOnlineSecs", 0);
    reg.setValue("isBanned", false);
    reg.endGroup();
    reg.sync();
    return true;
}

bool TelegramBotManager::banClientInRegistry(const QString &cn, bool banned)
{
    QSettings reg(registryIniPath, QSettings::IniFormat);
    QString key = cn;
    key.replace(QRegularExpression("[/\\\\ ]"), "_");
    reg.beginGroup(key);
    reg.setValue("isBanned", banned);
    reg.endGroup();
    reg.sync();
    return true;
}

void TelegramBotManager::saveRegistry(const QMap<QString, TgClientRecord> &clients)
{
    QSettings reg(registryIniPath, QSettings::IniFormat);
    reg.clear();

    for (const TgClientRecord &c : clients) {
        QString key = c.cn;
        key.replace(QRegularExpression("[/\\\\ ]"), "_");

        reg.beginGroup(key);
        reg.setValue("originalName",    c.cn);
        reg.setValue("displayName",     c.displayName);
        reg.setValue("telegramId",      c.telegramId);
        reg.setValue("registeredAt",    c.registeredAt.isNull() ? "" : c.registeredAt.toString("yyyy-MM-dd"));
        reg.setValue("expiryDate",      c.expiryDate.isNull() ? "" : c.expiryDate.toString("yyyy-MM-dd"));
        reg.setValue("firstSeen",       c.firstSeen.toString(Qt::ISODate));
        reg.setValue("lastSeen",        c.lastSeen.toString(Qt::ISODate));
        reg.setValue("totalBytesRx",    c.totalBytesRx);
        reg.setValue("totalBytesTx",    c.totalBytesTx);
        reg.setValue("sessionCount",    c.sessionCount);
        reg.setValue("totalOnlineSecs", c.totalOnlineSecs);
        reg.setValue("isBanned",        c.isBanned);
        reg.endGroup();
    }

    reg.sync();
}

void TelegramBotManager::saveClient(const QString &cn, TgClientRecord &rec)
{
    rec.cn = cn;
    auto clients = readRegistry();
    clients[cn] = rec;
    saveRegistry(clients);
}

bool TelegramBotManager::revokeClientCerts(const QString &cn)
{
    bool ok = true;
    for (const char* ext : {".crt", ".key", ".csr"}) {
        QString path = certsDir + "/" + cn + ext;
        if (QFile::exists(path)) {
            if (!QFile::remove(path))
                ok = false;
        }
    }
    return ok;
}

bool TelegramBotManager::certExists(const QString &cn) const
{
    return QFile::exists(certsDir + "/" + cn + ".crt") &&
    QFile::exists(certsDir + "/" + cn + ".key");
}

// ─── UI слоты ─────────────────────────────────────────────────────────────────

void TelegramBotManager::onBtnStartStop()
{
    if (pollingActive) {
        stopPolling();
    } else {
        onBtnSaveSettings();
        startPolling();
    }
}

void TelegramBotManager::onBtnSaveSettings()
{
    bool wasRunning = pollingActive;

    if (wasRunning) {
        stopPolling();
    }

    if (edtToken)       botToken       = edtToken->text().trimmed();
    if (edtServerAddr)  serverAddress  = edtServerAddr->text().trimmed();
    if (spinPort)       serverPort     = spinPort->value();
    if (cboProto)       serverProto    = cboProto->currentText();
    if (edtOvpnOutDir)  ovpnOutDir     = edtOvpnOutDir->text().trimmed();
    if (spinPollTimeout) pollTimeoutSec = spinPollTimeout->value();

    QDir().mkpath(ovpnOutDir);
    saveSettings();

    if (lblCertsDirInfo)  lblCertsDirInfo->setText(certsDir);
    if (lblRegistryInfo)  lblRegistryInfo->setText(registryIniPath);

    addLog("💾 Настройки сохранены.", "info");

    QString message;
    if (wasRunning) {
        message = "✅ Настройки сохранены.\n\n"
        "⚠️ Бот был остановлен для применения новых настроек.\n"
        "👉 Нажмите кнопку «Запустить бота» для продолжения работы.";
    } else {
        message = "✅ Настройки сохранены.\n\n"
        "👉 Для запуска бота нажмите кнопку «Запустить бота».";
    }

    QMessageBox::information(tabWidget, "Настройки сохранены", message);
    updateStatusLabel();
}

void TelegramBotManager::onBtnTestToken()
{
    QString token = edtToken ? edtToken->text().trimmed() : botToken;
    if (token.isEmpty()) {
        QMessageBox::warning(tabWidget, "Ошибка", "Введите токен бота.");
        return;
    }

    addLog("🔍 Проверяю токен...", "info");

    QNetworkRequest req = createTelegramRequest(TG_API_BASE + token + "/getMe", 10000);
    auto *reply = nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{
        QNetworkReply::NetworkError netErr = reply->error();
        QString netErrStr = reply->errorString();
        QByteArray data = reply->readAll();
        reply->deleteLater();

        // Сначала проверяем сетевую ошибку (прокси, таймаут, SSL и т.д.)
        if (netErr != QNetworkReply::NoError) {
            QString msg = QString("⚠️ Ошибка сети при проверке токена: %1").arg(netErrStr);
            addLog(msg, "error");
            QMessageBox::critical(tabWidget, "Ошибка сети",
                QString("Не удалось связаться с Telegram API.\n\n"
                        "Ошибка: %1\n\n"
                        "Убедитесь, что Tor запущен и SOCKS-порт доступен.").arg(netErrStr));
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject root = doc.object();
        if (root["ok"].toBool()) {
            QJsonObject bot = root["result"].toObject();
            QString name = bot["first_name"].toString();
            QString username = bot["username"].toString();
            addLog(QString("✅ Токен действителен! Бот: %1 (@%2)").arg(name, username), "info");
            if (lblBotInfo)
                lblBotInfo->setText(QString("🤖 %1 (@%2)").arg(name, username));
            QMessageBox::information(tabWidget, "Успех",
                                     QString("Токен действителен!\nБот: %1 (@%2)").arg(name, username));
        } else {
            QString err = root["description"].toString();
            if (err.isEmpty()) err = QString::fromUtf8(data.left(200));
            addLog("❌ Токен недействителен: " + err, "error");
            QMessageBox::critical(tabWidget, "Ошибка",
                                  "Токен недействителен:\n" + err);
        }
    });
}

void TelegramBotManager::onBtnAddAdmin()
{
    if (!edtNewAdminId) return;
    QString txt = edtNewAdminId->text().trimmed();
    bool ok;
    qint64 id = txt.toLongLong(&ok);
    if (!ok || id <= 0) {
        QMessageBox::warning(tabWidget, "Ошибка", "Введите корректный Telegram User ID.");
        return;
    }
    if (!adminIds.contains(id)) {
        adminIds.append(id);
        refreshAdminsList();
        edtNewAdminId->clear();
        addLog(QString("👮 Добавлен администратор: %1").arg(id), "info");
    }
}

void TelegramBotManager::onBtnRemoveAdmin()
{
    if (!lstAdmins) return;
    QListWidgetItem *item = lstAdmins->currentItem();
    if (!item) return;
    bool ok;
    qint64 id = item->text().split(' ').last().remove('(').remove(')').toLongLong(&ok);
    if (ok) {
        adminIds.removeAll(id);
        refreshAdminsList();
        addLog(QString("❌ Удалён администратор: %1").arg(id), "info");
    }
}

void TelegramBotManager::onBtnSendBroadcast()
{
    if (!edtBroadcast) return;
    QString text = edtBroadcast->toPlainText().trimmed();
    if (text.isEmpty()) return;
    if (!pollingActive) {
        QMessageBox::warning(tabWidget, "Бот не запущен",
                             "Запустите бота перед отправкой сообщений.");
        return;
    }

    int sent = 0;
    for (qint64 adminId : adminIds) {
        sendMessage(adminId, "📢 <b>Сообщение от сервера:</b>\n\n" + text);
        sent++;
    }
    addLog(QString("📢 Трансляция отправлена %1 администраторам").arg(sent), "info");
    edtBroadcast->clear();
}

void TelegramBotManager::onBtnClearLog()
{
    if (txtBotLog) txtBotLog->clear();
}

void TelegramBotManager::onTokenVisibilityToggle()
{
    if (!edtToken) return;
    bool hidden = (edtToken->echoMode() == QLineEdit::Password);
    edtToken->setEchoMode(hidden ? QLineEdit::Normal : QLineEdit::Password);
    if (btnToggleToken) btnToggleToken->setText(hidden ? "🙈" : "👁");
}

void TelegramBotManager::refreshAdminsList()
{
    if (!lstAdmins) {
        qWarning() << "refreshAdminsList: lstAdmins is null";
        return;
    }

    lstAdmins->clear();
    for (qint64 id : adminIds) {
        lstAdmins->addItem(QString("👤 Admin ID: %1").arg(id));
    }

    // Если список пуст, показываем подсказку
    if (adminIds.isEmpty()) {
        lstAdmins->addItem("❓ Нет администраторов");
    }
}

void TelegramBotManager::onPollTimeout()
{
    // Не используется напрямую
}

// ─── Вспомогательные ─────────────────────────────────────────────────────────

bool TelegramBotManager::isAdmin(qint64 userId) const
{
    return adminIds.contains(userId);
}

QString TelegramBotManager::sanitizeCN(const QString &name) const
{
    QString clean = name.trimmed();

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

    QString ascii;
    ascii.reserve(clean.size());
    for (const QChar &ch : clean) {
        if (ch.unicode() < 128)
            ascii += ch;
    }
    clean = ascii;

    clean.replace(QRegularExpression("[^a-zA-Z0-9_\\-]"), "_");
    clean.replace(QRegularExpression("_+"), "_");
    clean = clean.remove(QRegularExpression("^_+|_+$"));

    if (clean.length() > 64) clean = clean.left(64);
    return clean.isEmpty() ? "client" : clean;
}

QString TelegramBotManager::formatBytes(qint64 bytes) const
{
    if (bytes < 1024)             return QString("%1 Б").arg(bytes);
    if (bytes < 1024*1024)        return QString("%1 КБ").arg(bytes/1024.0, 0, 'f', 1);
    if (bytes < 1024*1024*1024LL) return QString("%1 МБ").arg(bytes/(1024.0*1024), 0, 'f', 1);
    return QString("%1 ГБ").arg(bytes/(1024.0*1024*1024), 0, 'f', 2);
}

QString TelegramBotManager::formatDuration(qint64 seconds) const
{
    if (seconds < 60)   return QString("%1 сек").arg(seconds);
    if (seconds < 3600) return QString("%1 мин").arg(seconds/60);
    if (seconds < 86400)return QString("%1 ч %2 мин").arg(seconds/3600).arg((seconds%3600)/60);
    return QString("%1 дн %2 ч").arg(seconds/86400).arg((seconds%86400)/3600);
}

QJsonArray TelegramBotManager::makeInlineKeyboard(
    const QList<QList<QPair<QString,QString>>> &rows) const
    {
        QJsonArray keyboard;
        for (const auto &row : rows) {
            QJsonArray btnRow;
            for (const auto &[text, data] : row) {
                QJsonObject btn;
                btn["text"]          = text;
                btn["callback_data"] = data;
                btnRow.append(btn);
            }
            keyboard.append(btnRow);
        }
        return keyboard;
    }

    void TelegramBotManager::addLog(const QString &msg, const QString &level)
    {
        if (!txtBotLog) {
            emit logMessage(msg, level);
            return;
        }

        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        QString color = "#ecf0f1";
        if (level == "warning") color = "#f39c12";
        else if (level == "error") color = "#e74c3c";
        else if (level == "info")  color = "#2ecc71";

        if (cboLogFilter) {
            QString filter = cboLogFilter->currentText().toLower();
            if (filter != "все" && filter != "all" && filter != level) {
                emit logMessage(msg, level);
                return;
            }
        }

        txtBotLog->append(
            QString("<span style='color:#7f8c8d;'>[%1]</span> "
            "<span style='color:%2;'>%3</span>")
            .arg(timestamp, color, msg.toHtmlEscaped())
        );

        QScrollBar *sb = txtBotLog->verticalScrollBar();
        sb->setValue(sb->maximum());

        emit logMessage(msg, level);
    }

    void TelegramBotManager::updateStatusLabel()
    {
        if (!lblBotStatus || !btnStartStop) return;

        if (pollingActive) {
            lblBotStatus->setText("● Работает");
            lblBotStatus->setStyleSheet("color: #27ae60; font-size: 11px;");
            btnStartStop->setText("⏹  Остановить бота");
            btnStartStop->setStyleSheet(
                "QPushButton { background: #e74c3c; color: white; border: none; "
                "border-radius: 5px; padding: 0 16px; font-weight: bold; }"
                "QPushButton:hover { background: #c0392b; }"
            );
        } else {
            lblBotStatus->setText("● Остановлен");
            lblBotStatus->setStyleSheet("color: #7f8c8d; font-size: 11px;");
            btnStartStop->setText("▶  Запустить бота");
            btnStartStop->setStyleSheet(
                "QPushButton { background: #27ae60; color: white; border: none; "
                "border-radius: 5px; padding: 0 16px; font-weight: bold; }"
                "QPushButton:hover { background: #2ecc71; }"
            );
        }
    }

    void TelegramBotManager::updateClientsTable()
    {
        emit clientsChanged();
    }

    QString TelegramBotManager::simpleXorEncrypt(const QString &input, const QString &key)
    {
        if (input.isEmpty() || key.isEmpty()) {
            qWarning() << "simpleXorEncrypt: пустой вход или ключ";
            return input;
        }

        QByteArray inputData = input.toUtf8();
        QByteArray keyData = key.toUtf8();
        QByteArray result;
        result.reserve(inputData.size());

        for (int i = 0; i < inputData.size(); ++i) {
            result.append(inputData[i] ^ keyData[i % keyData.size()]);
        }

        // Возвращаем в base64 для безопасного хранения
        return QString::fromLatin1(result.toBase64());
    }

    QString TelegramBotManager::simpleXorDecrypt(const QString &input, const QString &key)
    {
        if (input.isEmpty() || key.isEmpty()) {
            qWarning() << "simpleXorDecrypt: пустой вход или ключ";
            return input;
        }

        QByteArray inputData = QByteArray::fromBase64(input.toLatin1());
        if (inputData.isEmpty()) {
            qWarning() << "simpleXorDecrypt: не удалось декодировать base64";
            return input; // Возвращаем как есть, возможно это не зашифрованный токен
        }

        QByteArray keyData = key.toUtf8();
        QByteArray result;
        result.reserve(inputData.size());

        for (int i = 0; i < inputData.size(); ++i) {
            result.append(inputData[i] ^ keyData[i % keyData.size()]);
        }

        return QString::fromUtf8(result);
    }

    //Статистика DNS запросов
    void TelegramBotManager::handleStats(const TgMessage &msg)
    {
        if (!isAdmin(msg.userId)) {
            sendMessage(msg.chatId, "⛔ Нет доступа.");
            return;
        }

        // Безопасно получаем указатель на MainWindow
        MainWindow *mainWin = getMainWindow();

        // Проверяем доступность MainWindow
        if (!mainWin) {
            addLog("❌ Не удалось получить доступ к MainWindow", "error");
            sendMessage(msg.chatId, "❌ Внутренняя ошибка сервера.");
            return;
        }

        // Получаем DNS монитор через публичный метод
        DnsMonitor *dnsMonitor = mainWin->getDnsMonitor();
        if (!dnsMonitor) {
            addLog("❌ DNS монитор не инициализирован в MainWindow", "error");
            sendMessage(msg.chatId, "❌ DNS монитор не доступен.");
            return;
        }

        // Получаем статистику
        auto stats = dnsMonitor->getClientStats();

        // Формируем ответ
        QString response = "📊 <b>Статистика DNS запросов</b>\n\n";

        if (stats.isEmpty()) {
            response += "Нет данных.";
        } else {
            qint64 totalRequests = 0;
            qint64 totalSuspicious = 0;

            // Сортируем клиентов по имени
            QStringList clientNames = stats.keys();
            std::sort(clientNames.begin(), clientNames.end());

            for (const QString &clientName : clientNames) {
                const DnsClientStats &s = stats[clientName];
                totalRequests += s.totalRequests;
                totalSuspicious += s.suspiciousRequests;

                response += QString("👤 <b>%1</b>\n").arg(htmlEscape(s.clientName));
                response += QString("   📊 Всего: %1\n").arg(s.totalRequests);
                response += QString("   📡 DNS: %1 | HTTP: %2 | HTTPS: %3\n")
                .arg(s.dnsRequests)
                .arg(s.httpRequests)
                .arg(s.httpsRequests);

                if (s.suspiciousRequests > 0) {
                    response += QString("   ⚠️ <b>Подозрительных: %1</b>\n")
                    .arg(s.suspiciousRequests);
                }

                // Топ-3 категории
                if (!s.categoryCount.isEmpty()) {
                    response += "   📁 Категории: ";

                    // Сортируем категории по количеству запросов
                    QList<QPair<qint64, QString>> sortedCategories;
                    for (auto catIt = s.categoryCount.begin(); catIt != s.categoryCount.end(); ++catIt) {
                        sortedCategories.append({catIt.value(), catIt.key()});
                    }
                    std::sort(sortedCategories.begin(), sortedCategories.end(),
                              [](const auto &a, const auto &b) { return a.first > b.first; });

                    int catCount = 0;
                    for (const auto &item : sortedCategories) {
                        if (++catCount > 3) break;
                        response += QString("%1(%2) ").arg(item.second).arg(item.first);
                    }
                }

                // Топ-3 домена
                if (!s.topDomains.isEmpty()) {
                    response += "\n   🔝 Топ: ";

                    QList<QPair<qint64, QString>> sortedDomains;
                    for (auto domIt = s.topDomains.begin(); domIt != s.topDomains.end(); ++domIt) {
                        sortedDomains.append({domIt.value(), domIt.key()});
                    }
                    std::sort(sortedDomains.begin(), sortedDomains.end(),
                              [](const auto &a, const auto &b) { return a.first > b.first; });

                    int domCount = 0;
                    for (const auto &item : sortedDomains) {
                        if (++domCount > 3) break;
                        response += QString("%1 ").arg(item.second);
                    }
                }

                response += "\n\n";
            }

            response += QString("━━━━━━━━━━━━━━━━━━━━\n");
            response += QString("📊 <b>ИТОГО:</b> %1 запросов\n").arg(totalRequests);
            response += QString("⚠️ <b>Подозрительных:</b> %1\n").arg(totalSuspicious);

            // Добавляем информацию о времени
            response += QString("\n⏱ Обновлено: %1")
            .arg(QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss"));
        }

        // Отправляем ответ (с разбивкой если слишком длинный)
        if (response.length() > 4000) {
            // Разбиваем на части
            QStringList parts;
            QString currentPart;
            QStringList lines = response.split('\n');

            for (const QString &line : lines) {
                if (currentPart.length() + line.length() > 3500) {
                    parts.append(currentPart);
                    currentPart.clear();
                }
                currentPart += line + "\n";
            }
            if (!currentPart.isEmpty()) {
                parts.append(currentPart);
            }

            // Отправляем по частям
            for (int i = 0; i < parts.size(); ++i) {
                QString partMsg = parts[i];
                if (i == 0) {
                    // Первая часть уже имеет заголовок
                    sendMessage(msg.chatId, partMsg);
                } else {
                    // Добавляем индикатор продолжения
                    partMsg = QString("📊 <b>Продолжение (%1/%2)</b>\n\n%3")
                    .arg(i + 1).arg(parts.size()).arg(partMsg);
                    sendMessage(msg.chatId, partMsg);
                }
            }
        } else {
            sendMessage(msg.chatId, response);
        }

        addLog(QString("📊 Статистика отправлена admin %1").arg(msg.userId), "info");
    }

    // Список подозрительных доменов
    void TelegramBotManager::handleSuspicious(const TgMessage &msg)
    {
        if (!isAdmin(msg.userId)) {
            sendMessage(msg.chatId, "⛔ Нет доступа.");
            return;
        }

        // Безопасно получаем указатель на MainWindow
        MainWindow *mainWin = getMainWindow();

        if (!mainWin) {
            addLog("❌ Не удалось получить доступ к MainWindow", "error");
            sendMessage(msg.chatId, "❌ Внутренняя ошибка сервера.");
            return;
        }

        // Получаем DNS монитор через публичный метод
        DnsMonitor *dnsMonitor = mainWin->getDnsMonitor();
        if (!dnsMonitor) {
            addLog("❌ DNS монитор не инициализирован в MainWindow", "error");
            sendMessage(msg.chatId, "❌ DNS монитор не доступен.");
            return;
        }

        auto suspicious = dnsMonitor->getSuspiciousDomains();

        QString response = "🚨 <b>Список подозрительных доменов</b>\n\n";

        if (suspicious.isEmpty()) {
            response += "✅ Список пуст — подозрительных доменов не обнаружено.";
        } else {
            // Сортируем домены
            QStringList sorted = suspicious;
            std::sort(sorted.begin(), sorted.end());

            for (const QString &domain : sorted) {
                response += "• <code>" + domain + "</code>\n";
            }
            response += QString("\n📊 Всего: %1 доменов").arg(sorted.size());
        }

        sendMessage(msg.chatId, response);
        addLog(QString("🚨 Список подозрительных доменов отправлен admin %1").arg(msg.userId), "info");
    }

    //Топ доменов по клиентам
    void TelegramBotManager::handleTopDomains(const TgMessage &msg)
    {
        if (!isAdmin(msg.userId)) {
            sendMessage(msg.chatId, "⛔ Нет доступа.");
            return;
        }

        // Разбираем аргументы: /top [клиент] [количество]
        QStringList parts = msg.text.split(' ', Qt::SkipEmptyParts);
        QString targetClient;
        int limit = 10;

        if (parts.size() >= 2) {
            targetClient = parts[1];
        }
        if (parts.size() >= 3) {
            bool ok;
            int l = parts[2].toInt(&ok);
            if (ok && l > 0 && l <= 50) limit = l;
        }

        MainWindow *mainWin = getMainWindow();
        if (!mainWin) {
            addLog("❌ Не удалось получить доступ к MainWindow", "error");
            sendMessage(msg.chatId, "❌ Внутренняя ошибка сервера.");
            return;
        }

        // Получаем DNS монитор через публичный метод
        DnsMonitor *dnsMonitor = mainWin->getDnsMonitor();
        if (!dnsMonitor) {
            addLog("❌ DNS монитор не инициализирован в MainWindow", "error");
            sendMessage(msg.chatId, "❌ DNS монитор не доступен.");
            return;
        }

        auto stats = dnsMonitor->getClientStats();

        QString response;

        if (!targetClient.isEmpty() && stats.contains(targetClient)) {
            // Топ для конкретного клиента
            const DnsClientStats &s = stats[targetClient];
            response = QString("📈 <b>Топ-%1 доменов для %2</b>\n\n")
            .arg(limit).arg(htmlEscape(targetClient));

            QList<QPair<qint64, QString>> sorted;
            for (auto it = s.topDomains.begin(); it != s.topDomains.end(); ++it) {
                sorted.append({it.value(), it.key()});
            }
            std::sort(sorted.begin(), sorted.end(),
                      [](const auto &a, const auto &b) { return a.first > b.first; });

            int count = 0;
            for (const auto &item : sorted) {
                if (++count > limit) break;
                QString domain = item.second;
                bool isSuspicious = dnsMonitor->isSuspiciousDomain(domain);
                response += QString("%1. %2 %3\n")
                .arg(count, 2)
                .arg(domain)
                .arg(isSuspicious ? "⚠️" : "");
            }
        } else {
            // Общий топ по всем клиентам
            QMap<QString, qint64> globalTop;
            for (auto it = stats.begin(); it != stats.end(); ++it) {
                const DnsClientStats &s = it.value();
                for (auto domIt = s.topDomains.begin(); domIt != s.topDomains.end(); ++domIt) {
                    globalTop[domIt.key()] += domIt.value();
                }
            }

            QList<QPair<qint64, QString>> sorted;
            for (auto it = globalTop.begin(); it != globalTop.end(); ++it) {
                sorted.append({it.value(), it.key()});
            }
            std::sort(sorted.begin(), sorted.end(),
                      [](const auto &a, const auto &b) { return a.first > b.first; });

            response = QString("📈 <b>Топ-%1 доменов (все клиенты)</b>\n\n")
            .arg(limit);

            int count = 0;
            for (const auto &item : sorted) {
                if (++count > limit) break;
                QString domain = item.second;
                bool isSuspicious = dnsMonitor->isSuspiciousDomain(domain);

                // Собираем, какие клиенты посещали этот домен
                QStringList clients;
                for (auto it = stats.begin(); it != stats.end(); ++it) {
                    if (it.value().topDomains.contains(domain)) {
                        clients.append(it.value().clientName);
                    }
                }

                response += QString("%1. <code>%2</code> %3\n   👥 %4 | %5 запросов\n")
                .arg(count, 2)
                .arg(domain)
                .arg(isSuspicious ? "⚠️" : "")
                .arg(clients.join(", "))
                .arg(item.first);
            }
        }

        sendMessage(msg.chatId, response);
        addLog(QString("📈 Топ доменов отправлен admin %1").arg(msg.userId), "info");
    }
