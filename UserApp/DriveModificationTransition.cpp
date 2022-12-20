#include "DriveModificationTransition.h"
#include "DriveModificationEvent.h"

DriveModificationTransition::DriveModificationTransition(int state)
    : mState(state)
{
}

DriveModificationTransition::~DriveModificationTransition()
{}

bool DriveModificationTransition::eventTest(QEvent *event)
{
    if (event && event->type() == QEvent::Type(QEvent::User + 1) ) {
        auto driveModificationEvent = dynamic_cast<DriveModificationEvent*>(event);
        return driveModificationEvent && (driveModificationEvent->getState() == mState);
    }

    return false;
}

void DriveModificationTransition::onTransition(QEvent *)
{}