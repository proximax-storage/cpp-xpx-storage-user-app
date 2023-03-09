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
    explicit ChannelInfoDialog(QWidget *parent = nullptr);
    ~ChannelInfoDialog();

public:
    void init();
    void setName(const QString& name);
    void setId(const QString& id);
    void setDriveId(const QString& driveId);
    void setReplicators(const std::vector<std::string>& keys);
    void setKeys(const std::vector<std::string>& keys);
    void setPrepaid(const uint64_t prepaid);

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
    QString name;
    QString id;
    QString driveId;
    std::vector<std::string> replicators;
    std::vector<std::string> keys;
    uint64_t prepaid;
};
