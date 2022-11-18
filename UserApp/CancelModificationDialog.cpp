#include "CloseChannelDialog.h"

CloseChannelDialog::CloseChannelDialog(OnChainClient* onChainClient,
                                       const QString& channelId,
                                       const QString& alias,
                                       QWidget *parent)
    : QMessageBox(parent)
    , mChannelId(channelId)
    , mpOnChainClient(onChainClient)
{
    setWindowTitle("Confirmation");
    setText("Please confirm channel " + alias +  " removal");
    setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    setDefaultButton(QMessageBox::Cancel);
    setButtonText(QMessageBox::Ok, "Confirm");

    connect(this->button(QMessageBox::Ok), &QPushButton::released, this, &CloseChannelDialog::accept);
    connect(this->button(QMessageBox::Cancel), &QPushButton::released, this, &CloseChannelDialog::reject);
}

void CloseChannelDialog::accept() {
    mpOnChainClient->closeDownloadChannel(rawHashFromHex(mChannelId));
}

void CloseChannelDialog::reject() {
}
