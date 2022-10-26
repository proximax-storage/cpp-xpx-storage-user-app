#pragma once

#include <QDialog>

namespace Ui { class AddDriveDialog; }

class AddDriveDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit AddDriveDialog( QWidget *parent, bool initSettings = false );
    ~AddDriveDialog() override;

protected:
    void accept() override;
    void reject() override;

private:
    bool verify();

private:
    Ui::AddDriveDialog* ui;
};
