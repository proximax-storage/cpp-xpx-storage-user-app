#ifndef DOWNLOADPAYMENTDIALOG_H
#define DOWNLOADPAYMENTDIALOG_H

#include <QDialog>

namespace Ui {
class DownloadPaymentDialog;
}

class DownloadPaymentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DownloadPaymentDialog(QWidget *parent = nullptr);
    ~DownloadPaymentDialog();

private:
    Ui::DownloadPaymentDialog *ui;
};

#endif // DOWNLOADPAYMENTDIALOG_H
