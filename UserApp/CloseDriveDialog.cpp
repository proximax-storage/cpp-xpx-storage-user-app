#include "CloseDriveDialog.h"

CloseDriveDialog::CloseDriveDialog(OnChainClient* onChainClient,
                                   const QString& driveId,
                                   QWidget *parent)
    : QMessageBox(parent)
    , mDriveId(driveId)
    , mpOnChainClient(onChainClient)
{
    setWindowTitle("Confirmation");
    setText("Are you confirm to remove drive?");
    setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    setDefaultButton(QMessageBox::Cancel);
    setButtonText(QMessageBox::Ok, "Confirm");

    connect(this->button(QMessageBox::Ok), &QPushButton::released, this, &CloseDriveDialog::accept);
    connect(this->button(QMessageBox::Cancel), &QPushButton::released, this, &CloseDriveDialog::reject);
}

void CloseDriveDialog::accept() {
    mpOnChainClient->closeDrive(rawHashFromHex(mDriveId));
}

void CloseDriveDialog::reject() {
}
