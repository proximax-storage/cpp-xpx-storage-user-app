#ifndef REPLICATOROFFBOARDINGDIALOG_H
#define REPLICATOROFFBOARDINGDIALOG_H

#include <QDialog>

#include "OnChainClient.h"
#include "Models/Model.h"

namespace Ui {
class ReplicatorOffBoardingDialog;
}

class ReplicatorOffBoardingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplicatorOffBoardingDialog(OnChainClient* onChainClient,
                                         Model* model,
                                         QWidget *parent = nullptr);

    ~ReplicatorOffBoardingDialog();

public:
    void accept() override;
    void reject() override;

private:
    void validate();

private:
    Ui::ReplicatorOffBoardingDialog *ui;
    OnChainClient*                  mpOnChainClient;
    Model*                          mpModel;
};

#endif // REPLICATOROFFBOARDINGDIALOG_H
