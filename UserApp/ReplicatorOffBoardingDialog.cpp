#include "ReplicatorOffBoardingDialog.h"
#include "ui_ReplicatorOffBoardingDialog.h"

ReplicatorOffBoardingDialog::ReplicatorOffBoardingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReplicatorOffBoardingDialog)
{
    ui->setupUi(this);
}

ReplicatorOffBoardingDialog::~ReplicatorOffBoardingDialog()
{
    delete ui;
}
