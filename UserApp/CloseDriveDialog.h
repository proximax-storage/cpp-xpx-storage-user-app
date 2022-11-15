#ifndef CLOSEDRIVEDIALOG_H
#define CLOSEDRIVEDIALOG_H

#include <QMessageBox>
#include <QPushButton>

#include "OnChainClient.h"

class CloseDriveDialog : public QMessageBox
{
    Q_OBJECT

public:
    explicit CloseDriveDialog(OnChainClient* onChainClient,
                              const QString& driveId,
                              const QString& alias,
                              QWidget *parent = nullptr);

    ~CloseDriveDialog() = default;

public:
    void accept() override;
    void reject() override;

private:
    OnChainClient* mpOnChainClient;
    QString mDriveId;
};

#endif // CLOSEDRIVEDIALOG_H
