#include "DownloadPaymentDialog.h"
#include "ui_DownloadPaymentDialog.h"
#include "SelectEntityDialog.h"
#include <QRegularExpression>
#include <QToolTip>

DownloadPaymentDialog::DownloadPaymentDialog(OnChainClient* onChainClient,
                                             QWidget *parent) :
        QDialog(parent),
        ui(new Ui::DownloadPaymentDialog),
        mpOnChainClient(onChainClient)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Confirm");

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DownloadPaymentDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &DownloadPaymentDialog::reject);

    connect(ui->m_selectChannel, &QPushButton::released, this, [this] ()
    {
        SelectEntityDialog dialog(SelectEntityDialog::Entity::Channel, [this] (const QString& hash) { ui->m_channelKey->setText(hash); });
        dialog.exec();
    });

    QRegularExpression keyTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9]{64})")));
    connect(ui->m_channelKey, &QLineEdit::textChanged, this, [this, keyTemplate] (auto text)
    {
        if (!keyTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_channelKey->mapToGlobal(QPoint()), tr("Invalid channel!"));
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_channelKey->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_channelKey->setProperty("is_valid", true);
            validate();
        }
    });

    QRegularExpression unitsTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([0-9]{1,100})")));
    connect(ui->m_prepaid, &QLineEdit::textChanged, this, [this, unitsTemplate] (auto text)
    {
        if (!unitsTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_prepaid->mapToGlobal(QPoint()), tr("Invalid units amount!"));
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_prepaid->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_prepaid->setProperty("is_valid", true);
            validate();
        }
    });

    if (!keyTemplate.match(ui->m_channelKey->text()).hasMatch()) {
        QToolTip::showText(ui->m_channelKey->mapToGlobal(QPoint()), tr("Invalid channel!"));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_channelKey->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_channelKey->setProperty("is_valid", true);
        validate();
    }

    if (!unitsTemplate.match(ui->m_prepaid->text()).hasMatch()) {
        QToolTip::showText(ui->m_prepaid->mapToGlobal(QPoint()), tr("Invalid units amount!"));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_prepaid->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_prepaid->setProperty("is_valid", true);
        validate();
    }

    setWindowTitle("Download payment");
    setFocus();
}

DownloadPaymentDialog::~DownloadPaymentDialog()
{
    delete ui;
}

void DownloadPaymentDialog::accept() {
    mpOnChainClient->downloadPayment(rawHashFromHex(ui->m_channelKey->text()), ui->m_prepaid->text().toULongLong());
    QDialog::accept();
}

void DownloadPaymentDialog::reject() {
    QDialog::reject();
}

void DownloadPaymentDialog::validate() {
    if (ui->m_channelKey->property("is_valid").toBool() &&
        ui->m_prepaid->property("is_valid").toBool()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}