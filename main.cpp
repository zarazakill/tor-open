#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QStyleFactory>
#include <QTranslator>
#include <QLocale>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QTimer>
#include <QLibraryInfo>
#include <QLoggingCategory>

#ifdef Q_OS_LINUX
#include <unistd.h>
#include <sys/types.h>
#include <csignal>
#include <sys/wait.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

// Включаем подробное логирование Qt
Q_LOGGING_CATEGORY(lcMain, "app.main")

// Обработчик критических ошибок Qt
void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";

    switch (type) {
        case QtDebugMsg:
            fprintf(stderr, "[DEBUG] %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
            break;
        case QtInfoMsg:
            fprintf(stderr, "[INFO] %s\n", localMsg.constData());
            break;
        case QtWarningMsg:
            fprintf(stderr, "[WARNING] %s (%s:%u)\n", localMsg.constData(), file, context.line);
            break;
        case QtCriticalMsg:
            fprintf(stderr, "[CRITICAL] %s (%s:%u)\n", localMsg.constData(), file, context.line);
            break;
        case QtFatalMsg:
            fprintf(stderr, "[FATAL] %s (%s:%u)\n", localMsg.constData(), file, context.line);
            abort();
    }
}

#ifdef Q_OS_LINUX
// Демангинг C++ имен для backtrace
std::string demangle(const char* symbol) {
    size_t size;
    int status;
    char temp[1024];
    char* demangled;

    // Пытаемся найти demangled имя в строке
    if (sscanf(symbol, "%*[^(]%*[^_]%[^)+]", temp) == 1) {
        demangled = abi::__cxa_demangle(temp, nullptr, &size, &status);
        if (demangled) {
            std::string result(demangled);
            free(demangled);
            return result;
        }
    }

    // Если не получилось, пытаемся взять всю функцию
    if (sscanf(symbol, "%*[^(]%*[^_]%[^)+]s", temp) == 1) {
        return std::string(temp);
    }

    return std::string(symbol);
}

// Обработчик Segmentation Fault
void crashHandler(int sig) {
    void *array[100];
    char **messages;
    size_t size;

    fprintf(stderr, "\n========== ОШИБКА СЕГМЕНТИРОВАНИЯ (SIGSEGV) ==========\n");
    fprintf(stderr, "Получен сигнал %d (%s)\n", sig,
            sig == SIGSEGV ? "SIGSEGV" :
            sig == SIGABRT ? "SIGABRT" :
            sig == SIGFPE ? "SIGFPE" : "неизвестный");

    // Получаем backtrace
    size = backtrace(array, 100);
    messages = backtrace_symbols(array, size);

    fprintf(stderr, "\nСтек вызовов (%zu фреймов):\n", size);
    fprintf(stderr, "----------------------------------------\n");

    for (size_t i = 0; i < size; i++) {
        std::string demangled = demangle(messages[i]);
        fprintf(stderr, "  %zu: %s\n", i, demangled.c_str());
    }

    fprintf(stderr, "----------------------------------------\n");
    fprintf(stderr, "\nПопытка сохранить отладочную информацию...\n");

    // Сохраняем backtrace в файл
    char filename[256];
    time_t now = time(nullptr);
    struct tm *tm_info = localtime(&now);
    strftime(filename, sizeof(filename), "crash_%Y%m%d_%H%M%S.log", tm_info);

    FILE *log = fopen(filename, "w");
    if (log) {
        fprintf(log, "Crash time: %s", ctime(&now));
        fprintf(log, "Signal: %d\n\nStack trace:\n", sig);
        for (size_t i = 0; i < size; i++) {
            std::string demangled = demangle(messages[i]);
            fprintf(log, "%zu: %s\n", i, demangled.c_str());
        }
        fclose(log);
        fprintf(stderr, "✅ Отладочная информация сохранена в: %s\n", filename);
    } else {
        fprintf(stderr, "❌ Не удалось создать файл для сохранения информации\n");
    }

    free(messages);

    fprintf(stderr, "\n========================================================\n");

    // Завершаем процесс
    exit(1);
}

// Обработчик для всех критических сигналов
void setupSignalHandlers() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = crashHandler;

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);

    // Игнорируем SIGPIPE
    signal(SIGPIPE, SIG_IGN);
}

// Проверка памяти процесса
void checkMemoryStatus() {
    FILE *f = fopen("/proc/self/status", "r");
    if (f) {
        char line[256];
        fprintf(stderr, "\n=== Информация о памяти процесса ===\n");
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "VmPeak", 6) == 0 ||
                strncmp(line, "VmSize", 6) == 0 ||
                strncmp(line, "VmRSS", 5) == 0 ||
                strncmp(line, "VmData", 6) == 0 ||
                strncmp(line, "VmStk", 5) == 0) {
                fprintf(stderr, "%s", line);
                }
        }
        fclose(f);
        fprintf(stderr, "====================================\n\n");
    }
}
#endif

