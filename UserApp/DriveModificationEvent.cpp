#include "DriveModificationEvent.h"

DriveModificationEvent::DriveModificationEvent(DriveState state)
    : QEvent(QEvent::Type(QEvent::User + 1))
    , mState(state)
{
}

DriveModificationEvent::~DriveModificationEvent()
{}

DriveState DriveModificationEvent::getState() const {
    return mState;
}