#include "AddDownloadChannelDialog.h"
#include "ui_adddownloadchanneldialog.h"
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

    setWindowTitle("Add new download channel");
    setFocus();
}

AddDownloadChannelDialog::~AddDownloadChannelDialog()
{
    delete ui;
}

void AddDownloadChannelDialog::accept() {
    std::vector<std::array<uint8_t, 32>> listOfAllowedPublicKeys;

#if __MINGW32__ || __APPLE__
    QStringList keys = ui->keysLine->text().trimmed().split(",", Qt::SkipEmptyParts);
#else
    QStringList keys = ui->keysLine->text().trimmed().split(",", QString::SkipEmptyParts);
#endif
    for (const auto& key : keys) {
        listOfAllowedPublicKeys.emplace_back(rawHashFromHex(key));
    }

    if (listOfAllowedPublicKeys.empty()) {
        qInfo() << "listOfAllowedPublicKeys is empty";
    }

    //auto channelHash =
    mpOnChainClient->addDownloadChannel(
                ui->name->text().toStdString(),
                listOfAllowedPublicKeys,
                rawHashFromHex(ui->driveKey->text()),
                ui->prepaidAmountLine->text().toULongLong(),
                0); // feedback is unused for now

//    m_mainWin->addChannel( ui->name->text().toStdString(),
//                           ui->driveKey->text().toStdString(),
//                           sirius::drive::toString(channelHash),
//                           {} );

    QDialog::accept();
}

void AddDownloadChannelDialog::reject() {
    QDialog::reject();
}
