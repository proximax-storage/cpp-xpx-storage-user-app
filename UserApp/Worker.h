#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QTimer>
#include <QVariant>
#include <QUuid>

#include <queue>
#include <memory>
#include <thread>
#include <iostream>
#include <xpxchaincpp/model/storage/replicator.h>

class Worker : public QObject
{
    Q_OBJECT

public:
    Worker();
    ~Worker() override;

signals:
    void done(const QUuid& id, const QVariant& data);

public slots:
    void init(int delay, bool isLimited = true);
    void process(const QUuid& id, const std::function<QVariant()>& request);

private slots:
    void handler();

private:
    bool mIsLimited;
    bool mIsInitialized;
    int mDelay;
    QTimer *mpTimer;
    std::queue<std::pair<QUuid,std::function<QVariant()>>> mRequests;
};

#endif // WORKER_H
