#pragma once

#include <QDialog>
#include <functional>

namespace Ui {
class SelectEntityDialog;
}

class SelectEntityDialog : public QDialog
{
    Q_OBJECT
public:
    enum Entity {
        Drive,
        Channel
    };

public:
    explicit SelectEntityDialog(Entity entity, std::function<void(const QString&)> onSelect, QWidget *parent = nullptr );
    ~SelectEntityDialog();

    void accept() override;

private:
    Ui::SelectEntityDialog* ui;
    std::function<void( const QString&)> m_onSelect;
};

