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

class MainWin;

class AddDownloadChannelDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDownloadChannelDialog(OnChainClient* onChainClient,
                                      QWidget *parent = nullptr);
    ~AddDownloadChannelDialog();

public:
    void accept() override;
    void reject() override;

signals:
    void addDownloadChannel(const std::string&             channelName,
                            const std::string&             channelKey,
                            const std::string&             driveKey,
                            const std::vector<std::string> allowedPublicKeys);

private:
    void validate();

private:
    std::string                     mCurrentDriveKey;
    Ui::AddDownloadChannelDialog*   ui;
    OnChainClient*                  mpOnChainClient;
};

#endif // ADDDOWNLOADCHANNELDIALOG_H
