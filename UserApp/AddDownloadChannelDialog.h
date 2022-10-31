#ifndef ADDDOWNLOADCHANNELDIALOG_H
#define ADDDOWNLOADCHANNELDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <qdebug.h>
#include <QMessageBox>
#include <xpxchaincpp/model/storage/drive.h>

#include "OnChainClient.h"

namespace Ui {
class AddDownloadChannelDialog;
}

class AddDownloadChannelDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDownloadChannelDialog(OnChainClient* onChainClient,
                                      const QStringList& drives,
                                      QWidget *parent = nullptr);
    ~AddDownloadChannelDialog();

public:
    void accept() override;
    void reject() override;

private:
    Ui::AddDownloadChannelDialog *ui;
    OnChainClient* mpOnChainClient;
};

#endif // ADDDOWNLOADCHANNELDIALOG_H
