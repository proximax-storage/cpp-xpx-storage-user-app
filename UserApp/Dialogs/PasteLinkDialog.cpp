#include "PasteLinkDialog.h"
#include "mainwin.h"
#include "qapplication.h"
#include "Models/Model.h"
#include <QClipboard>
#include <QMessageBox>
#include <QPushButton>
// #include "ui_PasteLinkDialog.h"

PasteLinkDialog::PasteLinkDialog( MainWin* parent, Model* model )
    : QDialog( parent )
    // , ui( new Ui::PasteLinkDialog() )
    , m_mainwin(parent)
    , m_model(model)
{
    // ui->setupUi(this);
    setModal(true);
    m_layout = new QGridLayout(this);
    m_linkEdit = new QTextEdit;
    m_linkEdit->setFixedHeight(70);
    m_linkEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_linkEdit->setStyleSheet("background-color: gainsboro; color: dimgray;");
    m_linkEdit->setEnabled(false);
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

    m_layout->setSpacing(13);
    m_layout->addWidget(m_linkEdit, 0, 0, 1, 2);
    m_layout->addWidget(buttonBox, 4, 1, Qt::AlignRight);

    connect(m_pasteLinkButton, &QPushButton::clicked, this, [&](){
        QClipboard* clipboard = QApplication::clipboard();
        if (!clipboard) {
            qWarning() << LOG_SOURCE << "bad clipboard";
            return;
        }
        try
        {
            setLayout();
            m_linkEdit->setEnabled(true);
            m_linkEdit->setStyleSheet("");
            m_linkEdit->setText(clipboard->text());

            m_dataInfo.parseLink( clipboard->text().toStdString() );
            m_dataInfoIsSet = true;

            m_pathLabel->setText(QString::fromStdString(m_model->getDownloadFolder().string() + m_dataInfo.m_path));
            m_dataSizeLabel->setText(QString::fromStdString(std::to_string(m_dataInfo.m_totalSize)));
            if(!m_dataInfo.m_itemName.empty())
            {
                m_folderNameLabel->setText(QString::fromStdString(m_dataInfo.m_itemName));
                m_layout->addWidget(folderName, 1, 0, Qt::AlignRight);
                m_layout->addWidget(m_folderNameLabel, 1, 1);
            }
        }
        catch(...)
        {
            QMessageBox msgBox;
            msgBox.setText("Incorrect link");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();
        }
    });
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PasteLinkDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &PasteLinkDialog::reject);

    m_linkEdit->setPlainText("Link will be pasted here.");
}

PasteLinkDialog::~PasteLinkDialog()
{
    // delete ui;
}

void PasteLinkDialog::accept()
{
    if ( m_dataInfoIsSet )
    {
        QTimer::singleShot( 1, m_mainwin, [mainwin=m_mainwin, dataInfo=m_dataInfo]
        {
            mainwin->startEasyDownload( dataInfo );
        });
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
}

void PasteLinkDialog::setFontAndSize()
{

}
