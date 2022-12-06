#ifndef REPLICATORONBOARDINGDIALOG_H
#define REPLICATORONBOARDINGDIALOG_H

#include <QDialog>

namespace Ui {
class ReplicatorOnBoardingDialog;
}

class ReplicatorOnBoardingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplicatorOnBoardingDialog(QWidget *parent = nullptr);
    ~ReplicatorOnBoardingDialog();

private:
    Ui::ReplicatorOnBoardingDialog *ui;
};

#endif // REPLICATORONBOARDINGDIALOG_H
