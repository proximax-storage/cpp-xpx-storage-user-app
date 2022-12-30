#include "Model.h"
#include "AddDriveDialog.h"
#include "./ui_AddDriveDialog.h"

#include <QIntValidator>
#include <QFileDialog>
#include <QRegularExpression>
#include <QToolTip>

AddDriveDialog::AddDriveDialog( OnChainClient* onChainClient,
                                Model* model,
                                QWidget *parent ) :
    QDialog( parent ),
    ui( new Ui::AddDriveDialog() ),
    mp_onChainClient(onChainClient),
    mp_model(model)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Confirm");
    setModal(true);

    QRegularExpression nameTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9_]{1,40})")));
    connect(ui->m_driveName, &QLineEdit::textChanged, this, [this, nameTemplate] (auto text)
    {
        bool isDriveExists = mp_model->isDriveWithNameExists(text);
        if (!nameTemplate.match(text).hasMatch() || isDriveExists) {
            QToolTip::showText(ui->m_driveName->mapToGlobal(QPoint(0, 15)), tr(isDriveExists ? "Drive with the same name is exists!" : "Invalid name!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_driveName->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_driveName->setProperty("is_valid", true);
            validate();
        }
    });

    QRegularExpression replicatorNumberTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([0-9]{1,10})")));
    connect(ui->m_replicatorNumber, &QLineEdit::textChanged, this, [this, replicatorNumberTemplate] (auto text)
    {
        if (!replicatorNumberTemplate.match(text).hasMatch() || text.toULongLong() == 0) {
            QToolTip::showText(ui->m_replicatorNumber->mapToGlobal(QPoint(0, 15)), tr("Invalid replicator number amount!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_replicatorNumber->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_replicatorNumber->setProperty("is_valid", true);
            validate();
        }
    });

    QRegularExpression driveSizeTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([0-9]{1,100})")));
    connect(ui->m_size, &QLineEdit::textChanged, this, [this, driveSizeTemplate] (auto text)
    {
        if (!driveSizeTemplate.match(text).hasMatch() || text.toULongLong() == 0) {
            QToolTip::showText(ui->m_size->mapToGlobal(QPoint(0, 15)), tr("Invalid drive size!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_size->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_size->setProperty("is_valid", true);
            validate();
        }
    });

    connect(ui->m_localDriveFolder, &QLineEdit::textChanged, this, [this](auto text){
        if (text.trimmed().isEmpty()) {
            QToolTip::showText(ui->m_localDriveFolder->mapToGlobal(QPoint(0, 15)), tr("Invalid path!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_localDriveFolder->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_localDriveFolder->setProperty("is_valid", true);
            validate();
        }
    });

    connect(ui->m_localFolderBtn, &QPushButton::released, this, [this]()
    {
        QFlags<QFileDialog::Option> options = QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks;
#ifdef Q_OS_LINUX
        options |= QFileDialog::DontUseNativeDialog;
#endif
        const QString path = QFileDialog::getExistingDirectory(this, tr("Choose directory"), "/", options);
        ui->m_localDriveFolder->setText(path.trimmed());
    });

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddDriveDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AddDriveDialog::reject);

    bool isDriveExists = mp_model->isDriveWithNameExists(ui->m_driveName->text());
    if (!nameTemplate.match(ui->m_driveName->text()).hasMatch()) {
        QToolTip::showText(ui->m_driveName->mapToGlobal(QPoint(0, 15)), tr(isDriveExists ? "Drive with the same name is exists!" : "Invalid name!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_driveName->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->m_driveName->setProperty("is_valid", true);
        validate();
    }

    if (!replicatorNumberTemplate.match(ui->m_replicatorNumber->text()).hasMatch() || ui->m_replicatorNumber->text().toULongLong() == 0) {
        QToolTip::showText(ui->m_replicatorNumber->mapToGlobal(QPoint(0, 15)), tr("Invalid replicator number amount!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_replicatorNumber->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->m_replicatorNumber->setProperty("is_valid", true);
        validate();
    }

    if (!driveSizeTemplate.match(ui->m_size->text()).hasMatch() || ui->m_size->text().toULongLong() == 0) {
        QToolTip::showText(ui->m_size->mapToGlobal(QPoint(0, 15)), tr("Invalid drive size!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_size->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->m_size->setProperty("is_valid", true);
        validate();
    }

    if (ui->m_localDriveFolder->text().trimmed().isEmpty()) {
        QToolTip::showText(ui->m_localDriveFolder->mapToGlobal(QPoint(0, 15)), tr("Invalid path!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_localDriveFolder->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->m_localDriveFolder->setProperty("is_valid", true);
        validate();
    }

    setWindowTitle("Create Drive");
    setFocus();
}

AddDriveDialog::~AddDriveDialog()
{
    delete ui;
}

void AddDriveDialog::validate() {
    if (ui->m_driveName->property("is_valid").toBool() &&
        ui->m_replicatorNumber->property("is_valid").toBool() &&
        ui->m_size->property("is_valid").toBool() &&
        ui->m_localDriveFolder->property("is_valid").toBool()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}

void AddDriveDialog::accept()
{
    auto hash = mp_onChainClient->addDrive( ui->m_size->text().toULongLong(), ui->m_replicatorNumber->text().toULongLong() );

    Drive drive;
    drive.setName(ui->m_driveName->text().toStdString());
    drive.setKey(hash);
    drive.setReplicatorsCount(ui->m_replicatorNumber->text().toInt());
    drive.setSize(ui->m_size->text().toInt());
    drive.setLocalFolder(ui->m_localDriveFolder->text().toStdString());

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    mp_model->addDrive(drive);
    mp_model->setCurrentDriveKey( drive.getKey() );
    mp_model->saveSettings();
    auto currentDrive = mp_model->currentDrive();
    if (currentDrive) {
        currentDrive->updateState(creating);
    }

    QDialog::accept();
}

void AddDriveDialog::reject()
{
    QDialog::reject();
}
