#include "DownloadPaymentDialog.h"
#include "ui_DownloadPaymentDialog.h"

DownloadPaymentDialog::DownloadPaymentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DownloadPaymentDialog)
{
    ui->setupUi(this);
}

DownloadPaymentDialog::~DownloadPaymentDialog()
{
    delete ui;
}
