#ifndef ADDDOWNLOADCHANNELDIALOG_H
#define ADDDOWNLOADCHANNELDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <qdebug.h>
#include <QMessageBox>
#include <xpxchaincpp/model/storage/drive.h>

#include "OnChainClient.h"
#include "Models/Model.h"

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
    ~AddDownloadChannelDialog() override;

public:
    void accept() override;
    void reject() override;
    void displayInfo();
    
private:
    void startCreateChannel();
    void validate();

private:
    std::string                     mCurrentDriveKey;
    Ui::AddDownloadChannelDialog*   ui;
    OnChainClient*                  mpOnChainClient;
    Model*                          mpModel;
    QMessageBox*                    helpMessageBox = nullptr;

    bool                            m_forStreaming = false;
    QDialog*                        m_channelIsCreatingDailog = nullptr;
};

#endif // ADDDOWNLOADCHANNELDIALOG_H
