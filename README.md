# Tor Manager — Professional Privacy & Security Suite

## 📖 Описание / Description

**Tor Manager** — это профессиональное кроссплатформенное приложение для комплексного управления анонимностью и безопасностью сетевых соединений. Приложение объединяет управление сетью Tor, VPN-подключениями (OpenVPN), продвинутый мониторинг DNS-трафика, детализированную статистику клиентов и интеграцию с Telegram Bot для удалённого управления и уведомлений.

**Tor Manager** is a professional cross-platform application for comprehensive anonymity and network security management. The software combines Tor network control, VPN connections (OpenVPN), advanced DNS traffic monitoring, detailed client statistics, and Telegram Bot integration for remote management and notifications.

---

## 🌟 Ключевые возможности / Key Features

### 🔐 Управление Tor и VPN / Tor & VPN Management
- **RU**: Централизованное управление службами Tor и OpenVPN с автоматическим запуском и конфигурацией
- **EN**: Centralized management of Tor and OpenVPN services with automatic startup and configuration
- Поддержка мостов (bridges) для обхода блокировок
- Bridge support for circumventing censorship
- Автоматическое определение и настройка сетевых интерфейсов
- Automatic detection and configuration of network interfaces

### 📊 Мониторинг DNS-трафика / DNS Traffic Monitoring
- **RU**: Глубокий анализ DNS-запросов в реальном времени с классификацией доменов по категориям
- **EN**: Deep real-time DNS query analysis with domain categorization
- Обнаружение подозрительной активности и потенциальных угроз
- Suspicious activity detection and potential threat identification
- Отслеживание запросов по клиентам (IP, имя, тип запроса)
- Per-client request tracking (IP, name, request type)
- Категоризация: социальные сети, реклама, аналитика, трекеры
- Categories: social media, ads, analytics, trackers

### 📈 Статистика и отчётность / Statistics & Reporting
- **RU**: Детальная статистика по каждому клиенту с экспоненциальным сглаживанием данных
- **EN**: Detailed per-client statistics with exponential data smoothing
- Скорость передачи данных (Rx/Tx) в реальном времени
- Real-time data transfer speed (Rx/Tx)
- Топ доменов и категорий по частоте обращений
- Top domains and categories by request frequency
- Генерация отчётов за произвольные периоды
- Customizable period report generation
- Временные метки первого и последнего обращения
- First and last access timestamps

### 🤖 Telegram Bot Integration
- **RU**: Удалённое управление через Telegram Bot с интерактивным веб-интерфейсом
- **EN**: Remote management via Telegram Bot with interactive web interface
- Получение уведомлений о подозрительной активности
- Suspicious activity notifications
- Просмотр статистики и логов прямо в Telegram
- View statistics and logs directly in Telegram
- Быстрые команды для управления сервисами
- Quick commands for service management

### 🛡️ Безопасность и приватность / Security & Privacy
- **RU**: Многоуровневая система защиты с обработкой критических ошибок
- **EN**: Multi-level protection system with critical error handling
- Автоматический перехват и логирование сбоев (Segfault, Backtrace)
- Automatic crash interception and logging (Segfault, Backtrace)
- Демангинг C++ символов для читаемых отчётов об ошибках
- C++ symbol demangling for readable error reports
- Работа через pkexec для безопасного повышения привилегий
- pkexec integration for secure privilege escalation

### 🖥️ Пользовательский интерфейс / User Interface
- **RU**: Современный Qt-интерфейс с тёмной темой и адаптивным дизайном
- **EN**: Modern Qt-based interface with dark theme and adaptive design
- Интеграция с системным треем (System Tray)
- System tray integration
- Табличное представление данных с сортировкой и фильтрацией
- Tabular data view with sorting and filtering
- Календарь для выбора периодов отчётности
- Calendar for report period selection
- Экспорт данных в различные форматы
- Data export to multiple formats

---

## 🏗️ Архитектура / Architecture

### Основные компоненты / Core Components

| Компонент / Component | Описание / Description |
|----------------------|------------------------|
| `MainWindow` | Главное окно приложения с вкладками управления / Main application window with management tabs |
| `DnsMonitor` | Модуль мониторинга DNS через tcpdump и conntrack / DNS monitoring module via tcpdump and conntrack |
| `ClientStats` | Система сбора и агрегации статистики клиентов / Client statistics collection and aggregation |
| `TgBotManager` | Интеграция с Telegram Bot API / Telegram Bot API integration |
| `TorManager` | Управление процессами Tor и OpenVPN / Tor and OpenVPN process management |

### Технологический стек / Technology Stack

