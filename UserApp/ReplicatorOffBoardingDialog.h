#ifndef REPLICATOROFFBOARDINGDIALOG_H
#define REPLICATOROFFBOARDINGDIALOG_H

#include <QDialog>

namespace Ui {
class ReplicatorOffBoardingDialog;
}

class ReplicatorOffBoardingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplicatorOffBoardingDialog(QWidget *parent = nullptr);
    ~ReplicatorOffBoardingDialog();

private:
    Ui::ReplicatorOffBoardingDialog *ui;
};

#endif // REPLICATOROFFBOARDINGDIALOG_H
