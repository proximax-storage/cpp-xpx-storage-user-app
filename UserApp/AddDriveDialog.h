#pragma once

#include <QDialog>

namespace Ui { class AddDriveDialog; }

class AddDriveDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit AddDriveDialog( QWidget *parent, int editDriveIndex = -1 );
    ~AddDriveDialog() override;

protected:
    void accept() override;
    void reject() override;

private:
    bool verifyDriveName();
    bool verifyLocalDriveFolder();
    bool verifyKey();
    bool verify();

private:
    Ui::AddDriveDialog* ui;

    int m_editDriveIndex;
};
