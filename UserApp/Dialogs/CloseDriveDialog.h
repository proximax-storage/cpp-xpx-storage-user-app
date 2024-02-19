#ifndef CLOSEDRIVEDIALOG_H
#define CLOSEDRIVEDIALOG_H

#include <QMessageBox>
#include <QPushButton>

#include "OnChainClient.h"
#include "Entities/Drive.h"

class CloseDriveDialog : public QMessageBox
{
    Q_OBJECT

public:
    explicit CloseDriveDialog(OnChainClient* onChainClient,
                              Drive* drive,
                              QWidget *parent = nullptr);

    ~CloseDriveDialog() override;

public:
    void accept() override;
    void reject() override;

private:
    OnChainClient* mpOnChainClient;
    Drive* mDrive;
};

#endif // CLOSEDRIVEDIALOG_H
