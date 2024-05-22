#pragma once

#include <QDialog>

namespace Ui
{
class PasteLinkDialog;
}

class PasteLinkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasteLinkDialog(QWidget *parent = nullptr);
    ~PasteLinkDialog();

private:
    Ui::PasteLinkDialog *ui;
};
