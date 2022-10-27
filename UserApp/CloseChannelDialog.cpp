#include "CloseChannelDialog.h"

CloseChannelDialog::CloseChannelDialog(QWidget *parent) :
    QMessageBox(parent)
{
    QMessageBox::setWindowTitle("Confirmation");
    setText("Are you confirm to remove channel?");
    setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    setDefaultButton(QMessageBox::Cancel);
    setButtonText(QMessageBox::Ok, "Confirm");
}
