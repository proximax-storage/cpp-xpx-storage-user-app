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
    void setReplicators(const std::vector<QString>& keys);
    void setKeys(const std::vector<QString>& keys);
    void setPrepaid(const uint64_t prepaid);

public:
    void accept() override;
    void reject() override;

signals:
    void addDownloadChannel(const QString&             channelName,
                            const QString&             channelKey,
                            const QString&             driveKey,
                            const std::vector<QString> allowedPublicKeys);

private:
    void validate();

private:
    Ui::ChannelInfoDialog*   ui;
    QString name;
    QString id;
    QString driveId;
    std::vector<QString> replicators;
    std::vector<QString> keys;
    uint64_t prepaid;
};
