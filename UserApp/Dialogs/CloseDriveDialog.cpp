#include "CloseDriveDialog.h"

CloseDriveDialog::CloseDriveDialog(OnChainClient* onChainClient,
                                   Drive* drive,
                                   QWidget *parent)
    : QMessageBox(parent)
    , mDrive(drive)
    , mpOnChainClient(onChainClient)
{
    setWindowTitle("Confirmation");

    QString text;
    text.append("Please confirm drive '");
    text.append(mDrive->getName());
    text.append("' removal");
    setText(text);

    setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    setDefaultButton(QMessageBox::Cancel);
    button(QMessageBox::Ok)->setText("Confirm");

    connect(this->button(QMessageBox::Ok), &QPushButton::released, this, &CloseDriveDialog::accept);
    connect(this->button(QMessageBox::Cancel), &QPushButton::released, this, &CloseDriveDialog::reject);
}

CloseDriveDialog::~CloseDriveDialog()
{}

void CloseDriveDialog::accept()
{
    auto confirmationCallback = [this](auto fee)
    {
        if (showConfirmationDialog(fee)) {
            mDrive->updateDriveState(deleting);
            return true;
        }

        return false;
    };

    mpOnChainClient->closeDrive(rawHashFromHex(mDrive->getKey()), confirmationCallback);
    QDialog::accept();
}

void CloseDriveDialog::reject()
{
    QDialog::reject();
}