/**
 * Главная функция приложения Tor Manager (Серверная версия)
 */
int main(int argc, char *argv[])
{
    // Устанавливаем обработчик сообщений Qt
    qInstallMessageHandler(qtMessageHandler);

    // Гарантируем валидный рабочий каталог для дочерних процессов.
    // Если приложение запущено из удалённого/несуществующего каталога,
    // QProcess наследует невалидный cwd -> "getcwd: нет доступа к родительским каталогам"
    if (QDir::currentPath().isEmpty() || !QDir(QDir::currentPath()).exists()) {
        QDir::setCurrent("/");
    }

    QApplication app(argc, argv);

    // Настройка организации и приложения
    QCoreApplication::setOrganizationName("TorManager");
    QCoreApplication::setOrganizationDomain("tormanager.local");
    QCoreApplication::setApplicationName("TorManager-Server");
    QCoreApplication::setApplicationVersion("1.0.0");

    // Установка локали
    QLocale::setDefault(QLocale(QLocale::Russian, QLocale::RussianFederation));

    // Установка стиля
    app.setStyle(QStyleFactory::create("Fusion"));

    // Настройка русского перевода для Qt
    QTranslator qtTranslator;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QString translationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#else
    QString translationsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
    if (qtTranslator.load(QLocale(), "qt", "_", translationsPath)) {
        app.installTranslator(&qtTranslator);
    }

    // Создаем необходимые директории
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir appDir(appDataPath);
    if (!appDir.exists()) {
        appDir.mkpath(".");
    }

    QString torDataDir = appDataPath + "/tor_data";
    QDir torDir(torDataDir);
    if (!torDir.exists()) {
        torDir.mkpath(".");
        qDebug() << "Создана директория для данных Tor:" << torDataDir;
    }

    QString certsDir = appDataPath + "/certs";
    QDir certs(certsDir);
    if (!certs.exists()) {
        certs.mkpath(".");
        qDebug() << "Создана директория для сертификатов:" << certsDir;
    }

    // Проверка прав доступа (только для Linux)
    #ifdef Q_OS_LINUX
    uid_t uid = geteuid();
    if (uid != 0) {
        qDebug() << "Приложение запущено без root-прав. UID:" << uid;

        QSettings settings("TorManager", "TorVPN");
        bool suppressWarning = settings.value("security/suppressPrivilegeWarning", false).toBool();

        if (!suppressWarning) {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Предупреждение о привилегиях");
            msgBox.setText(
                "<h3>Требуются права root</h3>"
                "<p>Для управления VPN сервером и настройки файрвола (kill switch) "
                "требуются права суперпользователя.</p>"
                "<p><b>Возможные варианты:</b></p>"
                "<ul>"
                "<li>Запустите с sudo: <code>sudo ./TorManager</code></li>"
                "<li>Настройте polkit для OpenVPN</li>"
                "<li>Используйте pkexec</li>"
                "</ul>"
                "<p><b>Функции, которые не будут работать без root-прав:</b></p>"
                "<ul>"
                "<li>Запуск OpenVPN сервера</li>"
                "<li>Kill switch (блокировка трафика)</li>"
                "<li>Настройка файрвола</li>"
                "<li>Маршрутизация клиентов через Tor</li>"
                "</ul>"
                "<p>Tor будет работать в обычном режиме.</p>"
            );
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Ignore);
            msgBox.button(QMessageBox::Yes)->setText("Продолжить");
            msgBox.button(QMessageBox::No)->setText("Выйти");
            msgBox.button(QMessageBox::Ignore)->setText("Больше не показывать");

            int ret = msgBox.exec();

            if (ret == QMessageBox::No) {
                return 0;
            } else if (ret == QMessageBox::Ignore) {
                settings.setValue("security/suppressPrivilegeWarning", true);
                settings.sync();
            }
        }
    } else {
        qDebug() << "Приложение запущено с root-правами. Все функции доступны.";
    }

    // Выводим информацию о памяти
    checkMemoryStatus();

    // Устанавливаем обработчики сигналов
    setupSignalHandlers();
    #endif

    // Проверка наличия Tor
    QStringList torPaths = {
        "/usr/bin/tor",
        "/usr/local/bin/tor",
        "/usr/sbin/tor",
        "/snap/bin/tor"
    };

    for (const QString &path : torPaths) {
        if (QFile::exists(path)) {
            qDebug() << "Найден Tor:" << path;
            break;
        }
    }

    // Проверка наличия OpenVPN
    QStringList openVPNPaths = {
        "/usr/sbin/openvpn",
        "/usr/bin/openvpn",
        "/usr/local/sbin/openvpn",
        "/snap/bin/openvpn"
    };

    for (const QString &path : openVPNPaths) {
        if (QFile::exists(path)) {
            qDebug() << "Найден OpenVPN:" << path;
            break;
        }
    }

    qDebug() << "Приложение запущено. Версия:" << QCoreApplication::applicationVersion();
    qDebug() << "Qt версия:" << qVersion();
    qDebug() << "PID процесса:" << getpid();

    // Создаем главное окно с отловом исключений
    MainWindow *window = nullptr;

    try {
        qDebug() << "Создание главного окна...";
        window = new MainWindow();
        qDebug() << "Главное окно создано успешно";
    } catch (const std::exception &e) {
        qCritical() << "Исключение при создании окна:" << e.what();
        return 1;
    } catch (...) {
        qCritical() << "Неизвестное исключение при создании окна";
        return 1;
    }

    // Загружаем настройки окна
    QSettings settings("TorManager", "TorVPN");

    if (settings.contains("window/geometry")) {
        window->restoreGeometry(settings.value("window/geometry").toByteArray());
    }

    if (settings.contains("window/state")) {
        window->restoreState(settings.value("window/state").toByteArray());
    }

    bool startMinimized = settings.value("general/startMinimized", false).toBool();
    bool torWasRunning = settings.value("tor/wasRunning", false).toBool();
    bool serverWasRunning = settings.value("server/wasRunning", false).toBool();

    if (startMinimized) {
        qDebug() << "Запуск в свернутом режиме";
        window->hide();

        if (torWasRunning) {
            qDebug() << "Автоматический запуск Tor (был запущен ранее)";
            QTimer::singleShot(2000, window, &MainWindow::startTor);
        }

        if (serverWasRunning) {
            qDebug() << "Автоматический запуск VPN сервера (был запущен ранее)";
            QTimer::singleShot(5000, window, [window]() {
                window->startOpenVPNServer();
            });
        }

        if (window->trayIcon && window->trayIcon->isVisible()) {
            window->trayIcon->showMessage(
                "Tor Manager Server",
                "Приложение запущено в фоновом режиме. Дважды щелкните по иконке в трее для открытия.",
                QSystemTrayIcon::Information,
                3000
            );
        }
    } else {
        qDebug() << "Запуск в нормальном режиме";
        window->show();

        if (torWasRunning) {
            QTimer::singleShot(500, [window]() {
                QMessageBox::StandardButton reply = QMessageBox::question(
                    window,
                    "Автозапуск Tor",
                    "Tor был запущен при прошлом закрытии программы. Запустить Tor сейчас?",
                    QMessageBox::Yes | QMessageBox::No
                );

                if (reply == QMessageBox::Yes) {
                    window->startTor();
                }
            });
        }

        if (serverWasRunning) {
            QTimer::singleShot(1000, [window]() {
                QMessageBox::StandardButton reply = QMessageBox::question(
                    window,
                    "Автозапуск VPN сервера",
                    "VPN сервер был запущен при прошлом закрытии программы. Запустить сервер сейчас?",
                    QMessageBox::Yes | QMessageBox::No
                );

                if (reply == QMessageBox::Yes) {
                    window->startOpenVPNServer();
                }
            });
        }
    }

    // Обработка аргументов командной строки
    QStringList args = app.arguments();
    if (args.contains("--help") || args.contains("-h")) {
        QMessageBox::information(nullptr, "Справка Tor Manager Server",
                                 "<h3>Использование:</h3>"
                                 "<p>TorManager [опции]</p>"
                                 "<h4>Опции:</h4>"
                                 "<ul>"
                                 "<li><b>--help, -h</b> - показать эту справку</li>"
                                 "<li><b>--minimized</b> - запустить свернутым</li>"
                                 "<li><b>--start-tor</b> - автоматически запустить Tor</li>"
                                 "<li><b>--start-server</b> - автоматически запустить VPN сервер</li>"
                                 "<li><b>--quiet</b> - не показывать предупреждения</li>"
                                 "</ul>"
        );
        return 0;
    }

    if (args.contains("--minimized") && !startMinimized) {
        window->hide();
    }

    if (args.contains("--start-tor")) {
        QTimer::singleShot(1000, window, &MainWindow::startTor);
    }

    if (args.contains("--start-server")) {
        QTimer::singleShot(2000, window, &MainWindow::startOpenVPNServer);
    }

    qDebug() << "Запуск event loop...";

    int result = 0;
    try {
        result = app.exec();
        qDebug() << "Event loop завершен с кодом:" << result;
    } catch (const std::exception &e) {
        qCritical() << "Исключение в event loop:" << e.what();
        result = 1;
    } catch (...) {
        qCritical() << "Неизвестное исключение в event loop";
        result = 1;
    }

    // Сохраняем состояние перед выходом
    settings.setValue("tor/wasRunning", window->torRunning);
    settings.setValue("server/wasRunning", window->serverMode);
    settings.setValue("window/geometry", window->saveGeometry());
    settings.setValue("window/state", window->saveState());
    settings.sync();

    qDebug() << "Приложение завершается с кодом:" << result;

    delete window;

    return result;
}
