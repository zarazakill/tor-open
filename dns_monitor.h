#ifndef DNS_MONITOR_H
#define DNS_MONITOR_H

#include <QObject>
#include <QProcess>
#include <QMap>
#include <QSet>
#include <QDateTime>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QTcpSocket>
#include <QStandardPaths>

struct UrlAccess {
    QString clientName;
    QString clientIP;
    QString url;
    QDateTime timestamp;
    QString method;  // DNS, HTTP, HTTPS
};

class DnsMonitor : public QObject
{
    Q_OBJECT
public:
    explicit DnsMonitor(QObject *parent = nullptr);
    ~DnsMonitor();

    void startMonitoring(const QString &vpnNetwork);
    void stopMonitoring();
    void setClientMap(const QMap<QString, QString> &ipToClient);
    void setLastActiveClient(const QString &name, const QString &ip);

signals:
    void urlAccessed(const UrlAccess &access);
    void logMessage(const QString &msg, const QString &type);

private slots:
    // Источник 1: tcpdump
    void onTcpdumpOutput();

    // Источник 2: Tor лог (файл/journalctl)
    void onTorLogOutput();
    void onTorLogFinished();

    // Источник 2b: Tor ControlPort
    void onControlData();

    // Источник 3: conntrack поллинг
    void pollConntrack();

private:
    // ── Источник 1: tcpdump ──────────────────────────────────────────────
    QProcess   *tcpdumpProcess   = nullptr;
    void parseTcpdumpLine(const QString &line);
    QString detectTunInterface();

    // ── Источник 2: Tor лог ──────────────────────────────────────────────
    QProcess   *torLogProcess    = nullptr;
    QString     torLogFile;
    QStringList buffer;
    QTimer     *flushTimer       = nullptr;
    void parseLine(const QString &line);

    // ── Источник 2b: Tor ControlPort ─────────────────────────────────────
    QTcpSocket *torControl           = nullptr;
    bool        controlAuthenticated = false;
    void startTorController();
    void parseAddrMap(const QString &line);

    // ── Источник 3: conntrack ─────────────────────────────────────────────
    QTimer     *conntrackTimer   = nullptr;
    QSet<QString> seenConntrackKeys;
    bool        useIpConntrack   = false;
    bool isVpnClient(const QString &ip) const;

    // ── Кэш обратного DNS: IP → домен (заполняется из DNS-запросов) ──────
    QMap<QString, QString> ipToDomainCache;  // dst IP → домен из DNS ANSWER
    // Дедупликация URL-записей: не показывать один и тот же домен/ip
    // одного клиента чаще раза в 30 сек
    QMap<QString, QDateTime> seenUrlTimestamps; // "clientIP|url|method" → last time

    // ── Общее ─────────────────────────────────────────────────────────────
    QMap<QString, QString> clientIPMap;   // VPN IP → имя клиента
    QString lastActiveClient;
    QString lastActiveClientIP;

    void emitUrlAccess(const QString &srcIP, const QString &url,
                       const QString &method);
};

#endif // DNS_MONITOR_H
