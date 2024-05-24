#include "PasteLinkDialog.h"
#include "qclipboard.h"
#include "qmessagebox.h"
#include "qpushbutton.h"
#include "ui_PasteLinkDialog.h"

PasteLinkDialog::PasteLinkDialog(OnChainClient* onChainClient,
                                 Model* model,
                                 QWidget *parent)
    : QDialog( parent )
    , ui( new Ui::PasteLinkDialog() )
    , m_onChainClient(onChainClient)
    , m_model(model)
{
    ui->setupUi(this);
    setModal(true);
    m_pasteLinkButton = new QPushButton("Paste Link", this);
    ui->buttonBox->addButton(m_pasteLinkButton, QDialogButtonBox::ActionRole);
    connect(m_pasteLinkButton, &QPushButton::clicked, this, [&](){
        // if clipboard -> paste
        QClipboard* clipboard = QApplication::clipboard();
        if (!clipboard) {
            qWarning() << LOG_SOURCE << "bad clipboard";
            return;
        }
        if(validate(clipboard))
        {
            ui->m_linkEdit->setEnabled(true);
            ui->m_linkEdit->setStyleSheet("");
            ui->m_linkEdit->setText(clipboard->text());
        }
        else
        {
            QMessageBox msgBox;
            msgBox.setText("Incorrect link");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();
            reject();
        }
    });
    ui->m_linkEdit->setPlainText("Link will be pasted here.");
    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &PasteLinkDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &PasteLinkDialog::reject);
}

PasteLinkDialog::~PasteLinkDialog()
{
    delete ui;
}

void PasteLinkDialog::accept()
{
    // initiate download
}


void PasteLinkDialog::reject()
{
    QDialog::reject();
}

bool PasteLinkDialog:: validate(QClipboard* clipboard)
{
    // TODO

    QString text = clipboard->text();
    return !text.isEmpty();
    // return false;
}
