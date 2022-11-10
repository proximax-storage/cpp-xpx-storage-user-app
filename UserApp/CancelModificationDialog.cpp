#include "CancelModificationDialog.h"
#include "ui_CancelModificationDialog.h"

CancelModificationDialog::CancelModificationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CancelModificationDialog)
{
    ui->setupUi(this);
}

CancelModificationDialog::~CancelModificationDialog()
{
    delete ui;
}
