#include "ReplicatorOffBoardingDialog.h"
#include "ui_ReplicatorOffBoardingDialog.h"

ReplicatorOffBoardingDialog::ReplicatorOffBoardingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReplicatorOffBoardingDialog)
{
    ui->setupUi(this);
    setWindowTitle("Replicator offboarding");
}

ReplicatorOffBoardingDialog::~ReplicatorOffBoardingDialog()
{
    delete ui;
}
