#include "client_stats.h"
#include <QDateTime>
#include <QSet>

ClientStats::ClientStats(QObject *parent) : QObject(parent)
{
}

void ClientStats::updateSpeed(const QString &clientId, qint64 bytesReceived,
                              qint64 bytesSent, qint64 intervalMs)
{
    if (intervalMs <= 0) return;

    QMutexLocker locker(&mutex);

    ClientSpeedData &data = speeds[clientId];

    // Добавляем проверку на разумные значения
    if (data.lastUpdateMs > 0) {
        double dt = qMax(1.0, intervalMs / 1000.0); // минимум 1 секунда

        qint64 rxDelta = qMax(0LL, bytesReceived - data.lastBytesRx);
        qint64 txDelta = qMax(0LL, bytesSent - data.lastBytesTx);

        // Ограничиваем максимальную скорость (например, 1 Гбит/с)
        const double MAX_SPEED = 125000000.0; // 1 Гбит/с в байтах

        double instantRx = qMin(rxDelta / dt, MAX_SPEED);
        double instantTx = qMin(txDelta / dt, MAX_SPEED);

        data.avgRxSpeed = data.avgRxSpeed * (1 - SMOOTHING_FACTOR) + instantRx * SMOOTHING_FACTOR;
        data.avgTxSpeed = data.avgTxSpeed * (1 - SMOOTHING_FACTOR) + instantTx * SMOOTHING_FACTOR;
    }

    data.lastBytesRx = bytesReceived;
    data.lastBytesTx = bytesSent;
    data.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();
}

double ClientStats::getAvgRxSpeed(const QString &clientId) const
{
    QMutexLocker locker(&mutex);
    return speeds.value(clientId).avgRxSpeed;
}

double ClientStats::getAvgTxSpeed(const QString &clientId) const
{
    QMutexLocker locker(&mutex);
    return speeds.value(clientId).avgTxSpeed;
}

void ClientStats::cleanup(const QSet<QString> &activeClients)
{
    QMutexLocker locker(&mutex);
    for (auto it = speeds.begin(); it != speeds.end();) {
        if (!activeClients.contains(it.key())) {
            it = speeds.erase(it);
        } else {
            ++it;
        }
    }
}
