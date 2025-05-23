#include "ReplicatorOnBoardingDialog.h"
#include "ui_ReplicatorOnBoardingDialog.h"

#include <QToolTip>
#include <QPushButton>
#include <QRegularExpression>

ReplicatorOnBoardingDialog::ReplicatorOnBoardingDialog(OnChainClient* onChainClient,
                                                       Model* model,
                                                       QWidget *parent)
      : QDialog(parent)
      , ui(new Ui::ReplicatorOnBoardingDialog)
      , mpModel(model)
      , mpOnChainClient(onChainClient)
{
    ui->setupUi(this);
    setWindowTitle("My replicator onboarding");

    QRegularExpression nameTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9_]{1,40})")));
    connect(ui->alias, &QLineEdit::textChanged, this, [this, nameTemplate] (auto text)
    {
        bool isReplicatorExists = mpModel->isReplicatorWithNameExists(text);
        if (!nameTemplate.match(text).hasMatch() || isReplicatorExists) {
            QToolTip::showText(ui->alias->mapToGlobal(QPoint(0, 15)), tr(isReplicatorExists ? "Replicator with the same name is exists!" : "Invalid name!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->alias->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->alias->setProperty("is_valid", true);
            validate();
        }
    });

    bool isReplicatorAliasExists = mpModel->isReplicatorWithNameExists(ui->alias->text());
    if (!nameTemplate.match(ui->alias->text()).hasMatch() || isReplicatorAliasExists) {
        QToolTip::showText(ui->alias->mapToGlobal(QPoint(0, 15)), tr(isReplicatorAliasExists ? "Replicator with the same name is exists!" : "Invalid name!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->alias->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->alias->setProperty("is_valid", true);
        validate();
    }

    QRegularExpression keyTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([A-Fa-f0-9]{64})")));
    connect(ui->replicatorKey, &QLineEdit::textChanged, this, [this, keyTemplate] (auto text)
    {
        // TODO: Fix second check
        if (!keyTemplate.match(text).hasMatch() /*|| !mpModel->findReplicatorByPublicKey(getReplicatorPublicKey()).getPrivateKey().isEmpty()*/) {
            QToolTip::showText(ui->replicatorKey->mapToGlobal(QPoint(0, 15)), tr("Invalid or duplicate key!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->replicatorKey->setProperty("is_valid", false);
            ui->checkBox->setCheckState(Qt::Unchecked);
        } else {
            QToolTip::hideText();
            ui->replicatorKey->setText(ui->replicatorKey->text().toUpper());
            ui->replicatorKey->setProperty("is_valid", true);
            validate();

            mpOnChainClient->getBlockchainEngine()->getReplicatorById(getReplicatorPublicKey().toStdString(), [this] (auto, auto isSuccess, auto, auto ) {
                if (isSuccess) {
                    ui->checkBox->setCheckState(Qt::Checked);
                }
            });
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

    QRegularExpression capacityTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([0-9]{1,100})")));
    connect(ui->capacity, &QLineEdit::textChanged, this, [this, capacityTemplate] (auto text)
    {
        if (!capacityTemplate.match(text).hasMatch() || text.toULongLong() == 0) {
            QToolTip::showText(ui->capacity->mapToGlobal(QPoint(0, 15)), tr("Invalid capacity!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->capacity->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->capacity->setProperty("is_valid", true);
            validate();
        }
    });

    if (!capacityTemplate.match(ui->capacity->text()).hasMatch() || ui->capacity->text().toULongLong() == 0) {
        QToolTip::showText(ui->capacity->mapToGlobal(QPoint(0, 15)), tr("Invalid capacity!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->capacity->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->capacity->setProperty("is_valid", true);
        validate();
    }

    connect(ui->checkBox, &QCheckBox::stateChanged, this, [this](auto){
        validate();
    });

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ReplicatorOnBoardingDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ReplicatorOnBoardingDialog::reject);

    setFocus();
    ui->alias->setFocus();

    setTabOrder(ui->alias, ui->replicatorKey);
    setTabOrder(ui->replicatorKey, ui->capacity);
    setTabOrder(ui->capacity, ui->checkBox);
    setTabOrder(ui->checkBox, ui->buttonBox);
}

ReplicatorOnBoardingDialog::~ReplicatorOnBoardingDialog()
{
    delete ui;
}

bool ReplicatorOnBoardingDialog::isReplicatorExists() const {
    return ui->checkBox->isChecked();
}

QString ReplicatorOnBoardingDialog::getReplicatorPublicKey() const {
    auto keyPair = sirius::crypto::KeyPair::FromPrivate(sirius::crypto::PrivateKey::FromString(ui->replicatorKey->text().toStdString()));
    return rawHashToHex(keyPair.publicKey().array());
}

void ReplicatorOnBoardingDialog::accept() {
    CachedReplicator r;
    r.setName(ui->alias->text());
    r.setPublicKey(getReplicatorPublicKey());
    r.setPrivateKey(ui->replicatorKey->text());
    mpModel->addMyReplicator(r);
    mpModel->saveSettings();

    mpOnChainClient->replicatorOnBoarding(ui->replicatorKey->text(), ui->nodeBootKey->text(), ui->capacity->text().toLongLong(), isReplicatorExists());
    QDialog::accept();
}

void ReplicatorOnBoardingDialog::reject() {
    QDialog::reject();
}

void ReplicatorOnBoardingDialog::validate() {
    if (isReplicatorExists()) {
        ui->capacity->hide();
        ui->label_2->hide();
    } else {
        ui->capacity->show();
        ui->label_2->show();
    }

    if (ui->alias->property("is_valid").toBool() &&
        ui->replicatorKey->property("is_valid").toBool() &&
        (isReplicatorExists() || ui->capacity->property("is_valid").toBool())) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}
