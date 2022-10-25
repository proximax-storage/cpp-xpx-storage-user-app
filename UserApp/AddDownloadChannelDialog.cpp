#include "AddDownloadChannelDialog.h"
#include "ui_adddownloadchanneldialog.h"

AddDownloadChannelDialog::AddDownloadChannelDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDownloadChannelDialog)
{
    ui->setupUi(this);
}

AddDownloadChannelDialog::~AddDownloadChannelDialog()
{
    delete ui;
}
