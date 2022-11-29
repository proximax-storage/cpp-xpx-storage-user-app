#pragma once

#include <QPushButton>
#include <QMenu>

class PopupMenu : public QMenu
{
    Q_OBJECT
public:
    explicit PopupMenu(QPushButton* button, QWidget* parent = 0);
    void showEvent(QShowEvent* event);
private:
    QPushButton* b;
};
