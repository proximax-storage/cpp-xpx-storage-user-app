#ifndef DOWNLOADPAYMENTDIALOG_H
#define DOWNLOADPAYMENTDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <qdebug.h>
#include <QMessageBox>
#include "OnChainClient.h"
#include "Model.h"

namespace Ui {
class DownloadPaymentDialog;
}

class DownloadPaymentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DownloadPaymentDialog(OnChainClient* onChainClient,
                                   Model* model,
                                   QWidget *parent = nullptr);
    ~DownloadPaymentDialog() override;

public:
    void accept() override;
    void reject() override;

private:
    void validate();

private:
    std::string mCurrentChannelKey;
    Ui::DownloadPaymentDialog *ui;
    OnChainClient* mpOnChainClient;
    Model* mpModel;
};

#endif // DOWNLOADPAYMENTDIALOG_H
