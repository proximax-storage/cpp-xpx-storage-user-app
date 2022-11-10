#ifndef STORAGEPAYMENTDIALOG_H
#define STORAGEPAYMENTDIALOG_H

#include <QDialog>

namespace Ui {
class StoragePaymentDialog;
}

class StoragePaymentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StoragePaymentDialog(QWidget *parent = nullptr);
    ~StoragePaymentDialog();

private:
    Ui::StoragePaymentDialog *ui;
};

#endif // STORAGEPAYMENTDIALOG_H
