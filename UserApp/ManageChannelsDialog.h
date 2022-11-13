#pragma once

#include <QDialog>

namespace Ui { class ManageChannelsDialog; }

class ManageChannelsDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit ManageChannelsDialog( QWidget *parent );
    ~ManageChannelsDialog() override;

protected:
    void accept() override;
    void reject() override;

signals:
    void updateChannels();

private:
    bool verify();

private:
    Ui::ManageChannelsDialog* ui;
};
