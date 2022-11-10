#ifndef CANCELMODIFICATIONDIALOG_H
#define CANCELMODIFICATIONDIALOG_H

#include <QDialog>

namespace Ui {
class CancelModificationDialog;
}

class CancelModificationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CancelModificationDialog(QWidget *parent = nullptr);
    ~CancelModificationDialog();

private:
    Ui::CancelModificationDialog *ui;
};

#endif // CANCELMODIFICATIONDIALOG_H
