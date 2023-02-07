#include "AddDownloadChannelDialog.h"
#include "ui_AddDownloadChannelDialog.h"
#include "Utils.h"
#include "mainwin.h"
#include "Drive.h"
#include <QToolTip>
#include <QRegularExpression>

#include <boost/algorithm/string.hpp>

AddDownloadChannelDialog::AddDownloadChannelDialog(OnChainClient* onChainClient,
                                                   Model* model,
                                                   QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDownloadChannelDialog),
    mpOnChainClient(onChainClient),
    mpModel(model)
{
    ui->setupUi(this);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Confirm");

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

    std::vector<std::string> drivesKeys;
    drivesKeys.reserve(mpModel->getDrives().size());
    for ( const auto& [key, drive] : mpModel->getDrives()) {
        drivesKeys.push_back(key);
        ui->selectDriveBox->addItem(drive.getName().c_str());
    }

    ui->selectDriveBox->model()->sort(0);
    ui->selectDriveBox->insertItem(0, "Select from my drives");

    QRegularExpression keyTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9]{64})")));
    connect(ui->driveKey, &QLineEdit::textChanged, this, [this, keyTemplate, drivesKeys] (auto text)
    {
        if (!keyTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->driveKey->mapToGlobal(QPoint(0, 15)), tr("Invalid key!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->driveKey->setProperty("is_valid", false);
            mCurrentDriveKey.clear();
            ui->selectDriveBox->setCurrentIndex(0);
        } else {
            QToolTip::hideText();
            ui->driveKey->setProperty("is_valid", true);
            mCurrentDriveKey = ui->driveKey->text().toStdString();
            validate();

            auto index = ui->selectDriveBox->currentIndex();
            if (index >= 1) {
                bool isEquals = boost::iequals( drivesKeys[--index], mCurrentDriveKey );
                if (!isEquals) {
                    ui->selectDriveBox->setCurrentIndex(0);
                }
            }
        }
    });

    if (!keyTemplate.match(ui->driveKey->text()).hasMatch()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->driveKey->setProperty("is_valid", false);
        mCurrentDriveKey.clear();
        ui->selectDriveBox->setCurrentIndex(0);
        QToolTip::showText(ui->driveKey->mapToGlobal(QPoint(0, 15)), tr("Invalid key!"), nullptr, {}, 3000);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->driveKey->setProperty("is_valid", true);
        mCurrentDriveKey = ui->driveKey->text().toStdString();
        validate();

        auto index = ui->selectDriveBox->currentIndex();
        if (index >= 1) {
            bool isEquals = boost::iequals( drivesKeys[--index], mCurrentDriveKey );
            if (!isEquals) {
                ui->selectDriveBox->setCurrentIndex(0);
            }
        }
    }

//    connect(ui->keysLine, &QLineEdit::textChanged, this, [this] (auto text)
//    {
//        if (!text.trimmed().isEmpty()) {
//            QToolTip::showText(ui->keysLine->mapToGlobal(QPoint(0, 15)), tr("Invalid keys!"));
//            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
//            ui->keysLine->setProperty("is_valid", false);
//        } else {
//            QToolTip::hideText();
//            ui->keysLine->setProperty("is_valid", true);
//            validate();
//        }
//    });
//
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

    connect( ui->selectDriveBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, drivesKeys] (int index)
    {
        if (index == 0) {
            mCurrentDriveKey.clear();
            ui->driveKey->setText("");
        } else if (index >= 1) {
            mCurrentDriveKey = drivesKeys[--index];
            ui->driveKey->setText(mCurrentDriveKey.c_str());
        }
    }, Qt::QueuedConnection);

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
                rawHashFromHex(mCurrentDriveKey.c_str()),
                ui->prepaidAmountLine->text().toULongLong(),
                0); // feedback is unused for now

    channelHash = QString::fromStdString(channelHash).toUpper().toStdString();

    qDebug() << LOG_SOURCE << "addChannelHash: " << channelHash.c_str();

    std::vector<std::string> publicKeys;
    keys.reserve((int)listOfAllowedPublicKeys.size());
    for (const auto& key : listOfAllowedPublicKeys) {
        publicKeys.push_back(rawHashToHex(key).toStdString());
    }

    emit addDownloadChannel(ui->name->text().toStdString(), channelHash, mCurrentDriveKey, publicKeys);

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
