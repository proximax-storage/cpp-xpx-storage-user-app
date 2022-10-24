#include "Worker.h"

void Worker::init(int delay) {
    mDelay = delay;
    mpTimer = new QTimer(this);
    connect(mpTimer, &QTimer::timeout, this, &Worker::handler);
}

void Worker::process(const QUuid& id, const std::function<QVariant()>& request) {
    mRequests.emplace(id, request);
    if (!mpTimer->isActive()) {
        mpTimer->start(mDelay);
    }
}

void Worker::handler() {
    if (mRequests.empty()) {
        mpTimer->stop();
    } else {
        const auto frontRequest = mRequests.front();
        emit done(frontRequest.first, frontRequest.second());
        mRequests.pop();
    }
}

Worker::~Worker()
{
    if (mpTimer) {
        mpTimer->deleteLater();
    }
}