#!/bin/bash
# Скрипт установки Tor Manager with OpenVPN
# Debian 13 / Ubuntu / Fedora / Arch

set -e
set -u

echo "======================================"
echo "  Установка Tor Manager with OpenVPN"
echo "======================================"
echo

# ── Проверка: не запускать от root ──────────────────────────────────
if [[ $EUID -eq 0 ]]; then
    echo "ОШИБКА: Не запускайте скрипт от root. sudo будет запрошен при необходимости."
    exit 1
fi

# ── Определение дистрибутива ────────────────────────────────────────
if [ ! -f /etc/os-release ]; then
    echo "ОШИБКА: Не удалось определить дистрибутив Linux (/etc/os-release не найден)"
    exit 1
fi

. /etc/os-release
OS="${ID:-unknown}"
VERSION="${VERSION_ID:-unknown}"
echo "Обнаружен дистрибутив: $OS $VERSION"
echo

# ── Проверка наличия базовых инструментов ───────────────────────────
for cmd in cmake make git; do
    if ! command -v "$cmd" &>/dev/null; then
        echo "Будет установлен: $cmd"
    fi
done

# ── Установка зависимостей ──────────────────────────────────────────
echo "Шаг 1: Установка зависимостей..."

case "$OS" in
    ubuntu|debian)
        # Debian 13 (Trixie) и новее используют Qt6
        VERSION_NUM="${VERSION%%.*}"
        if [[ "$OS" == "debian" && "${VERSION_NUM:-0}" -ge 13 ]] || \
           [[ "$OS" == "ubuntu" && "${VERSION%%.*}" -ge 24 ]]; then
            echo "Используем Qt6..."
            QT_PKG="qt6-base-dev qt6-positioning-dev"
        else
            echo "Используем Qt5..."
            QT_PKG="qtbase5-dev qttools5-dev"
        fi

        # Ждём если apt заблокирован другим процессом
        MAX_WAIT=60
        WAITED=0
        while fuser /var/lib/dpkg/lock-frontend &>/dev/null 2>&1; do
            if [ $WAITED -ge $MAX_WAIT ]; then
                echo "ОШИБКА: apt заблокирован более ${MAX_WAIT}с. Закройте другие установщики."
                exit 1
            fi
            echo "Ожидание освобождения apt lock... (${WAITED}s)"
            sleep 5
            WAITED=$((WAITED + 5))
        done

        sudo apt-get update -q || true   # игнорируем ошибки сторонних репозиториев (nodesource и т.д.)
        sudo apt-get install -y --no-install-recommends \
            build-essential cmake git \
            $QT_PKG \
            tor openvpn obfs4proxy
        ;;

    fedora)
        sudo dnf install -y gcc-c++ cmake git \
            qt6-qtbase-devel qt6-qtpositioning-devel \
            tor openvpn obfs4
        ;;

    arch|manjaro)
        sudo pacman -S --needed --noconfirm \
            base-devel cmake git \
            qt6-base qt6-positioning \
            tor openvpn obfs4proxy
        ;;

    *)
        echo "ОШИБКА: Неподдерживаемый дистрибутив: $OS"
        echo "Установите зависимости вручную:"
        echo "  Qt6 (или Qt5), cmake, make, gcc, tor, openvpn"
        exit 1
        ;;
esac

# ── Проверка что ключевые зависимости установлены ───────────────────
echo
echo "Проверка установленных зависимостей..."

# Функция: ищет программу в PATH и в стандартных системных каталогах
find_tool() {
    command -v "$1" &>/dev/null && return 0
    for dir in /usr/sbin /sbin /usr/local/sbin /usr/bin /usr/local/bin; do
        [ -x "$dir/$1" ] && return 0
    done
    return 1
}

MISSING=""
for cmd in cmake make g++ tor openvpn; do
    if ! find_tool "$cmd"; then
        MISSING="$MISSING $cmd"
    fi
done

if [ -n "$MISSING" ]; then
    echo "ОШИБКА: Следующие программы не найдены после установки:$MISSING"
    echo "Проверьте вывод apt/dnf/pacman выше на наличие ошибок."
    exit 1
fi
echo "Все зависимости в наличии"

# ── Очистка предыдущей сборки ───────────────────────────────────────
echo
echo "Шаг 2: Очистка предыдущей сборки..."
if [ -d "build" ]; then
    echo "Удаление старой папки build..."
    rm -rf build
fi

# ── Сборка ──────────────────────────────────────────────────────────
echo
echo "Шаг 3: Сборка проекта..."
mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Release .. 2>&1 | tail -20

NPROC=$(nproc 2>/dev/null || echo 2)
make -j"$NPROC"

# ── Установка ───────────────────────────────────────────────────────
echo
echo "Шаг 4: Установка приложения..."
sudo make install

# ── Итог ────────────────────────────────────────────────────────────
echo
echo "======================================"
echo "  Установка завершена успешно!"
echo "======================================"
echo
echo "Запуск приложения:"
echo "  sudo TorManager"
echo
echo "Или через меню приложений: 'Tor Manager'"
echo
echo "Лог ошибок приложения:"
echo "  ~/.local/share/TorManager/TorVPN/tormanager_errors.log"
echo
