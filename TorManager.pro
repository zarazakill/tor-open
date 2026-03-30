QT += core gui widgets network concurrent
CONFIG += qt c++17 warn_on link_pkgconfig

TEMPLATE = app
TARGET = TorManager

VERSION = 1.1.0

DEFINES += APP_VERSION=\"$$VERSION\"
DEFINES += APP_NAME=\"TorManagerServer\"
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

# ---------- SOURCE FILES ----------
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    tgbot_manager.cpp \
    dns_monitor.cpp \
    client_stats.cpp

HEADERS += \
    mainwindow.h \
    tgbot_manager.h \
    dns_monitor.h \
    client_stats.h

INCLUDEPATH += .

# ---------- COMPILER FLAGS ----------
QMAKE_CXXFLAGS += -fstack-protector-strong
QMAKE_CXXFLAGS += -D_FORTIFY_SOURCE=2
QMAKE_CXXFLAGS += -pthread
QMAKE_CXXFLAGS += -Wno-reorder

QMAKE_LFLAGS += -pthread
QMAKE_LFLAGS += -Wl,-z,relro,-z,now

# ---------- RELEASE ----------
CONFIG(release, debug|release) {

    QMAKE_CXXFLAGS += -O3
    DEFINES += QT_NO_DEBUG_OUTPUT
}

# ---------- DEBUG ----------
CONFIG(debug, debug|release) {

    DEFINES += DEBUG_MODE
    TARGET = $$join(TARGET,,,d)
}

# ---------- LINUX ----------
unix:!macx {

    DEFINES += _GNU_SOURCE

    LIBS += -ldl -lz

    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }

    target.path = $$PREFIX/bin
    INSTALLS += target
}

# ---------- OPENSSL ----------
packagesExist(openssl) {
    PKGCONFIG += openssl
    DEFINES += HAVE_OPENSSL
    message("OpenSSL found")
}

# ---------- FEATURE FLAGS ----------
DEFINES += QT_NETWORK_ALLOW_IPV6
DEFINES += MAX_LOG_LINES=10000

DEFINES += BRIDGE_TEST_TIMEOUT=5000
DEFINES += CLIENT_STATS_UPDATE_INTERVAL=5000
DEFINES += CLIENTS_REFRESH_INTERVAL=3000

DEFINES += DEFAULT_CONFIG_PATH=\"/etc/TorManager\"

# ---------- BUILD INFO ----------
message("========================================")
message("Tor Manager Server")
message("Qt version: $$QT_VERSION")
message("Build: $$CONFIG")
message("C++: C++17")
message("========================================")
