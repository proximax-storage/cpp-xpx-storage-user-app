#include "CancelModificationDialog.h"

CancelModificationDialog::CancelModificationDialog(OnChainClient* onChainClient,
                                                   const QString& driveId,
                                                   const QString& alias,
                                                   QWidget *parent)
    : QMessageBox(parent)
    , mpOnChainClient(onChainClient)
    , mDriveId(driveId)
{
    setWindowTitle("Confirmation");
    setText("Please confirm canceling last modification for drive: " + alias);
    setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    setDefaultButton(QMessageBox::Cancel);
    setButtonText(QMessageBox::Ok, "Confirm");

    connect(this->button(QMessageBox::Ok), &QPushButton::released, this, &CancelModificationDialog::accept);
    connect(this->button(QMessageBox::Cancel), &QPushButton::released, this, &CancelModificationDialog::reject);
}

void CancelModificationDialog::accept() {
    mpOnChainClient->cancelDataModification(rawHashFromHex(mDriveId));
}

void CancelModificationDialog::reject() {
}
