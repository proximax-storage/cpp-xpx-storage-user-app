#pragma once

#include "OnChainClient.h"
#include "Entities/DataInfo.h"
#include <QDialog>

namespace Ui
{
class PasteLinkDialog;
}

class MainWin;

class PasteLinkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasteLinkDialog( MainWin* parent = nullptr );
    ~PasteLinkDialog();

public:
    void accept() override;
    void reject() override;
    bool validate(QClipboard* clipboard);


private:
    Ui::PasteLinkDialog     *ui;
    MainWin*                m_mainwin;
    QPushButton*            m_pasteLinkButton;
    std::optional<DataInfo> m_dataInfo;
};
