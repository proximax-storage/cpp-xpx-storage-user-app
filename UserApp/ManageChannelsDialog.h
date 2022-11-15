#pragma once

#include <QDialog>
#include <OnChainClient.h>

namespace Ui { class ManageChannelsDialog; }

class ManageChannelsDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit ManageChannelsDialog( OnChainClient* onChainClient, QWidget *parent );
    ~ManageChannelsDialog() override;

protected:
    void accept() override;
    void reject() override;

signals:
    void updateChannels();
    void addDownloadChannel(const std::string&             channelName,
                            const std::string&             channelKey,
                            const std::string&             driveKey,
                            const std::vector<std::string> allowedPublicKeys);

private:
    bool verify();

private:
    Ui::ManageChannelsDialog* ui;
    OnChainClient* mpOnChainClient;
};
