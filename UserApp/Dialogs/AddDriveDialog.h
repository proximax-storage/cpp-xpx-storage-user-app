#pragma once

#include <QDialog>
#include "OnChainClient.h"
#include "Models/Model.h"

namespace Ui { class AddDriveDialog; }

class AddDriveDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit AddDriveDialog( OnChainClient* onChainClient,
                             Model* model,
                             QWidget *parent = nullptr);
    ~AddDriveDialog() override;

public:
    void accept() override;
    void reject() override;

private:
    void validate();

private:
    Ui::AddDriveDialog* ui;
    Model* mp_model;
    OnChainClient* mp_onChainClient;
};
