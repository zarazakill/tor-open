#ifndef CLIENT_STATS_H
#define CLIENT_STATS_H

#include <QObject>
#include <QMap>
#include <QMutex>

class ClientStats : public QObject
{
    Q_OBJECT
public:
    explicit ClientStats(QObject *parent = nullptr);

    // Обновление скорости с экспоненциальным сглаживанием
    void updateSpeed(const QString &clientId, qint64 bytesReceived, qint64 bytesSent, qint64 intervalMs);

    // Получение сглаженных значений
    double getAvgRxSpeed(const QString &clientId) const;
    double getAvgTxSpeed(const QString &clientId) const;

    // Очистка для offline клиентов
    void cleanup(const QSet<QString> &activeClients);

private:
    struct ClientSpeedData {
        double avgRxSpeed = 0.0;
        double avgTxSpeed = 0.0;
        qint64 lastBytesRx = 0;
        qint64 lastBytesTx = 0;
        qint64 lastUpdateMs = 0;
    };

    mutable QMutex mutex;
    QMap<QString, ClientSpeedData> speeds;

    static constexpr double SMOOTHING_FACTOR = 0.3; // вес нового значения
};

#endif // CLIENT_STATS_H
