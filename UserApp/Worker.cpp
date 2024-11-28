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
        return;
    }

    const auto frontRequest = mRequests.front();
    if (frontRequest.first.isNull() || !frontRequest.second) {
        qWarning() << "Worker::handler: Invalid request handler. Skipping.";
        mRequests.pop();
        return;
    }

    try {
        emit done(frontRequest.first, frontRequest.second());
    } catch (const std::exception& e) {
        qWarning() << "Worker::handler: Exception caught during signal emit: " << e.what();
    } catch (...) {
        qWarning() << "Worker::handler: Unknown exception caught during signal emit.";
    }

    mRequests.pop();
}

Worker::~Worker()
{
    if (mIsLimited && mpTimer) {
        mpTimer->deleteLater();
    }
}