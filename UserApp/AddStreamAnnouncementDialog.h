#pragma once

#include <QDialog>
#include "OnChainClient.h"
#include "Model.h"

namespace Ui { class AddStreamAnnouncementDialog; }

class AddStreamAnnouncementDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit AddStreamAnnouncementDialog( OnChainClient* onChainClient,
                             Model* model,
                             QWidget *parent = nullptr);
    ~AddStreamAnnouncementDialog() override;

public:
    void accept() override;
    void reject() override;

    std::string streamFolderName() const { return mUniqueFolderName; };

private:
    void validate();

private:
    Ui::AddStreamAnnouncementDialog* ui;
    Model*                           m_model;
    OnChainClient*                   mp_onChainClient;
    std::string                      mDriveKey;
    std::string                      mUniqueFolderName;
};
