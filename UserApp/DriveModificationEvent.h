#ifndef DRIVEMODIFICATIONEVENT_H
#define DRIVEMODIFICATIONEVENT_H

#include <QEvent>
#include "Drive.h"


class DriveModificationEvent : public QEvent {
    public:
        DriveModificationEvent(DriveState state);
        ~DriveModificationEvent() override;

    public:
    DriveState getState() const;

    private:
        const DriveState mState;
};


#endif //DRIVEMODIFICATIONEVENT_H
