#include "CloseChannelDialog.h"
#include "ui_closechanneldialog.h"

CloseChannelDialog::CloseChannelDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CloseChannelDialog)
{
    ui->setupUi(this);
}

CloseChannelDialog::~CloseChannelDialog()
{
    delete ui;
}
