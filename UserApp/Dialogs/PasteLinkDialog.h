#pragma once

#include "OnChainClient.h"
#include <QDialog>

namespace Ui
{
class PasteLinkDialog;
}

class PasteLinkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasteLinkDialog(OnChainClient* onChainClient,
                             Model* model,
                             QWidget *parent = nullptr);
    ~PasteLinkDialog();

public:
    void accept() override;
    void reject() override;
    bool validate(QClipboard* clipboard);


private:
    Ui::PasteLinkDialog     *ui;
    Model*                  m_model;
    OnChainClient*          m_onChainClient;
    QPushButton*            m_pasteLinkButton;
    // QMessageBox*            helpMessageBox = nullptr;
};
