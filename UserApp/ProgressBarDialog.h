#ifndef PROGRESSBARDIALOG_H
#define PROGRESSBARDIALOG_H

#include <QDialog>

namespace Ui {
class ProgressBarDialog;
}

class ProgressBarDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProgressBarDialog(QWidget *parent = nullptr);
    ~ProgressBarDialog();

private:
    Ui::ProgressBarDialog *ui;
};

#endif // PROGRESSBARDIALOG_H
