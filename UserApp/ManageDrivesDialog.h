#pragma once

#include <QDialog>

namespace Ui { class ManageDrivesDialog; }

class OnChainClient;

class ManageDrivesDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit ManageDrivesDialog( OnChainClient* onChainClient, QWidget *parent );
    ~ManageDrivesDialog() override;

signals:
    void updateDrives(const std::string& driveId, const std::string& oldLocalPath);

protected:
    void accept() override;
    void reject() override;

private:
    bool verify();

private:
    Ui::ManageDrivesDialog* ui;
    OnChainClient* m_onChainClient;
};
