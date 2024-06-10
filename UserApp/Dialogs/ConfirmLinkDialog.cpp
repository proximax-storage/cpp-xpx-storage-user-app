#include "ConfirmLinkDialog.h"
#include "Utils.h"
#include "qapplication.h"
#include "qclipboard.h"
#include "qpushbutton.h"

ConfirmLinkDialog::ConfirmLinkDialog( QWidget*  parent
                                    , DataInfo dataInfo)
    : QDialog(parent)
    , m_dataInfo(dataInfo)
{
    m_layout = new QGridLayout(this);

    driveName = new QLabel("Drive Name:");
    path = new QLabel("Path:");
    dataSize = new QLabel("Data Size:");
    folderNameForSaving = new QLabel("Folder Name For Saving:");

    m_driveNameConfirmLabel = new QLabel(QString::fromStdString(m_dataInfo.m_driveName));
    m_pathConfirmLabel = new QLabel(QString::fromStdString(m_dataInfo.m_path));
    m_dataSizeConfirmLabel = new QLabel(QString::fromStdString(std::to_string(m_dataInfo.m_totalSize)));
    m_dataNameConfirmEdit = new QLineEdit;

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    if (okButton) {
        okButton->setText("Copy Link To Clipboard");
    }
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfirmLinkDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConfirmLinkDialog::reject);

    setFontAndSize();
    setLayout();
    // just in case:
    // std::string str = sirius::drive::toString(m_dataInfo.m_driveKey);

    if(m_dataInfo.m_path == "/")
    {
        m_layout->addWidget(folderNameForSaving, 3, 0, Qt::AlignRight);
        m_layout->addWidget(m_dataNameConfirmEdit, 3, 1);
        m_dataNameConfirmEdit->setText(QString::fromStdString(m_dataInfo.m_driveName));  
    }
}

ConfirmLinkDialog::~ConfirmLinkDialog()
{
}

void ConfirmLinkDialog::accept()
{
    m_dataInfo.m_itemName = m_dataNameConfirmEdit->text().toStdString();
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

void ConfirmLinkDialog::setFontAndSize()
{
    QFont font;
    font.setPointSize(13);

    driveName->setFont(font);
    path->setFont(font);
    dataSize->setFont(font);
    folderNameForSaving->setFont(font);
    m_driveNameConfirmLabel->setFont(font);
    m_pathConfirmLabel->setFont(font);
    m_dataSizeConfirmLabel->setFont(font);
    m_dataNameConfirmEdit->setFont(font);
    buttonBox->setFont(font);

    QFontMetrics fontMetrics(font);
    int minHeight = fontMetrics.height() + fontMetrics.descent() + 2;
    driveName->setMinimumHeight(minHeight);
    path->setMinimumHeight(minHeight);
    dataSize->setMinimumHeight(minHeight);
    folderNameForSaving->setMinimumHeight(minHeight);
    m_driveNameConfirmLabel->setMinimumHeight(minHeight);
    m_pathConfirmLabel->setMinimumHeight(minHeight);
    m_dataSizeConfirmLabel->setMinimumHeight(minHeight);
    m_dataNameConfirmEdit->setMinimumHeight(minHeight);
    buttonBox->setMinimumHeight(minHeight);
}

void ConfirmLinkDialog::setLayout()
{
    m_layout->setSpacing(13);

    m_layout->addWidget(driveName, 0, 0, Qt::AlignRight);
    m_layout->addWidget(m_driveNameConfirmLabel, 0, 1);

    m_layout->addWidget(path, 1, 0, Qt::AlignRight);
    m_layout->addWidget(m_pathConfirmLabel, 1, 1);

    m_layout->addWidget(dataSize, 2, 0, Qt::AlignRight);
    m_layout->addWidget(m_dataSizeConfirmLabel, 2, 1);

    m_layout->addWidget(buttonBox, 4, 1, Qt::AlignRight);
}
