#include "DownloadPaymentDialog.h"
#include "Utils.h"
#include "ui_DownloadPaymentDialog.h"
#include <QRegularExpression>
#include <QToolTip>

#include <boost/algorithm/string.hpp>

DownloadPaymentDialog::DownloadPaymentDialog(OnChainClient* onChainClient,
                                             Model* model,
                                             QWidget *parent) :
        QDialog(parent),
        ui(new Ui::DownloadPaymentDialog),
        mpOnChainClient(onChainClient),
        mpModel(model)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Confirm");

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DownloadPaymentDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &DownloadPaymentDialog::reject);

    std::vector<QString> channelsKeys;
    channelsKeys.reserve(mpModel->getDownloadChannels().size());
    for ( const auto& [key, channel] : mpModel->getDownloadChannels()) {
        channelsKeys.push_back(key);
        ui->selectChannelBox->addItem(channel.getName());
    }

    ui->selectChannelBox->model()->sort(0);
    ui->selectChannelBox->insertItem(0, "Select from my channels");

    connect( ui->selectChannelBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, channelsKeys] (int index)
    {
        if (index == 0) {
            mCurrentChannelKey.clear();
            ui->m_channelKey->setText("");
        } else if (index >= 1) {
            mCurrentChannelKey = channelsKeys[--index];
            ui->m_channelKey->setText(mCurrentChannelKey);
        }
    }, Qt::QueuedConnection);

    QRegularExpression keyTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9]{64})")));
    connect(ui->m_channelKey, &QLineEdit::textChanged, this, [this, keyTemplate, channelsKeys] (auto text)
    {
        if (!keyTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_channelKey->mapToGlobal(QPoint(0, 15)), tr("Invalid channel!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_channelKey->setProperty("is_valid", false);
            mCurrentChannelKey.clear();
            ui->selectChannelBox->setCurrentIndex(0);
        } else {
            QToolTip::hideText();
            ui->m_channelKey->setProperty("is_valid", true);
            mCurrentChannelKey = ui->m_channelKey->text();
            validate();

            auto index = ui->selectChannelBox->currentIndex();
            if (index >= 1) {
                bool isEquals = channelsKeys[--index].compare(mCurrentChannelKey, Qt::CaseInsensitive) == 0;
                if (!isEquals) {
                    ui->selectChannelBox->setCurrentIndex(0);
                }
            }
        }
    });

    QRegularExpression unitsTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([0-9]{1,100})")));
    connect(ui->m_prepaid, &QLineEdit::textChanged, this, [this, unitsTemplate] (auto text)
    {
        if (!unitsTemplate.match(text).hasMatch() || text.toULongLong() == 0) {
            QToolTip::showText(ui->m_prepaid->mapToGlobal(QPoint(0, 15)), tr("Invalid units amount!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_prepaid->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_prepaid->setProperty("is_valid", true);
            validate();
        }
    });

    if (!keyTemplate.match(ui->m_channelKey->text()).hasMatch()) {
        QToolTip::showText(ui->m_channelKey->mapToGlobal(QPoint(0, 15)), tr("Invalid channel!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_channelKey->setProperty("is_valid", false);
        mCurrentChannelKey.clear();
        ui->selectChannelBox->setCurrentIndex(0);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_channelKey->setProperty("is_valid", true);
        mCurrentChannelKey = ui->m_channelKey->text();
        validate();

        auto index = ui->selectChannelBox->currentIndex();
        if (index >= 1) {
            bool isEquals = channelsKeys[--index].compare(mCurrentChannelKey, Qt::CaseInsensitive) == 0;
            if (!isEquals) {
                ui->selectChannelBox->setCurrentIndex(0);
            }
        }
    }

    if (!unitsTemplate.match(ui->m_prepaid->text()).hasMatch() || ui->m_prepaid->text().toULongLong() == 0) {
        QToolTip::showText(ui->m_prepaid->mapToGlobal(QPoint(0, 15)), tr("Invalid units amount!"), nullptr, {}, 3000);
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
    ui->m_channelKey->setFocus();

    setTabOrder(ui->m_channelKey, ui->selectChannelBox);
    setTabOrder(ui->selectChannelBox, ui->m_prepaid);
    setTabOrder(ui->m_prepaid, ui->buttonBox);
}

DownloadPaymentDialog::~DownloadPaymentDialog()
{
    delete ui;
}

void DownloadPaymentDialog::accept() {
    mpOnChainClient->downloadPayment(rawHashFromHex(mCurrentChannelKey), ui->m_prepaid->text().toULongLong());
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