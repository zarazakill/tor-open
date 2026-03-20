#ifndef DNS_MONITOR_H
#define DNS_MONITOR_H

#include <QObject>
#include <QProcess>
#include <QMap>
#include <QSet>
#include <QDateTime>
#include <QTimer>
#include <QTcpSocket>
#include <QMutex>
#include <QHostInfo>
#include <QElapsedTimer>
#include <QThread>
#include <QNetworkInterface>
#include <QRegularExpression>

// Структуры данных должны быть объявлены до класса

struct UrlAccess {
    QString clientName;
    QString clientIP;
    QString url;
    QDateTime timestamp;
    QString method;  // DNS, HTTPS, HTTP
    QString category; // Категория сайта (social, ads, analytics, etc)
};

// Структура для кэша с timestamp
struct CacheEntry {
    QString domain;
    QDateTime timestamp;
    QString category;
};

// Статистика по клиенту для DNS запросов - переименовано чтобы избежать конфликта с client_stats.h
struct DnsClientStats {
    QString clientName;
    qint64 totalRequests = 0;
    qint64 dnsRequests = 0;
    qint64 httpRequests = 0;
    qint64 httpsRequests = 0;
    qint64 suspiciousRequests = 0;
    QMap<QString, qint64> categoryCount; // social: 5, ads: 10, etc
    QMap<QString, qint64> topDomains;     // домен -> количество
    QDateTime lastRequest;
    QDateTime firstSeen;
};

// Категории доменов
struct DomainCategory {
    QString name;
    QStringList domains;
    QString color; // для отображения
};

class DnsMonitor : public QObject
{
    Q_OBJECT
public:
    explicit DnsMonitor(QObject *parent = nullptr);
    ~DnsMonitor();

    // Публичные методы (обновлены в соответствии с cpp)
    void startMonitoring(const QString &vpnNetwork);
    void stopMonitoring();
    void setClientMap(const QMap<QString, QString> &ipToClient);
    void setLastActiveClient(const QString &name, const QString &ip);
    QMap<QString, DnsClientStats> getClientStats() const;
    QStringList getSuspiciousDomains() const;
    void generateReport(const QString &clientName = "");
    bool isSuspiciousDomain(const QString &domain);
    QString detectTunInterface();
    bool isMonitoring() const { return tcpdumpProcess && tcpdumpProcess->state() == QProcess::Running; }


public slots:
    void updateClientMap(const QMap<QString, QString> &ipToClient);

signals:
    void urlAccessed(const UrlAccess &access);
    void logMessage(const QString &msg, const QString &type);
    // НОВЫЕ СИГНАЛЫ из cpp
    void suspiciousActivity(const QString &clientName, const QString &domain, const QString &reason);
    void statsUpdated(const QString &clientName, const DnsClientStats &stats);

private slots:
    // Слоты для tcpdump (были только в cpp, теперь объявлены)
    void onTcpdumpOutput();
    void onTcpdumpError();
    void onTcpdumpFinished();

    // Слоты для conntrack
    void pollConntrack();

    // Слоты для управления логами
    void flushLogBuffer();

    // Слоты для Tor ControlPort
    void onControlData();
    void onControlSocketConnected();
    void onControlSocketError();
    void onControlSocketTimeout();
    void sendTorCommand(const QString &cmd);
    void startTorController(); // Добавлено
    void parseAddrMap(const QString &line); // Добавлено

    // НОВЫЙ СЛОТ из cpp
    void updateStatsTimer();

private:
    // Источник 1: tcpdump
    QProcess *tcpdumpProcess = nullptr;
    void parseTcpdumpLine(const QString &line);
    void parseTlsSni(const QByteArray &packet, const QString &srcIP);
    QTimer *restartTimer = nullptr;
    int  restartAttempts   = 0;
    bool m_monitoringActive = false;  // false = остановлен, не перезапускать
    bool tcpdumpHasCaptured = false;
    int tcpdumpRestartDelay = 1000;
    qint64 lastPacketTime = 0;
    QTimer *stabilityTimer = nullptr;

    // Источник 2: Tor ControlPort
    QTcpSocket *torControl = nullptr;
    bool controlAuthenticated = false;
    bool controlConnecting = false;
    QTimer *controlReconnectTimer = nullptr;

    // Источник 3: conntrack
    QTimer *conntrackTimer = nullptr;
    QSet<QString> seenConntrackKeys;
    bool useIpConntrack = false;
    bool isVpnClient(const QString &ip) const;

    // Источник 4: DNS резолв (обратный)
    void reverseLookup(const QString &ip, const QString &srcIP, const QString &method);
    QMap<QString, QTimer*> pendingReverseLookups;

    // Кэш IP → домен с категорией
    QMap<QString, CacheEntry> ipToDomainCache;

    // Дедупликация
    QMap<QString, QDateTime> seenUrlTimestamps;
    static const int DEDUP_SECONDS = 30;

    // Данные клиентов
    QMap<QString, QString> clientIPMap;
    QString lastActiveClient;
    QString lastActiveClientIP;

    // Буфер логов
    QStringList logBuffer;
    QTimer *logFlushTimer = nullptr;
    QMutex logMutex;
    QMutex clientMapMutex;
    mutable QMutex cacheMutex;

    // Поля для парсинга TLS
    QByteArray m_currentPacket;
    QString m_currentSrcIP;
    qint64 m_currentPacketTime = 0;

    // статистика и категоризация
    QMap<QString, DnsClientStats> clientStats;
    QMap<QString, DomainCategory> domainCategories;
    QStringList suspiciousDomains;
    QSet<QString> warnedDomains;
    QTimer *statsUpdateTimer = nullptr;
    mutable QMutex statsMutex;

    // ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
    void initDomainCategories();
    QString categorizeDomain(const QString &domain);
    QString getDomainCategoryColor(const QString &category);
    void checkSuspiciousActivity(const QString &clientName, const QString &domain);
    void updateClientStats(const QString &clientName, const UrlAccess &access);
    void emitUrlAccess(const QString &srcIP, const QString &url, const QString &method);
    void cleanupOldEntries();
    bool isIgnoredDomain(const QString &domain);
    QString extractDomainFromReverse(const QString &ptr);

};

#endif // DNS_MONITOR_H
