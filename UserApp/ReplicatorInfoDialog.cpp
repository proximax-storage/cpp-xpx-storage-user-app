#include "ReplicatorInfoDialog.h"
#include "ui_ReplicatorInfoDialog.h"
#include "mainwin.h"

#include <QToolTip>
#include <QPushButton>
#include <QRegularExpression>

ReplicatorInfoDialog::ReplicatorInfoDialog(const std::string& replicator,
                                           Model* model,
                                           QWidget* parent) :
    QDialog(parent),
    ui(new Ui::ReplicatorInfoDialog),
    mpModel(model)
{
    ui->setupUi(this);

    mPublicKey = replicator;
    auto cachedReplicator = mpModel->findReplicatorByPublicKey(replicator);
    ui->alias->setText(cachedReplicator.getName().c_str());
    ui->publicKey->setText(cachedReplicator.getPublicKey().c_str());
    ui->publicKey->setReadOnly(true);
    ui->drivesAmount->setText("0");
    ui->drivesAmount->setReadOnly(true);
    ui->channelsAmount->setText("0");
    ui->channelsAmount->setReadOnly(true);
    ui->freeSpace->setText("0");
    ui->freeSpace->setReadOnly(true);
    ui->totalSpace->setText("0");
    ui->totalSpace->setReadOnly(true);

    QRegularExpression nameTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9_]{1,40})")));
    connect(ui->alias, &QLineEdit::textChanged, this, [this, nameTemplate] (auto text)
    {
        if (!nameTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->alias->mapToGlobal(QPoint(0, 15)), tr("Invalid name!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->alias->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->alias->setProperty("is_valid", true);
            validate();
        }
    });

    if (!nameTemplate.match(ui->alias->text()).hasMatch()) {
        QToolTip::showText(ui->alias->mapToGlobal(QPoint(0, 15)), tr("Invalid name!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->alias->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->alias->setProperty("is_valid", true);
        validate();
    }

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ReplicatorInfoDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ReplicatorInfoDialog::reject);

    setWindowTitle("Replicator info");
    setFocus();
}

ReplicatorInfoDialog::~ReplicatorInfoDialog()
{
    delete ui;
}

void ReplicatorInfoDialog::accept() {
    auto replicator = mpModel->findReplicatorByPublicKey(mPublicKey);
    if (replicator.getName() != ui->alias->text().toStdString()) {
        mpModel->updateReplicatorAlias(replicator.getPublicKey(), ui->alias->text().toStdString());
        mpModel->saveSettings();
        emit ((MainWin*)parent())->refreshMyReplicatorsTable();
    }

    QDialog::accept();
}

void ReplicatorInfoDialog::reject() {
    QDialog::reject();
}

void ReplicatorInfoDialog::updateFreeSpace(uint64_t freeSpace) {
    ui->freeSpace->setText(QString::number(freeSpace));
}

void ReplicatorInfoDialog::updateUsedSpace(uint64_t usedSpace) {
    ui->totalSpace->setText(QString::number(usedSpace + ui->totalSpace->text().toULongLong() + ui->freeSpace->text().toULongLong()));
}

void ReplicatorInfoDialog::updateDrivesAmount(uint64_t amount) {
    ui->drivesAmount->setText(QString::number(amount));
}

void ReplicatorInfoDialog::updateChannelsAmount(uint64_t amount) {
    ui->channelsAmount->setText(QString::number(amount));
}

void ReplicatorInfoDialog::validate() {
    if (ui->alias->property("is_valid").toBool()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}