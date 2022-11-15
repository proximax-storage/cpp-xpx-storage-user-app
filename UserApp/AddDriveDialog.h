#pragma once

#include <QDialog>
#include "OnChainClient.h"
#include "Model.h"

namespace Ui { class AddDriveDialog; }

class AddDriveDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit AddDriveDialog( OnChainClient* onChainClient,
                             QWidget *parent = nullptr);
    ~AddDriveDialog() override;

signals:
    void updateDrivesCBox();

public:
    void accept() override;
    void reject() override;

private:
    void validate();

private:
    Ui::AddDriveDialog* ui;
    OnChainClient* mp_onChainClient;
};
