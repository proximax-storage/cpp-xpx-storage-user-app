#pragma once

#include "Entities/DataInfo.h"
#include <QDialog>

namespace Ui
{
class ComfirmLinkDialog;
}

class ComfirmLinkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ComfirmLinkDialog( QWidget *parent   = nullptr
                              , DataInfo dataInfo = {} );
    ~ComfirmLinkDialog();

    void accept() override;
    void reject() override;

private:
    Ui::ComfirmLinkDialog*  ui;
    DataInfo                m_dataInfo;
};
