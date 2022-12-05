#include "Settings.h"
#include "Model.h"
#include "AddDriveDialog.h"
#include "./ui_AddDriveDialog.h"

#include <QIntValidator>
#include <QFileDialog>
#include <QRegularExpression>
#include <QToolTip>

AddDriveDialog::AddDriveDialog( OnChainClient* onChainClient,
                                QWidget *parent ) :
    QDialog( parent ),
    ui( new Ui::AddDriveDialog() ),
    mp_onChainClient(onChainClient)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Confirm");
    setModal(true);

    QRegularExpression nameTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9_]{1,40})")));
    connect(ui->m_driveName, &QLineEdit::textChanged, this, [this, nameTemplate] (auto text)
    {
        if (!nameTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_driveName->mapToGlobal(QPoint(0, 15)), tr("Invalid name!"), nullptr, {}, 3000);
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
        if (!replicatorNumberTemplate.match(text).hasMatch()) {
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
    connect(ui->m_maxDriveSize, &QLineEdit::textChanged, this, [this, driveSizeTemplate] (auto text)
    {
        if (!driveSizeTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_maxDriveSize->mapToGlobal(QPoint(0, 15)), tr("Invalid drive size!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_maxDriveSize->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_maxDriveSize->setProperty("is_valid", true);
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

    if (!nameTemplate.match(ui->m_driveName->text()).hasMatch()) {
        QToolTip::showText(ui->m_driveName->mapToGlobal(QPoint(0, 15)), tr("Invalid name!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_driveName->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->m_driveName->setProperty("is_valid", true);
        validate();
    }

    if (!replicatorNumberTemplate.match(ui->m_replicatorNumber->text()).hasMatch()) {
        QToolTip::showText(ui->m_replicatorNumber->mapToGlobal(QPoint(0, 15)), tr("Invalid replicator number amount!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_replicatorNumber->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->m_replicatorNumber->setProperty("is_valid", true);
        validate();
    }

    if (!driveSizeTemplate.match(ui->m_maxDriveSize->text()).hasMatch()) {
        QToolTip::showText(ui->m_maxDriveSize->mapToGlobal(QPoint(0, 15)), tr("Invalid drive size!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_maxDriveSize->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->m_maxDriveSize->setProperty("is_valid", true);
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
        ui->m_maxDriveSize->property("is_valid").toBool() &&
        ui->m_localDriveFolder->property("is_valid").toBool()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}

void AddDriveDialog::accept()
{
    auto hash = mp_onChainClient->addDrive( ui->m_driveName->text().toStdString(),
                                            ui->m_maxDriveSize->text().toULongLong(),
                                            ui->m_replicatorNumber->text().toULongLong() );

    DriveInfo drive;

    drive.m_name             = ui->m_driveName->text().toStdString();
    drive.m_driveKey         = hash;
    drive.m_replicatorNumber = ui->m_replicatorNumber->text().toInt();
    drive.m_maxDriveSize     = ui->m_maxDriveSize->text().toInt();
    drive.m_localDriveFolder = ui->m_localDriveFolder->text().toStdString();

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    Model::drives().push_back( drive );
    Model::setCurrentDriveIndex( (int)Model::drives().size()-1 );
    gSettings.save();

    emit updateDrivesCBox();

    QDialog::accept();
}

void AddDriveDialog::reject()
{
    QDialog::reject();
}
