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
                             QWidget *parent, DriveInfo* editDrivePtr = nullptr );
    ~AddDriveDialog() override;

signals:
    void updateDrivesCBox();

protected:
    void accept() override;
    void reject() override;

private:
    bool verifyDriveName();
    bool verifyLocalDriveFolder();
    bool verifyKey();
    bool verify();

private:
    Ui::AddDriveDialog* ui;

    std::optional<DriveInfo> m_editDriveInfo;
    OnChainClient*           mp_onChainClient;
};
