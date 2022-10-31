#include "AddDownloadChannelDialog.h"
#include "ui_adddownloadchanneldialog.h"

AddDownloadChannelDialog::AddDownloadChannelDialog(OnChainClient* onChainClient,
                                                   const QStringList& drives,
                                                   QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDownloadChannelDialog),
    mpOnChainClient(onChainClient)
{
    ui->setupUi(this);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Send");

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);
    ui->progressBar->setValue(0);
    ui->progressBar->hide();

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
    ui->progressBar->show();

    connect(mpOnChainClient, &OnChainClient::downloadChannelOpenTransactionFailed, this, [this](auto channelKey, auto errorText) {
        qWarning() << "downloadChannelOpenTransactionFailed: " << errorText;

        QMessageBox::warning(nullptr, "Alert", "channel key " + channelKey + " " + errorText);
        reject();
    }, Qt::QueuedConnection);

    connect(mpOnChainClient, &OnChainClient::downloadChannelOpenTransactionConfirmed, this, [this](auto, auto) {
        ui->progressBar->hide();
        ui->alias->setDisabled(false);
        ui->drive->setDisabled(false);
        ui->myDrives->setDisabled(false);
        ui->keysLine->setDisabled(false);
        ui->prepaidAmountLine->setDisabled(false);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Confirmed");
        ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("Close");
        ui->buttonBox->button(QDialogButtonBox::Cancel)->setDisabled(false);
        QDialog::accept();
    }, Qt::QueuedConnection);

    std::vector<std::array<uint8_t, 32>> listOfAllowedPublicKeys;

#if __MINGW32__ || __APPLE__
    QStringList keys = ui->keysLine->text().trimmed().split(",", Qt::SkipEmptyParts);
#else
    QStringList keys = ui->keysLine->text().trimmed().split(",", QString::SkipEmptyParts);
#endif
    for (const auto& key : keys) {
        listOfAllowedPublicKeys.emplace_back(TransactionsEngine::rawHashFromHex(key));
    }

    if (listOfAllowedPublicKeys.empty()) {
        qInfo() << "listOfAllowedPublicKeys is empty";
    }

    mpOnChainClient->addDownloadChannel(
                listOfAllowedPublicKeys,
                TransactionsEngine::rawHashFromHex(ui->drive->text()),
                ui->prepaidAmountLine->text().toULongLong(),
                0); // feedback is unused for now


    ui->alias->setDisabled(true);
    ui->drive->setDisabled(true);
    ui->myDrives->setDisabled(true);
    ui->keysLine->setDisabled(true);
    ui->prepaidAmountLine->setDisabled(true);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setDisabled(true);
}

void AddDownloadChannelDialog::reject() {
    QDialog::reject();
}
