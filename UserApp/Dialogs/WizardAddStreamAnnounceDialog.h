#pragma once

#include <QDialog>
#include "OnChainClient.h"
#include "Models/Model.h"

namespace Ui { class WizardAddStreamAnnounceDialog; }

class  WizardAddStreamAnnounceDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit WizardAddStreamAnnounceDialog( OnChainClient* onChainClient,
                             Model* model,
                             QString driveKey,
                             QWidget *parent = nullptr);
    ~WizardAddStreamAnnounceDialog() override;

public:
    void accept() override;
    void reject() override;

    QString streamFolderName() const { return mUniqueFolderName; };
    void onDriveCreated( const Drive* drive );
    void addDrivesToCBox();

private slots:
    void on_m_driveSelection_activated(int index);

private:
    void validate();

private:
    Ui::WizardAddStreamAnnounceDialog* ui;
    Model*                           m_model;
    OnChainClient*                   mp_onChainClient;
    QString                          mDriveKey;
    QString                          mUniqueFolderName;
};
