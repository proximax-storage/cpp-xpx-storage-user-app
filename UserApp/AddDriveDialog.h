#pragma once

#include <QDialog>
#include "OnChainClient.h"

namespace Ui { class AddDriveDialog; }

class AddDriveDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit AddDriveDialog( OnChainClient* onChainClient,
                             QWidget *parent, int editDriveIndex = -1 );
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

    int m_editDriveIndex;
    OnChainClient* mp_onChainClient;
};
