#include "ChannelInfoDialog.h"
#include "ui_ChannelInfoDialog.h"
#include "Model.h"
#include <QToolTip>
#include <QRegularExpression>

#include <boost/algorithm/string.hpp>

ChannelInfoDialog::ChannelInfoDialog( const ChannelInfo& channelInfo,
                                                   QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChannelInfoDialog)
{
    ui->setupUi(this);

    ui->name->setText( QString::fromStdString(channelInfo.m_name) );
    ui->driveKey->setText( QString::fromStdString(channelInfo.m_driveHash) );

    std::string allowedPublicKeys;
    for( const auto& key : channelInfo.m_allowedPublicKeys )
    {
        allowedPublicKeys += "."+key;
    }
    ui->keysLine->setText( QString::fromStdString(allowedPublicKeys) );

    ui->prepaidAmountLine->setText( "?" ); //QString::number(channelInfo.m_prepaid) );

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
