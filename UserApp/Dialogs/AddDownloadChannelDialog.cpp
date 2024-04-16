#include "AddDownloadChannelDialog.h"
#include "ui_AddDownloadChannelDialog.h"
#include "Utils.h"
#include "mainwin.h"
#include "Entities/Drive.h"
#include <QToolTip>
#include <QRegularExpression>

#include <boost/algorithm/string.hpp>

AddDownloadChannelDialog::AddDownloadChannelDialog(OnChainClient* onChainClient,
                                                   Model* model,
                                                   QWidget *parent,
                                                   std::string driveKey ) :
    QDialog(parent),
    ui(new Ui::AddDownloadChannelDialog),
    mpOnChainClient(onChainClient),
    mpModel(model),
    m_forStreaming( !driveKey.empty() )
{
    ui->setupUi(this);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Confirm");
    ui->buttonBox->button(QDialogButtonBox::Help)->setText("Help");

    QRegularExpression nameTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9_]{1,40})")));
    connect(ui->name, &QLineEdit::textChanged, this, [this, nameTemplate] (auto text)
    {
        bool isChannelExists = mpModel->isChannelWithNameExists(text);
        if (!nameTemplate.match(text).hasMatch() || isChannelExists) {
            QToolTip::showText(ui->name->mapToGlobal(QPoint(0, 15)), tr(isChannelExists ? "Channel with the same name is exists!" : "Invalid name!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->name->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->name->setProperty("is_valid", true);
            validate();
        }
    });


    bool isChannelExists = mpModel->isChannelWithNameExists(ui->name->text());
    if (!nameTemplate.match(ui->name->text()).hasMatch()) {
        QToolTip::showText(ui->name->mapToGlobal(QPoint(0, 15)), tr(isChannelExists ? "Channel with the same name is exists!" : "Invalid name!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->name->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->name->setProperty("is_valid", true);
        validate();
    }

    QRegularExpression keyTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9]{64})")));
    connect(ui->driveKey, &QLineEdit::textChanged, this, [this, keyTemplate] (auto text)
            {
                if (!keyTemplate.match(text).hasMatch()) {
                    QToolTip::showText(ui->driveKey->mapToGlobal(QPoint(0, 15)), tr("Invalid key!"), nullptr, {}, 3000);
                    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
                    ui->driveKey->setProperty("is_valid", false);
                    mCurrentDriveKey.clear();
                } else {
                    QToolTip::hideText();
                    ui->driveKey->setProperty("is_valid", true);
                    mCurrentDriveKey = ui->driveKey->text().toStdString();
                    validate();
                }
            });

//    if (!ui->keysLine->text().trimmed().isEmpty()) {
//        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
//        ui->driveKey->setProperty("is_valid", false);
//        QToolTip::showText(ui->driveKey->mapToGlobal(QPoint(0, 15)), tr("Invalid keys!"));
//    } else {
//        QToolTip::hideText();
//        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
//        ui->driveKey->setProperty("is_valid", true);
//        validate();
//    }

    QRegularExpression unitsTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([0-9]{1,100})")));
    connect(ui->prepaidAmountLine, &QLineEdit::textChanged, this, [this, unitsTemplate] (auto text)
    {
        if (!unitsTemplate.match(text).hasMatch() || text.toULongLong() == 0) {
            QToolTip::showText(ui->prepaidAmountLine->mapToGlobal(QPoint(0, 15)), tr("Invalid amount!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->prepaidAmountLine->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->prepaidAmountLine->setProperty("is_valid", true);
            validate();
        }
    });

    if (!unitsTemplate.match(ui->prepaidAmountLine->text()).hasMatch() || ui->prepaidAmountLine->text().toULongLong() == 0) {
        QToolTip::showText(ui->prepaidAmountLine->mapToGlobal(QPoint(0, 15)), tr("Invalid units amount!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->prepaidAmountLine->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->prepaidAmountLine->setProperty("is_valid", true);
        validate();
    }

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddDownloadChannelDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AddDownloadChannelDialog::reject);
    connect(ui->buttonBox, &QDialogButtonBox::helpRequested, this, &AddDownloadChannelDialog::displayInfo);

    
    ui->driveKey->setText( QString::fromStdString( driveKey ) );

    setWindowTitle("Add new download channel");
    setFocus();
    ui->name->setFocus();

    setTabOrder(ui->name, ui->driveKey);
    setTabOrder(ui->driveKey, ui->buttonBox);
}

AddDownloadChannelDialog::~AddDownloadChannelDialog()
{
    if(helpMessageBox) {
        helpMessageBox->hide();
        delete helpMessageBox;
        helpMessageBox = nullptr;
    }

    delete ui;
}

void AddDownloadChannelDialog::startCreateChannel()
{
    std::vector<std::array<uint8_t, 32>> listOfAllowedPublicKeys;

    if (listOfAllowedPublicKeys.empty()) {
        qInfo() << LOG_SOURCE << "listOfAllowedPublicKeys is empty";
    }

    std::vector<std::string> publicKeys;
    for (const auto& key : listOfAllowedPublicKeys) {
        publicKeys.push_back(rawHashToHex(key).toStdString());
    }

    const auto channelName = ui->name->text().toStdString();
    auto callback = [client = mpOnChainClient, channelName, currDriveKey = mCurrentDriveKey, publicKeys](std::string hash) {
        hash = QString::fromStdString(hash).toUpper().toStdString();
        qDebug() << "AddDownloadChannelDialog::accept::addChannelHash: " << hash.c_str();
        emit client->getDialogSignalsEmitter()->addDownloadChannel(channelName, hash, currDriveKey, publicKeys);
    };

    mpOnChainClient->addDownloadChannel(ui->name->text().toStdString(),
                                        listOfAllowedPublicKeys,rawHashFromHex(mCurrentDriveKey.c_str()),
                                        ui->prepaidAmountLine->text().toULongLong(),
                                        0,
                                        callback); // feedback is unused for now

}

void AddDownloadChannelDialog::accept() {
    startCreateChannel();
    QDialog::accept();
}

void AddDownloadChannelDialog::reject() {
    QDialog::reject();
}

void AddDownloadChannelDialog::validate() {
    if (ui->name->property("is_valid").toBool() &&
        ui->driveKey->property("is_valid").toBool() &&
        ui->prepaidAmountLine->property("is_valid").toBool()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}


void AddDownloadChannelDialog::displayInfo()
{
    if(helpMessageBox) {
        helpMessageBox->hide();
        helpMessageBox = nullptr;
    }
    QString message = "<html>"
                      "A <b>channel</b> is created for downloading files from a remote drive.<br>"
                      "A channel is only active for at most <b>24 hours</b> and then closes automatically.<br><br>"
                      "<b>Channel name</b> is a preferred name to associate with the remote drive. "
                      "It can contain capital and small latin letters as well as digits.<br><br>"
                      "<b>Drive key</b> can be received from the remote drive owner. "
                      "It contains 64 characters (letters and digits).<br><br>"
                      "<b>Prepaid MBs</b> is an approximate amount of data you are going to download. "
                      "Unused xpx is refunded to you when the channel is closed."
                      "</html>";
    helpMessageBox = new QMessageBox(this);
    helpMessageBox->setWindowTitle("Help");
    helpMessageBox->setText(message);
    helpMessageBox->setWindowModality(Qt::NonModal); // Set the NonModal flag
    helpMessageBox->show();
    helpMessageBox->move(this->x() + this->width()/2-this->helpMessageBox->width()/2, this->y() + this->height() + 60);
}

