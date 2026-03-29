cmake_minimum_required(VERSION 3.16)

project(TorManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---- ищем Qt6 ----
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Concurrent
    Sql
    Positioning
)

# ---- исходники ----
set(SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    dns_monitor.cpp
    dns_monitor.h
    tgbot_manager.cpp
    tgbot_manager.h
    mtproxymanager.cpp
    mtproxymanager.h
    client_stats.cpp
    client_stats.h
)

add_executable(TorManager ${SOURCES})

# ---- линковка Qt ----
target_link_libraries(TorManager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    Qt6::Sql
    Qt6::Positioning
)
