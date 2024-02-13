#ifndef ADDDOWNLOADCHANNELDIALOG_H
#define ADDDOWNLOADCHANNELDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <qdebug.h>
#include <QMessageBox>
#include <xpxchaincpp/model/storage/drive.h>

#include "OnChainClient.h"
#include "Model.h"

namespace Ui {
class AddDownloadChannelDialog;
}

class MainWin;

class AddDownloadChannelDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDownloadChannelDialog(OnChainClient* onChainClient,
                                      Model* model,
                                      QWidget *parent = nullptr,
                                      std::string driveKey = "" );
    ~AddDownloadChannelDialog();

public:
    void accept() override;
    void reject() override;
    void displayInfo();

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
    Model*                          mpModel;
    QMessageBox*                    helpMessageBox = nullptr;

};

#endif // ADDDOWNLOADCHANNELDIALOG_H
