#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>
#include "Settings.h"

namespace Ui { class SettingsDialog; }

class SettingsDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit SettingsDialog( QWidget *parent, bool initSettings = false );
    ~SettingsDialog() override;

protected:
    void accept() override;
    void reject() override;

private slots:
    void onNewAccountBtn();

private:
    void updateAccountFields();
    void fillAccountCbox( bool initSettings );
    bool verify();

private:
    Ui::SettingsDialog* ui;

    // not saved properties
    Settings m_settingsCopy;
};

#endif // SETTINGDIALOG_H
