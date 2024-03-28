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
                             std::string driveKey,
                             QWidget *parent = nullptr);
    ~WizardAddStreamAnnounceDialog() override;

public:
    void accept() override;
    void reject() override;

    std::string streamFolderName() const { return mUniqueFolderName; };
    void setCreatedDrive(Drive* drive);
    void addDrivesToCBox();

private slots:
    void on_m_driveSelection_activated(int index);

private:
    void validate();

private:
    Ui::WizardAddStreamAnnounceDialog* ui;
    Model*                           m_model;
    OnChainClient*                   mp_onChainClient;
    std::string                      mDriveKey;
    std::string                      mUniqueFolderName;
};
