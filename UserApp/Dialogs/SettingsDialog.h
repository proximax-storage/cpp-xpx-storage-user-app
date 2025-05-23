#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>

#include "Entities/Settings.h"

namespace Ui { class SettingsDialog; }

class SettingsDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit SettingsDialog( Settings* settings, QWidget *parent, bool initSettings = false );
    ~SettingsDialog() override;

public:
signals:
    void closeLibtorrentPorts();

protected:
    void accept() override;
    void reject() override;

private slots:
    void onNewAccount();
    void onRemoveAccount();
    void onReset();

private:
    void updateAccountFields();
    void fillAccountCbox( bool initSettings );
    void validate();
    void initTooltips();

private:
    Ui::SettingsDialog* ui;
    Settings* mpSettings;
    Settings* mpSettingsDraft;
    QPushButton* mpReset;
};

#endif // SETTINGDIALOG_H
