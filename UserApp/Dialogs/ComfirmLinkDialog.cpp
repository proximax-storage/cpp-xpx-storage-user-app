#include "ComfirmLinkDialog.h"
#include "Dialogs/ui_ComfirmLinkDialog.h"
#include "Utils.h"
#include "drive/Utils.h"
#include "qclipboard.h"
#include "qpushbutton.h"
#include "ui_ComfirmLinkDialog.h"

ComfirmLinkDialog::ComfirmLinkDialog( QWidget *parent
                                    , DataInfo dataInfo)
    : QDialog(parent)
    , ui(new Ui::ComfirmLinkDialog)
    , m_dataInfo(dataInfo)
{
    ui->setupUi(this);
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (okButton) {
        okButton->setText("Copy Link To Clipboard");
    }

    std::string str = sirius::drive::toString(m_dataInfo.m_driveKey);

    ui->m_driveKeyConfirmLabel->setText(QString::fromStdString(str));
    ui->m_pathConfirmationLabel->setText(QString::fromStdString(m_dataInfo.m_path));
    ui->m_dataSizeConfirmationLabel->setText(QString::fromStdString(std::to_string(m_dataInfo.m_totalSize)));
}

ComfirmLinkDialog::~ComfirmLinkDialog()
{
    delete ui;
}

void ComfirmLinkDialog::accept()
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

void ComfirmLinkDialog::reject()
{
    QDialog::reject();
}
