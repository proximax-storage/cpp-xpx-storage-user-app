#include "Worker.h"

Worker::Worker()
    : mIsInitialized(false)
{}

void Worker::init(int delay, bool isLimited) {
    mIsLimited = isLimited;
    if (mIsLimited) {
        mDelay = delay;
        mpTimer = new QTimer(this);
        connect(mpTimer, &QTimer::timeout, this, &Worker::handler);

        mIsInitialized = true;
        if (!mRequests.empty() && !mpTimer->isActive()) {
            mpTimer->start(mDelay);
        }
    }
}

void Worker::process(const QUuid& id, const std::function<QVariant()>& request) {
    if (mIsLimited) {
        mRequests.emplace(id, request);
        if (mIsInitialized && !mpTimer->isActive()) {
            mpTimer->start(mDelay);
        }
    } else {
        emit done(id, request());
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
    if (mIsLimited && mpTimer) {
        mpTimer->deleteLater();
    }
}