#ifndef PRIVKEYDIALOG_H
#define PRIVKEYDIALOG_H

#include <QDialog>

namespace Ui { class PrivKeyDialog; }

class Settings;

class PrivKeyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrivKeyDialog( QWidget *parent, Settings& settings );
    ~PrivKeyDialog();

protected:
    void accept() override;
    void reject() override;

private slots:
    void onGenerateKeysBtn();
    void onLoadFromFileBtn();

private:
    void verify();

private:
    Ui::PrivKeyDialog*  ui;
    Settings&           m_settings;
};

#endif // PRIVKEYDIALOG_H
