#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>
#include <QRegExp>

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
    void validate();

private:
    Ui::SettingsDialog* ui;
};

#endif // SETTINGDIALOG_H
