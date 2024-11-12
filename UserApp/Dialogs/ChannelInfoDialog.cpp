#include "ChannelInfoDialog.h"
#include "ui_ChannelInfoDialog.h"
#include "Models/Model.h"
#include "Entities/DownloadChannel.h"
#include <QToolTip>
#include <QRegularExpression>

ChannelInfoDialog::ChannelInfoDialog( QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChannelInfoDialog)
{
    ui->setupUi(this);
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

void ChannelInfoDialog::init()
{
    ui->name->setText(name);
    ui->driveKey->setText(driveId);
    ui->id->setText(id);

    QString allowedPublicKeys;
    for( int i = 0; i < keys.size(); i++ )
    {
        if (i != 0 && i != keys.size() - 1)
        {
            allowedPublicKeys += ", " + keys[i];
        }
        else
        {
            allowedPublicKeys += keys[i];
        }
    }

    QString replicatorKeys;
    for( int i = 0; i < replicators.size(); i++ )
    {
        if (i != 0 && i != replicators.size() - 1)
        {
            replicatorKeys += ", " + replicators[i];
        }
        else
        {
            replicatorKeys += replicators[i];
        }
    }

    ui->replicators->setText(replicatorKeys);

    ui->keysLine->setText(allowedPublicKeys);
    ui->prepaidAmountLine->setText(QString::number(prepaid));

    ui->name->setReadOnly(true);
    ui->id->setReadOnly(true);
    ui->replicators->setReadOnly(true);
    ui->driveKey->setReadOnly(true);
    ui->keysLine->setReadOnly(true);
    ui->prepaidAmountLine->setReadOnly(true);
}

void ChannelInfoDialog::setName(const QString& channelName)
{
    name = channelName;
}

void ChannelInfoDialog::setId(const QString& channelId)
{
    id = channelId;
}

void ChannelInfoDialog::setDriveId(const QString& drive)
{
    driveId = drive;
}

void ChannelInfoDialog::setReplicators(const std::vector<QString>& replicatorKeys)
{
    replicators = replicatorKeys;
}

void ChannelInfoDialog::setKeys(const std::vector<QString>& pKeys)
{
    keys = pKeys;
}

void ChannelInfoDialog::setPrepaid(const uint64_t amount)
{
    prepaid = amount;
}
