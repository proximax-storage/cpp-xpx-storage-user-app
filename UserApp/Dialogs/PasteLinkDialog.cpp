#include "PasteLinkDialog.h"
#include "ui_PasteLinkDialog.h"

PasteLinkDialog::PasteLinkDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PasteLinkDialog)
{
    ui->setupUi(this);
}

PasteLinkDialog::~PasteLinkDialog()
{
    delete ui;
}
