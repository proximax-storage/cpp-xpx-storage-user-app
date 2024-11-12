#include "ConfirmLinkDialog.h"
#include "Utils.h"
#include "qapplication.h"
#include "qclipboard.h"
#include "qpushbutton.h"

#include <qmessagebox.h>

ConfirmLinkDialog::ConfirmLinkDialog( QWidget*  parent
                                    , DataInfo dataInfo)
    : QDialog(parent)
    , m_dataInfo(dataInfo)
{
    setModal(true);
    m_layout = new QGridLayout(this);

    driveName = new QLabel("Drive Name:");
    path = new QLabel("Path:");
    dataSize = new QLabel("Data Size:");
    folderNameForSaving = new QLabel("Folder Name For Saving:");

    m_driveNameConfirmLabel = new QLabel(m_dataInfo.m_driveName);
    m_pathConfirmLabel = new QLabel(m_dataInfo.m_path);
    m_dataSizeConfirmLabel = new QLabel(dataSizeToString(m_dataInfo.m_totalSize));
    m_folderNameConfirmEdit = new QLineEdit("");

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    if (okButton) {
        okButton->setText("Copy Link To Clipboard");
    }

    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfirmLinkDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConfirmLinkDialog::reject);

    setLayout();
    // just in case if you want to display drive key in UI:
    // std::string str = sirius::drive::toString(m_dataInfo.m_driveKey);

    if(m_dataInfo.m_path == "/")
    {
        m_layout->addWidget(folderNameForSaving, 3, 0, Qt::AlignRight);
        m_layout->addWidget(m_folderNameConfirmEdit, 3, 1);
        m_folderNameConfirmEdit->setText(m_dataInfo.m_driveName);
    }
}

ConfirmLinkDialog::~ConfirmLinkDialog() = default;

bool ConfirmLinkDialog::contains_invalid_chars(const QString& filename, const QString& invalid_chars) {
    const auto utf8buffer = qStringToStdStringUTF8(filename);
    return std::any_of(utf8buffer.begin(), utf8buffer.end(), [&](char c) {
        return qStringToStdStringUTF8(invalid_chars).find(c) != std::string::npos;
    });
}

bool ConfirmLinkDialog::isValidFolderName(const QString& filename) {
//    const QString windows_invalid_chars = "<>:\"/\\|?*";
//    if (contains_invalid_chars(filename, windows_invalid_chars)) {
//        return false;
//    }
//
//    const std::string reserved_names[] = {
//        "CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
//        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9", ".."
//    };
//
//    auto upper_filename = qStringToStdStringUTF8(filename);
//    std::transform(upper_filename.begin(), upper_filename.end(), upper_filename.begin(), ::toupper);
//    for (const auto& reserved_name : reserved_names) {
//        if (upper_filename == reserved_name) {
//            return false;
//        }
//    }
//
//    const QString unix_invalid_chars = "\0/";
//    if (contains_invalid_chars(filename, unix_invalid_chars)) {
//        return false;
//    }
//
//    // Length check (optional, depending on file system limitations)
//    if (filename.length() > 255) { // Example limit, can be adjusted
//        return false;
//    }
//
//    try {
//        const auto fileNameUtf8 = qStringToStdStringUTF8(filename);
//        std::filesystem::path p(fileNameUtf8);
//        return !p.empty() && p.filename() == fileNameUtf8;
//    }
//    catch (const std::filesystem::filesystem_error&) {
//        return false;
//    }

    return true;
}

void ConfirmLinkDialog::accept()
{
    if( (m_dataInfo.m_path == "/") && !isValidFolderName(m_folderNameConfirmEdit->text()) )
    {
        QMessageBox msgBox(QMessageBox::Warning
                           , "Warning"
                           , m_folderNameConfirmEdit->text() + ": incorrect folder name!"
                           , QMessageBox::Ok, this);

        connect(msgBox.button(QMessageBox::Ok), &QPushButton::clicked, this, [&]() {
            m_folderNameConfirmEdit->setText(m_dataInfo.m_driveName);
        });

        msgBox.exec();
    }
    else
    {
        m_dataInfo.m_itemName = m_folderNameConfirmEdit->text();
        QString link = m_dataInfo.getLink();
        QClipboard* clipboard = QApplication::clipboard();
        if ( !clipboard ) {
            qWarning() << "ConfirmLinkDialog::accept: bad clipboard";
            return;
        }

        clipboard->setText(link, QClipboard::Clipboard );
        if ( clipboard->supportsSelection() )
        {
            clipboard->setText(link, QClipboard::Selection );
        }

        reject();
    }
}

void ConfirmLinkDialog::reject()
{
    QDialog::reject();
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
