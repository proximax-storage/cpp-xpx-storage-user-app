#include "ReplicatorOnBoardingDialog.h"
#include "ui_ReplicatorOnBoardingDialog.h"

ReplicatorOnBoardingDialog::ReplicatorOnBoardingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReplicatorOnBoardingDialog)
{
    ui->setupUi(this);
}

ReplicatorOnBoardingDialog::~ReplicatorOnBoardingDialog()
{
    delete ui;
}
