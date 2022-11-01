#include "AddDownloadChannelDialog.h"
#include "ui_adddownloadchanneldialog.h"
#include "Utils.h"
#include "Model.h"

AddDownloadChannelDialog::AddDownloadChannelDialog(OnChainClient* onChainClient,
                                                   const QStringList& drives,
                                                   QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDownloadChannelDialog),
    mpOnChainClient(onChainClient)
{
    ui->setupUi(this);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Send");
    ui->myDrives->addItems(drives);

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

    mpOnChainClient->addDownloadChannel(
                ui->alias->text().toStdString(),
                listOfAllowedPublicKeys,
                rawHashFromHex(ui->drive->text()),
                ui->prepaidAmountLine->text().toULongLong(),
                0); // feedback is unused for now

    QDialog::accept();
}

void AddDownloadChannelDialog::reject() {
    QDialog::reject();
}
