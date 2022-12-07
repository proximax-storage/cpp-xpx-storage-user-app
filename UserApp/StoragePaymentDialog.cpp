#include "StoragePaymentDialog.h"
#include "ui_StoragePaymentDialog.h"
#include <QRegularExpression>
#include <QToolTip>

#include <boost/algorithm/string.hpp>

StoragePaymentDialog::StoragePaymentDialog(OnChainClient* onChainClient,
                                           QWidget *parent) :
        QDialog(parent),
        ui(new Ui::StoragePaymentDialog),
        mpOnChainClient(onChainClient)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Confirm");

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &StoragePaymentDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &StoragePaymentDialog::reject);

    ui->selectDriveBox->addItem("Select from my drives");

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    std::vector<std::string> drivesKeys;
    drivesKeys.reserve(Model::drives().size());
    for ( const auto& drive : Model::drives()) {
        drivesKeys.push_back(drive.m_driveKey);
        ui->selectDriveBox->addItem(drive.m_name.c_str());
    }

    lock.unlock();

    connect( ui->selectDriveBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, drivesKeys] (int index)
    {
        if (index >= 1) {
            mCurrentDriveKey = drivesKeys[--index];
            ui->m_driveKy->setText(mCurrentDriveKey.c_str());
        }
    }, Qt::QueuedConnection);

    QRegularExpression keyTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9]{64})")));
    connect(ui->m_driveKy, &QLineEdit::textChanged, this, [this, keyTemplate, drivesKeys] (auto text)
    {
        if (!keyTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_driveKy->mapToGlobal(QPoint(0, 15)), tr("Invalid key!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_driveKy->setProperty("is_valid", false);
            mCurrentDriveKey.clear();
            ui->selectDriveBox->setCurrentIndex(0);
        } else {
            QToolTip::hideText();
            ui->m_driveKy->setProperty("is_valid", true);
            mCurrentDriveKey = ui->m_driveKy->text().toStdString();
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

    QRegularExpression unitsTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([0-9]{1,100})")));
    connect(ui->m_unitsAmount, &QLineEdit::textChanged, this, [this, unitsTemplate] (auto text)
    {
        if (!unitsTemplate.match(text).hasMatch() || text.toULongLong() == 0) {
            QToolTip::showText(ui->m_unitsAmount->mapToGlobal(QPoint(0, 15)), tr("Invalid units amount!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_unitsAmount->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_unitsAmount->setProperty("is_valid", true);
            validate();
        }
    });

    if (!keyTemplate.match(ui->m_driveKy->text()).hasMatch()) {
        QToolTip::showText(ui->m_driveKy->mapToGlobal(QPoint(0, 15)), tr("Invalid key!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_driveKy->setProperty("is_valid", false);
        mCurrentDriveKey.clear();
        ui->selectDriveBox->setCurrentIndex(0);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_driveKy->setProperty("is_valid", true);
        mCurrentDriveKey = ui->m_driveKy->text().toStdString();
        validate();

        auto index = ui->selectDriveBox->currentIndex();
        if (index >= 1) {
            bool isEquals = boost::iequals( drivesKeys[--index], mCurrentDriveKey );
            if (!isEquals) {
                ui->selectDriveBox->setCurrentIndex(0);
            }
        }
    }

    if (!unitsTemplate.match(ui->m_unitsAmount->text()).hasMatch() || ui->m_unitsAmount->text().toULongLong() == 0) {
        QToolTip::showText(ui->m_unitsAmount->mapToGlobal(QPoint(0, 15)), tr("Invalid units amount!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_unitsAmount->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_unitsAmount->setProperty("is_valid", true);
        validate();
    }

    setWindowTitle("Storage payment");
    setFocus();
}

StoragePaymentDialog::~StoragePaymentDialog()
{
    delete ui;
}

void StoragePaymentDialog::accept() {
    mpOnChainClient->storagePayment(rawHashFromHex(mCurrentDriveKey.c_str()), ui->m_unitsAmount->text().toULongLong());
    QDialog::accept();
}

void StoragePaymentDialog::reject() {
    QDialog::reject();
}

void StoragePaymentDialog::validate() {
    if (ui->m_driveKy->property("is_valid").toBool() &&
        ui->m_unitsAmount->property("is_valid").toBool()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}
