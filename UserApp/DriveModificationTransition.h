#ifndef DRIVEMODIFICATIONTRANSITION_H
#define DRIVEMODIFICATIONTRANSITION_H


#include <QAbstractTransition>

class DriveModificationTransition : public QAbstractTransition {
    Q_OBJECT

public:
        explicit DriveModificationTransition(int state);
        ~DriveModificationTransition() override;

    protected:
        bool eventTest(QEvent* e) override;
        void onTransition(QEvent*) override;

    private:
        int mState;
};


#endif //DRIVEMODIFICATIONTRANSITION_H
