#include "PasteLinkDialog.h"
#include "mainwin.h"
#include "qapplication.h"
#include "Models/Model.h"
#include <QClipboard>
#include <QMessageBox>
#include <QPushButton>

PasteLinkDialog::PasteLinkDialog( MainWin* parent, Model* model )
    : QDialog( parent )
    , m_mainwin(parent)
    , m_model(model)
{
    setModal(true);
    m_layout = new QGridLayout(this);

    m_linkEdit = new QTextEdit;
    m_linkEdit->setMaximumHeight(70);
    m_linkEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_linkEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_linkEdit->setStyleSheet("background-color: gainsboro; color: dimgray;");
    m_linkEdit->setText("Link will be pasted here.");
    m_linkEdit->setReadOnly(true);

    path= new QLabel("Path:");
    dataSize = new QLabel("Data Size:");
    folderName = new QLabel("Folder Name:");
    m_pathLabel = new QLabel("");
    m_dataSizeLabel = new QLabel("");
    m_folderNameLabel = new QLabel("");

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
    if(okButton) {
        okButton->setText("Download");
    }
    m_pasteLinkButton = new QPushButton("Paste Link", this);
    buttonBox->addButton(m_pasteLinkButton, QDialogButtonBox::ActionRole);

    setFontAndSize();
    m_layout->setSpacing(13);
    m_layout->addWidget(m_linkEdit, 0, 0, 1, 2);
    m_layout->addWidget(buttonBox, 1, 1, Qt::AlignRight);

    connect(m_pasteLinkButton, &QPushButton::clicked, this, [&](){
        QClipboard* clipboard = QApplication::clipboard();
        if (!clipboard) {
            qWarning() << LOG_SOURCE << "bad clipboard";
            return;
        }
        try
        {
            m_linkEdit->setStyleSheet("");
            m_linkEdit->setText(clipboard->text());

            m_dataInfo.parseLink( clipboard->text().toStdString() );
            m_dataInfoIsSet = true;

            setLayout();
            m_pathLabel->setText(QString::fromStdString(m_model->getDownloadFolder().string() + m_dataInfo.m_path));

            std::string sizeInMb = std::to_string( m_dataInfo.m_totalSize/double(1000000));
            m_dataSizeLabel->setText(QString::fromStdString(sizeInMb.substr(0, sizeInMb.find('.') + 3)  + " Mb"));
            if(!m_dataInfo.m_itemName.empty())
            {
                m_folderNameLabel->setText(QString::fromStdString(m_dataInfo.m_itemName));
                m_layout->addWidget(folderName, 1, 0, Qt::AlignRight);
                m_layout->addWidget(m_folderNameLabel, 1, 1);
            }
        }
        catch(...)
        {
            QMessageBox msgBox(QMessageBox::Warning
                              , "Warning"
                              , m_linkEdit->toPlainText() + ": incorrect link!"
                              , QMessageBox::Ok, this);
            connect(msgBox.button(QMessageBox::Ok), &QPushButton::clicked, this, [&]() {
                m_linkEdit->setText("Link will be pasted here.");
                m_linkEdit->setStyleSheet("background-color: gainsboro; color: dimgray;");
            });
            msgBox.exec();
        }
    });
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PasteLinkDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &PasteLinkDialog::reject);
}

PasteLinkDialog::~PasteLinkDialog()
{
}

void PasteLinkDialog::accept()
{
    if ( m_dataInfoIsSet )
    {
        QTimer::singleShot( 1, m_mainwin, [mainwin=m_mainwin, dataInfo=m_dataInfo]
        {
            mainwin->startEasyDownload( dataInfo );
        });
        QDialog::accept();
    }
}


void PasteLinkDialog::reject()
{
    QDialog::reject();
}

void PasteLinkDialog::setLayout()
{
    m_layout->addWidget(path, 2, 0, Qt::AlignRight);
    m_layout->addWidget(m_pathLabel, 2, 1);

    m_layout->addWidget(dataSize, 3, 0, Qt::AlignRight);
    m_layout->addWidget(m_dataSizeLabel, 3, 1);

    m_layout->addWidget(buttonBox, 4, 1, Qt::AlignRight);
}

void PasteLinkDialog::setFontAndSize()
{
    QFont font;
    font.setPointSize(13);

    m_linkEdit->setFont(font);
    path->setFont(font);
    dataSize->setFont(font);
    folderName->setFont(font);
    m_pathLabel->setFont(font);
    m_dataSizeLabel->setFont(font);
    m_folderNameLabel->setFont(font);
    buttonBox->setFont(font);

    QFontMetrics fontMetrics(font);
    int minHeight = fontMetrics.height() + fontMetrics.descent() + 2;

    path->setMinimumHeight(minHeight);
    dataSize->setMinimumHeight(minHeight);
    folderName->setMinimumHeight(minHeight);
    m_pathLabel->setMinimumHeight(minHeight);
    m_dataSizeLabel->setMinimumHeight(minHeight);
    m_folderNameLabel->setMinimumHeight(minHeight);
    buttonBox->setMinimumHeight(minHeight);
}
