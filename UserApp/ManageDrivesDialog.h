#pragma once

#include <QDialog>

namespace Ui { class ManageDrivesDialog; }

class ManageDrivesDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit ManageDrivesDialog( QWidget *parent );
    ~ManageDrivesDialog() override;

protected:
    void accept() override;
    void reject() override;

private:
    bool verify();

private:
    Ui::ManageDrivesDialog* ui;
};
