#ifndef PRIVKEYDIALOG_H
#define PRIVKEYDIALOG_H

#include <QDialog>

namespace Ui { class PrivKeyDialog; }

class Settings;

class PrivKeyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrivKeyDialog( Settings* settings, QWidget *parent = nullptr );
    ~PrivKeyDialog() override;

protected:
    void accept() override;
    void reject() override;

private slots:
    void onGenerateKeysBtn();
    void onLoadFromFileBtn();

private:
    void init();
    void validate();
    bool isAccountExists();

private:
    Ui::PrivKeyDialog*  ui;
    Settings*           mp_settings;
};

#endif // PRIVKEYDIALOG_H
