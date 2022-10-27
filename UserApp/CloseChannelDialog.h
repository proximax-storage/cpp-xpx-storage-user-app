#ifndef CLOSECHANNELDIALOG_H
#define CLOSECHANNELDIALOG_H

#include <QMessageBox>

class CloseChannelDialog : public QMessageBox
{
    Q_OBJECT

public:
    explicit CloseChannelDialog(QWidget *parent = nullptr);
    ~CloseChannelDialog() = default;

private:
};

#endif // CLOSECHANNELDIALOG_H
