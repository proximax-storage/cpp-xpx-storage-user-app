#ifndef REPLICATORONBOARDINGDIALOG_H
#define REPLICATORONBOARDINGDIALOG_H

#include <QDialog>

#include "OnChainClient.h"
#include "Models/Model.h"

namespace Ui {
class ReplicatorOnBoardingDialog;
}

class ReplicatorOnBoardingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplicatorOnBoardingDialog(OnChainClient* onChainClient,
                                        Model* model,
                                        QWidget *parent = nullptr);

    ~ReplicatorOnBoardingDialog();

public:
    void accept() override;
    void reject() override;

private:
    bool isReplicatorExists() const;
    std::string getReplicatorPublicKey() const;
    void validate();

private:
    Ui::ReplicatorOnBoardingDialog *ui;
    OnChainClient*                  mpOnChainClient;
    Model*                          mpModel;
};

#endif // REPLICATORONBOARDINGDIALOG_H
