#include "StoragePaymentDialog.h"
#include "ui_StoragePaymentDialog.h"

StoragePaymentDialog::StoragePaymentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StoragePaymentDialog)
{
    ui->setupUi(this);
}

StoragePaymentDialog::~StoragePaymentDialog()
{
    delete ui;
}
