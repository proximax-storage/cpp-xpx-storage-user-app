#include "ReplicatorOffBoardingDialog.h"
#include "ui_ReplicatorOffBoardingDialog.h"

#include <QToolTip>
#include <QPushButton>
#include <QRegularExpression>

ReplicatorOffBoardingDialog::ReplicatorOffBoardingDialog(OnChainClient* onChainClient,
                                                         Model* model,
                                                         QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReplicatorOffBoardingDialog),
    mpOnChainClient(onChainClient),
    mpModel(model)
{
    ui->setupUi(this);
    setWindowTitle("Replicator offboarding");

    ui->drives->addItem("Select from my drives");
    for ( const auto& [key, drive] : mpModel->getDrives()) {
        ui->drives->addItem(drive.getName().c_str());
    }

    connect( ui->drives, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        if (index < 0) {
            return;
        }

        if (index == 0) {
            ui->drive->setText("");
            return;
        }

        auto drive = mpModel->findDriveByNameOrPublicKey(ui->drives->currentText().toStdString());
        if (drive) {
            ui->drive->setText(drive->getKey().c_str());
        } else {
            qWarning() << "Cached drive not found. Drive id: " << ui->drives->currentText().toStdString();
        }
    });

    ui->cachedReplicators->addItem("Select from my replicators");
    for (const auto& replicator : mpModel->getMyReplicators()) {
        ui->cachedReplicators->addItem(replicator.second.getName().c_str());
    }

    connect( ui->cachedReplicators, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        if (index < 0) {
            return;
        }

        if (index == 0) {
            ui->replicatorKey->setText("");
            return;
        }

        auto replicator = mpModel->findReplicatorByNameOrPublicKey(ui->cachedReplicators->currentText().toStdString());
        if (replicator.getPrivateKey().empty()) {
            qWarning() << "Cached replicator not found. Replicator id: " << ui->cachedReplicators->currentText().toStdString();
        } else {
            ui->replicatorKey->setText(replicator.getPrivateKey().c_str());
        }
    });

    QRegularExpression keyTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9]{64})")));
    connect(ui->drive, &QLineEdit::textChanged, this, [this, keyTemplate] (auto text)
    {
        if (!keyTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->drive->mapToGlobal(QPoint(0, 15)), tr("Invalid drive key!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->drive->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->drive->setProperty("is_valid", true);
            validate();
        }
    });

    if (!keyTemplate.match(ui->drive->text()).hasMatch()) {
        QToolTip::showText(ui->drive->mapToGlobal(QPoint(0, 15)), tr("Invalid drive key!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->drive->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->drive->setProperty("is_valid", true);
        validate();
    }

    connect(ui->replicatorKey, &QLineEdit::textChanged, this, [this, keyTemplate] (auto text)
    {
        if (!keyTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->replicatorKey->mapToGlobal(QPoint(0, 15)), tr("Invalid replicator key!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->replicatorKey->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->replicatorKey->setProperty("is_valid", true);
            validate();
        }
    });

    if (!keyTemplate.match(ui->replicatorKey->text()).hasMatch()) {
        QToolTip::showText(ui->replicatorKey->mapToGlobal(QPoint(0, 15)), tr("Invalid replicator key!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->replicatorKey->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->replicatorKey->setProperty("is_valid", true);
        validate();
    }

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ReplicatorOffBoardingDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ReplicatorOffBoardingDialog::reject);

    setFocus();
    ui->drive->setFocus();

    setTabOrder(ui->drive, ui->drives);
    setTabOrder(ui->drives, ui->replicatorKey);
    setTabOrder(ui->replicatorKey, ui->cachedReplicators);
    setTabOrder(ui->cachedReplicators, ui->buttonBox);
}

ReplicatorOffBoardingDialog::~ReplicatorOffBoardingDialog()
{
    delete ui;
}

void ReplicatorOffBoardingDialog::accept() {
    const auto driveId = rawHashFromHex(ui->drive->text());
    mpOnChainClient->replicatorOffBoarding(driveId, ui->replicatorKey->text());
    QDialog::accept();
}

void ReplicatorOffBoardingDialog::reject() {
    QDialog::reject();
}

void ReplicatorOffBoardingDialog::validate() {
    if (ui->replicatorKey->property("is_valid").toBool() &&
        ui->drive->property("is_valid").toBool()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}