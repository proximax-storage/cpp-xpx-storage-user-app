#ifndef ADDDOWNLOADCHANNELDIALOG_H
#define ADDDOWNLOADCHANNELDIALOG_H

#include <QDialog>

namespace Ui {
class AddDownloadChannelDialog;
}

class AddDownloadChannelDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDownloadChannelDialog(QWidget *parent = nullptr);
    ~AddDownloadChannelDialog();

private:
    Ui::AddDownloadChannelDialog *ui;
};

#endif // ADDDOWNLOADCHANNELDIALOG_H
