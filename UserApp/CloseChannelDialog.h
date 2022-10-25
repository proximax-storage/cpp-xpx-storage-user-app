#ifndef CLOSECHANNELDIALOG_H
#define CLOSECHANNELDIALOG_H

#include <QDialog>

namespace Ui {
class CloseChannelDialog;
}

class CloseChannelDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CloseChannelDialog(QWidget *parent = nullptr);
    ~CloseChannelDialog();

private:
    Ui::CloseChannelDialog *ui;
};

#endif // CLOSECHANNELDIALOG_H
