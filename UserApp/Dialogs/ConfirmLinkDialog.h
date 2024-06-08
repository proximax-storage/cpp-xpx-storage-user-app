#pragma once

#include "Entities/DataInfo.h"
#include <QDialog>

namespace Ui
{
class ConfirmLinkDialog;
}

class ConfirmLinkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfirmLinkDialog( QWidget *parent   = nullptr
                              , DataInfo dataInfo = {} );
    ~ConfirmLinkDialog();

    void accept() override;
    void reject() override;

private:
    Ui::ConfirmLinkDialog*  ui;
    DataInfo                m_dataInfo;
    bool                    m_itemNameChanged = false;
};
