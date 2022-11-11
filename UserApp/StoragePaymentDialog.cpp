#include "StoragePaymentDialog.h"
#include "ui_StoragePaymentDialog.h"
#include "SelectEntityDialog.h"
#include <QRegularExpression>
#include <QToolTip>

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

    connect(ui->m_selectDrive, &QPushButton::released, this, [this] ()
    {
        SelectEntityDialog dialog(SelectEntityDialog::Entity::Drive, [this] (const QString& hash) { ui->m_driveKy->setText(hash); });
        dialog.exec();
    });

    QRegularExpression keyTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9]{64})")));
    connect(ui->m_driveKy, &QLineEdit::textChanged, this, [this, keyTemplate] (auto text)
    {
        if (!keyTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_driveKy->mapToGlobal(QPoint()), tr("Invalid key!"));
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_driveKy->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_driveKy->setProperty("is_valid", true);
            validate();
        }
    });

    QRegularExpression unitsTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([0-9]{1,100})")));
    connect(ui->m_unitsAmount, &QLineEdit::textChanged, this, [this, unitsTemplate] (auto text)
    {
        if (!unitsTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_unitsAmount->mapToGlobal(QPoint()), tr("Invalid units amount!"));
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_unitsAmount->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_unitsAmount->setProperty("is_valid", true);
            validate();
        }
    });

    if (!keyTemplate.match(ui->m_driveKy->text()).hasMatch()) {
        QToolTip::showText(ui->m_driveKy->mapToGlobal(QPoint()), tr("Invalid key!"));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_driveKy->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_driveKy->setProperty("is_valid", true);
        validate();
    }

    if (!unitsTemplate.match(ui->m_unitsAmount->text()).hasMatch()) {
        QToolTip::showText(ui->m_unitsAmount->mapToGlobal(QPoint()), tr("Invalid units amount!"));
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
    mpOnChainClient->storagePayment(rawHashFromHex(ui->m_driveKy->text()), ui->m_unitsAmount->text().toULongLong());
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
