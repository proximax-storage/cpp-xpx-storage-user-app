#ifndef CANCELMODIFICATIONDIALOG_H
#define CANCELMODIFICATIONDIALOG_H

#include <QMessageBox>
#include <QPushButton>

#include "OnChainClient.h"

class CancelModificationDialog : public QMessageBox
{
    Q_OBJECT

public:
    explicit CancelModificationDialog(OnChainClient* onChainClient,
                                      const QString& driveId,
                                      const QString& alias,
                                      QWidget *parent = nullptr);

    ~CancelModificationDialog() = default;

public:
    void accept() override;
    void reject() override;

private:
    OnChainClient* mpOnChainClient;
    QString mDriveId;
};

#endif // CANCELMODIFICATIONDIALOG_H
