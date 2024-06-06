#include "ConfirmLinkDialog.h"
#include "Dialogs/ui_ConfirmLinkDialog.h"
#include "Utils.h"
#include "drive/Utils.h"
#include "qclipboard.h"
#include "qpushbutton.h"
#include "ui_ConfirmLinkDialog.h"

ConfirmLinkDialog::ConfirmLinkDialog( QWidget *parent
                                    , DataInfo dataInfo)
    : QDialog(parent)
    , ui(new Ui::ConfirmLinkDialog)
    , m_dataInfo(dataInfo)
{
    ui->setupUi(this);
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (okButton) {
        okButton->setText("Copy Link To Clipboard");
    }

    std::string str = sirius::drive::toString(m_dataInfo.m_driveKey);

    ui->m_driveNameConfirmLabel->setText(QString::fromStdString(str));
    ui->m_pathConfirmLabel->setText(QString::fromStdString(m_dataInfo.m_path));
    ui->m_dataSizeConfirmLabel->setText(QString::fromStdString(std::to_string(m_dataInfo.m_totalSize)));
    ui->m_dataNameConfirmLabel->setText();
}

ConfirmLinkDialog::~ConfirmLinkDialog()
{
    delete ui;
}

void ConfirmLinkDialog::accept()
{
    std::string link = m_dataInfo.getLink();
    QClipboard* clipboard = QApplication::clipboard();
    if ( !clipboard ) {
        qWarning() << LOG_SOURCE << "bad clipboard";
        return;
    }
    clipboard->setText( QString::fromStdString(link), QClipboard::Clipboard );
    if ( clipboard->supportsSelection() )
    {
        clipboard->setText( QString::fromStdString(link), QClipboard::Selection );
    }
    reject();
}

void ConfirmLinkDialog::reject()
{
    QDialog::reject();
}