- **Фреймворк**: Qt 5/6 (C++)
- **Сборка**: CMake / qmake
- **Сеть**: QTcpSocket, QNetworkAccessManager
- **Данные**: JSON, QSettings, SQLite (опционально)
- **Мониторинг**: tcpdump, conntrack, /proc filesystem
- **Платформы**: Linux (Debian 13+, Ubuntu 24+, Fedora, Arch)

---

## 🚀 Установка / Installation

### Требования / Requirements

- **ОС / OS**: Linux (Debian 13+, Ubuntu 24.04+, Fedora 40+, Arch Linux)
- **Qt**: 5.15+ или Qt 6.x
- **Зависимости / Dependencies**:
  - tor, openvpn
  - tcpdump, conntrack
  - cmake, make, git
  - pkexec (polkit)

### Быстрая установка / Quick Install

```bash
git clone <repository-url>
cd TorManager
chmod +x install.sh
./install.sh
```

Скрипт автоматически определит дистрибутив и установит необходимые зависимости.

The script will automatically detect your distribution and install required dependencies.

---

## 📋 Использование / Usage

### Запуск / Launch

```bash
# Через меню приложений / Via application menu
Tor Manager

# Из терминала / From terminal
pkexec /usr/local/bin/TorManager
```

### Основные функции / Main Functions

1. **Запуск Tor / Start Tor**
   - Активация службы Tor с выбранными мостами
   - Enable Tor service with selected bridges

2. **Подключение VPN / Connect VPN**
   - Импорт и управление профилями OpenVPN
   - Import and manage OpenVPN profiles

3. **Мониторинг / Monitoring**
   - Просмотр DNS-запросов в реальном времени
   - Real-time DNS query viewing
   - Фильтрация по клиентам и категориям
   - Filter by clients and categories

4. **Отчёты / Reports**
   - Генерация отчётов по активности клиентов
   - Generate client activity reports
   - Экспорт статистики
   - Export statistics

5. **Telegram Bot**
   - Настройка бота через токен
   - Configure bot via token
   - Получение уведомлений
   - Receive notifications

---

## 🔧 Конфигурация / Configuration

Приложение использует следующие конфигурационные файлы:

Application uses the following configuration files:

- `~/.config/TorManager/config.ini` — основные настройки / main settings
- `/etc/tor/torrc` — конфигурация Tor / Tor configuration
- `/etc/openvpn/*.ovpn` — профили VPN / VPN profiles

---

## 📊 Примеры статистики / Statistics Examples

### Структура данных клиента / Client Data Structure

```cpp
struct DnsClientStats {
    QString clientName;              // Имя клиента / Client name
    qint64 totalRequests;            // Всего запросов / Total requests
    qint64 dnsRequests;              // DNS запросы / DNS requests
    qint64 httpRequests;             // HTTP запросы / HTTP requests
    qint64 httpsRequests;            // HTTPS запросы / HTTPS requests
    qint64 suspiciousRequests;       // Подозрительные / Suspicious
    QMap<QString, qint64> categoryCount;  // По категориям / By category
    QMap<QString, qint64> topDomains;     // Топ доменов / Top domains
    QDateTime lastRequest;           // Последний запрос / Last request
    QDateTime firstSeen;             // Первое появление / First seen
};
```

---

## 🛠️ Для разработчиков / For Developers

### Сборка из исходников / Build from Source

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### Структура проекта / Project Structure

```
TorManager/
├── main.cpp              # Точка входа / Entry point
├── mainwindow.cpp/h      # Главное окно / Main window
├── dns_monitor.cpp/h     # DNS мониторинг / DNS monitoring
├── client_stats.cpp/h    # Статистика клиентов / Client stats
├── tgbot_manager.cpp/h   # Telegram Bot / Telegram Bot
├── TorManager.desktop    # Desktop entry
├── CMakeLists.txt        # CMake конфигурация / CMake config
└── install.sh            # Скрипт установки / Install script
```

---

## 📝 Лицензия / License

Professional proprietary software / Профессиональное проприетарное ПО

---

## 📞 Поддержка / Support

Для вопросов и предложений обращайтесь в службу технической поддержки.

For questions and suggestions, please contact technical support.

---

## ⚠️ Отказ от ответственности / Disclaimer

Используйте данное программное обеспечение ответственно и в соответствии с законодательством вашей страны. Авторы не несут ответственности за неправомерное использование приложения.

Use this software responsibly and in accordance with the laws of your country. The authors are not responsible for misuse of the application.

---

**Версия / Version**: 1.0  
**Последнее обновление / Last Updated**: 2025  
**Платформа / Platform**: Linux (Qt 5/6)
