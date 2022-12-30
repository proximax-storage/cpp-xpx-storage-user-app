#include "ChannelInfoDialog.h"
#include "ui_ChannelInfoDialog.h"
#include "Model.h"
#include "DownloadChannel.h"
#include <QToolTip>
#include <QRegularExpression>

ChannelInfoDialog::ChannelInfoDialog( const DownloadChannel& channel,
                                                   QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChannelInfoDialog)
{
    ui->setupUi(this);

    ui->name->setText( QString::fromStdString(channel.getName()) );
    ui->driveKey->setText( QString::fromStdString(channel.getDriveKey()) );

    std::string allowedPublicKeys;
    for( const auto& key : channel.getAllowedPublicKeys() )
    {
        allowedPublicKeys += "."+key;
    }
    ui->keysLine->setText( QString::fromStdString(allowedPublicKeys) );

    ui->prepaidAmountLine->setText( "?" ); //QString::number(channel.m_prepaid) );

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ChannelInfoDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ChannelInfoDialog::reject);

    setWindowTitle("Channel Info");
    setFocus();
}

ChannelInfoDialog::~ChannelInfoDialog()
{
    delete ui;
}

void ChannelInfoDialog::accept() {
    QDialog::accept();
}

void ChannelInfoDialog::reject() {
    QDialog::reject();
}
