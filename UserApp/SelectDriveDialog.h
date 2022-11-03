#pragma once

#include <QDialog>
#include <functional>

namespace Ui {
class SelectDriveDialog;
}

class SelectDriveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectDriveDialog( std::function<void( const QString&)> onSelect, QWidget *parent = nullptr );
    ~SelectDriveDialog();

    void accept() override;

private:
    Ui::SelectDriveDialog* ui;
    std::function<void( const QString&)> m_onSelect;
};

