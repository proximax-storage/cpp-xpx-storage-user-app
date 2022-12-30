#pragma once

#include <QDialog>

namespace Ui {
class ChannelInfoDialog;
}

struct DownloadChannel;

class ChannelInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChannelInfoDialog( const DownloadChannel& channel,
                                      QWidget *parent = nullptr);
    ~ChannelInfoDialog();

public:
    void accept() override;
    void reject() override;

signals:
    void addDownloadChannel(const std::string&             channelName,
                            const std::string&             channelKey,
                            const std::string&             driveKey,
                            const std::vector<std::string> allowedPublicKeys);

private:
    void validate();

private:
    Ui::ChannelInfoDialog*   ui;
};
