#ifndef DOWNLOADPAYMENTDIALOG_H
#define DOWNLOADPAYMENTDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <qdebug.h>
#include <QMessageBox>
#include "OnChainClient.h"

namespace Ui {
class DownloadPaymentDialog;
}

class DownloadPaymentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DownloadPaymentDialog(OnChainClient* onChainClient,
                                   QWidget *parent = nullptr);
    ~DownloadPaymentDialog() override;

public:
    void accept() override;
    void reject() override;

private:
    void validate();

private:
    Ui::DownloadPaymentDialog *ui;
    OnChainClient* mpOnChainClient;
};

#endif // DOWNLOADPAYMENTDIALOG_H
