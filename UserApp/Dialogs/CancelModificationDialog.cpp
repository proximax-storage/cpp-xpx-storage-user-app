#include "CancelModificationDialog.h"
#include "Utils.h"

CancelModificationDialog::CancelModificationDialog(OnChainClient* onChainClient,
                                                   const QString& driveId,
                                                   const QString& alias,
                                                   QWidget *parent)
    : QMessageBox(parent)
    , mpOnChainClient(onChainClient)
    , mDriveId(driveId)
{
    setWindowTitle("Confirmation");
    setText("Please confirm canceling last modification for the drive: " + alias);
    setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    setDefaultButton(QMessageBox::Cancel);
    button(QMessageBox::Ok)->setText("Confirm");

    connect(this->button(QMessageBox::Ok), &QPushButton::released, this, &CancelModificationDialog::accept);
    connect(this->button(QMessageBox::Cancel), &QPushButton::released, this, &CancelModificationDialog::reject);
}

void CancelModificationDialog::accept() {
    auto confirmationCallback = [](auto fee)
    {
        return showConfirmationDialog(fee);
    };

    mpOnChainClient->cancelDataModification(rawHashFromHex(mDriveId), confirmationCallback);
}

void CancelModificationDialog::reject() {
}
