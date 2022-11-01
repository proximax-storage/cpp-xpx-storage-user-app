#ifndef CLOSECHANNELDIALOG_H
#define CLOSECHANNELDIALOG_H

#include <QMessageBox>
#include <QPushButton>

#include "OnChainClient.h"

class CloseChannelDialog : public QMessageBox
{
    Q_OBJECT

public:
    explicit CloseChannelDialog(OnChainClient* onChainClient,
                                const QString& channelId,
                                QWidget *parent = nullptr);

    ~CloseChannelDialog() = default;

public:
    void accept() override;
    void reject() override;

private:
    OnChainClient* mpOnChainClient;
    QString mChannelId;
};

#endif // CLOSECHANNELDIALOG_H
