#pragma once

#include <QDialog>
#include "OnChainClient.h"
#include "Models/Model.h"
#include "qmessagebox.h"

namespace Ui { class AddDriveStreamDialog; }

class AddDriveStreamDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit AddDriveStreamDialog( OnChainClient* onChainClient,
                             Model* model,
                             QWidget *parent = nullptr
                            );
    ~AddDriveStreamDialog() override;

public:
    void accept() override;
    void reject() override;
    void displayInfo();

private:
    void validate();

private:
    Ui::AddDriveStreamDialog*   ui;
    Model*                      mp_model;
    OnChainClient*              mp_onChainClient;
    QMessageBox*                helpMessageBox = nullptr;
};
