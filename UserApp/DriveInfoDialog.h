#pragma once

#include <QDialog>

namespace Ui { class DriveInfoDialog; }

struct DriveInfo;

class DriveInfoDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit DriveInfoDialog( const DriveInfo&,
                             QWidget *parent = nullptr);
    ~DriveInfoDialog() override;

signals:
    void updateDrivesCBox();

public:
    void accept() override;
    void reject() override;

private:
    void validate();

private:
    Ui::DriveInfoDialog* ui;
};
