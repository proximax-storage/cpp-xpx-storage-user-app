#pragma once

#include <QDialog>
#include "OnChainClient.h"
#include "Model.h"

namespace Ui { class AddStreamRefDialog; }

class AddStreamRefDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit AddStreamRefDialog( OnChainClient* onChainClient,
                             Model* model,
                             QWidget *parent = nullptr);
    ~AddStreamRefDialog() override;

public:
    void accept() override;
    void reject() override;

private:
    void validate();

private:
    Ui::AddStreamRefDialog* ui;
    Model* mpModel;
    OnChainClient* mp_onChainClient;
};
