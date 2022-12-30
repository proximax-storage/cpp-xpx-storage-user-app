#pragma once

#include <QDialog>

namespace Ui { class DriveInfoDialog; }

struct Drive;

class DriveInfoDialog : public QDialog
{
public:
    Q_OBJECT

public:
    explicit DriveInfoDialog(const Drive& drive,
                             QWidget *parent = nullptr);
    ~DriveInfoDialog() override;

public:
    void accept() override;
    void reject() override;

private:
    Ui::DriveInfoDialog* ui;
};
