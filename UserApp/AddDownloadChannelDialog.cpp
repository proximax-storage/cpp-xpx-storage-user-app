#include "AddDownloadChannelDialog.h"
#include "SelectDriveDialog.h"
#include "ui_AddDownloadChannelDialog.h"
#include "Utils.h"
#include "Model.h"
#include "mainwin.h"

AddDownloadChannelDialog::AddDownloadChannelDialog(OnChainClient* onChainClient,
                                                   MainWin *parent) :
    QDialog(parent),
    ui(new Ui::AddDownloadChannelDialog),
    m_mainWin(parent),
    mpOnChainClient(onChainClient)
{
    ui->setupUi(this);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Send");

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddDownloadChannelDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AddDownloadChannelDialog::reject);

    connect(ui->m_selectMyDriveBtn, &QPushButton::released, this, [this] ()
    {
        SelectDriveDialog dialog( [this] (const QString& hash)
        {
            ui->driveKey->setText(hash);
        });
        dialog.exec();
    });

    setWindowTitle("Add new download channel");
    setFocus();
}

AddDownloadChannelDialog::~AddDownloadChannelDialog()
{
    delete ui;
}

void AddDownloadChannelDialog::accept() {
    std::vector<std::array<uint8_t, 32>> listOfAllowedPublicKeys;

    QStringList keys = ui->keysLine->text().trimmed().split(",", Qt::SkipEmptyParts);
    for (const auto& key : keys) {
        listOfAllowedPublicKeys.emplace_back(rawHashFromHex(key));
    }

    if (listOfAllowedPublicKeys.empty()) {
        qInfo() << LOG_SOURCE << "listOfAllowedPublicKeys is empty";
    }


    auto channelHash = mpOnChainClient->addDownloadChannel(
                ui->name->text().toStdString(),
                listOfAllowedPublicKeys,
                rawHashFromHex(ui->driveKey->text()),
                ui->prepaidAmountLine->text().toULongLong(),
                0); // feedback is unused for now

    std::vector<std::string> publicKeys;
    keys.reserve((int)listOfAllowedPublicKeys.size());
    for (const auto& key : listOfAllowedPublicKeys) {
        publicKeys.push_back(rawHashToHex(key).toStdString());
    }

    emit addDownloadChannel(ui->name->text().toStdString(), channelHash, ui->driveKey->text().toStdString(), publicKeys);

    QDialog::accept();
}

void AddDownloadChannelDialog::reject() {
    QDialog::reject();
}
