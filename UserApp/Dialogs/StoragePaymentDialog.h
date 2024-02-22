#ifndef STORAGEPAYMENTDIALOG_H
#define STORAGEPAYMENTDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <qdebug.h>
#include <QMessageBox>
#include "OnChainClient.h"
#include "Models/Model.h"

namespace Ui {
class StoragePaymentDialog;
}

class StoragePaymentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StoragePaymentDialog(OnChainClient* onChainClient,
                                  Model* model,
                                  QWidget *parent = nullptr);
    ~StoragePaymentDialog() override;

public:
    void accept() override;
    void reject() override;

private:
    void validate();

private:
    std::string mCurrentDriveKey;
    Ui::StoragePaymentDialog *ui;
    OnChainClient* mpOnChainClient;
    Model* mpModel;
};

#endif // STORAGEPAYMENTDIALOG_H
