#include "AddDownloadChannelDialog.h"
#include "SelectEntityDialog.h"
#include "ui_AddDownloadChannelDialog.h"
#include "Utils.h"
#include "mainwin.h"
#include <QToolTip>
#include <QRegularExpression>

AddDownloadChannelDialog::AddDownloadChannelDialog(OnChainClient* onChainClient,
                                                   QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDownloadChannelDialog),
    mpOnChainClient(onChainClient)
{
    ui->setupUi(this);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Confirm");

    QRegularExpression nameTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9_]{1,40})")));
    connect(ui->name, &QLineEdit::textChanged, this, [this, nameTemplate] (auto text)
    {
        if (!nameTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->name->mapToGlobal(QPoint()), tr("Invalid name!"));
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->name->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->name->setProperty("is_valid", true);
            validate();
        }
    });

    if (!nameTemplate.match(ui->name->text()).hasMatch()) {
        QToolTip::showText(ui->name->mapToGlobal(QPoint()), tr("Invalid name!"));
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
            QToolTip::showText(ui->driveKey->mapToGlobal(QPoint()), tr("Invalid key!"));
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->driveKey->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->driveKey->setProperty("is_valid", true);
            validate();
        }
    });

    if (!keyTemplate.match(ui->driveKey->text()).hasMatch()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->driveKey->setProperty("is_valid", false);
        QToolTip::showText(ui->driveKey->mapToGlobal(QPoint()), tr("Invalid key!"));
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->driveKey->setProperty("is_valid", true);
        validate();
    }

//    connect(ui->keysLine, &QLineEdit::textChanged, this, [this] (auto text)
//    {
//        if (!text.trimmed().isEmpty()) {
//            QToolTip::showText(ui->keysLine->mapToGlobal(QPoint()), tr("Invalid keys!"));
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
//        QToolTip::showText(ui->driveKey->mapToGlobal(QPoint()), tr("Invalid keys!"));
//    } else {
//        QToolTip::hideText();
//        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
//        ui->driveKey->setProperty("is_valid", true);
//        validate();
//    }

    QRegularExpression unitsTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([0-9]{1,100})")));
    connect(ui->prepaidAmountLine, &QLineEdit::textChanged, this, [this, unitsTemplate] (auto text)
    {
        if (!unitsTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->prepaidAmountLine->mapToGlobal(QPoint()), tr("Invalid amount!"));
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->prepaidAmountLine->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->prepaidAmountLine->setProperty("is_valid", true);
            validate();
        }
    });

    if (!unitsTemplate.match(ui->prepaidAmountLine->text()).hasMatch()) {
        QToolTip::showText(ui->prepaidAmountLine->mapToGlobal(QPoint()), tr("Invalid units amount!"));
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

    connect(ui->m_selectMyDriveBtn, &QPushButton::released, this, [this] ()
    {
        SelectEntityDialog dialog(SelectEntityDialog::Entity::Drive, [this] (const QString& hash) { ui->driveKey->setText(hash); });
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

void AddDownloadChannelDialog::validate() {
    if (ui->name->property("is_valid").toBool() &&
        ui->driveKey->property("is_valid").toBool() &&
        ui->prepaidAmountLine->property("is_valid").toBool()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}