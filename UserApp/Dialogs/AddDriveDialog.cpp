#include "Models/Model.h"
#include "AddDriveDialog.h"
#include "./ui_AddDriveDialog.h"

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
    ui->buttonBox->button(QDialogButtonBox::Help)->setText("Help");

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
        bool isDirExists = QDir(text).exists();
        if (text.trimmed().isEmpty() || !isDirExists) {
#ifndef __APPLE__
            QToolTip::showText(ui->m_localDriveFolder->mapToGlobal(QPoint(0, 15)), tr(isDirExists ? "Invalid path!" : "Folder does not exist!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
#endif
            ui->m_localDriveFolder->setProperty("is_valid", true);
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
    connect(ui->buttonBox, &QDialogButtonBox::helpRequested, this, &AddDriveDialog  ::displayInfo);


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

    bool isDirExists = QDir(ui->m_localDriveFolder->text().trimmed()).exists();
    if (ui->m_localDriveFolder->text().trimmed().isEmpty() || !isDirExists) {
        QToolTip::showText(ui->m_localDriveFolder->mapToGlobal(QPoint(0, 15)), tr(isDirExists ? "Invalid path!" : "Folder does not exists!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_localDriveFolder->setProperty("is_valid", true);
    } else {
        QToolTip::hideText();
        ui->m_localDriveFolder->setProperty("is_valid", true);
        validate();
    }

    setWindowTitle("Create Drive");
    setFocus();
    ui->m_driveName->setFocus();

    setTabOrder(ui->m_driveName, ui->m_replicatorNumber);
    setTabOrder(ui->m_replicatorNumber, ui->m_size);
    setTabOrder(ui->m_size, ui->m_localDriveFolder);
    setTabOrder(ui->m_localDriveFolder, ui->m_localFolderBtn);
    setTabOrder(ui->m_localFolderBtn, ui->buttonBox);
}

AddDriveDialog::~AddDriveDialog()
{
    if(helpMessageBox) {
        helpMessageBox->hide();
        delete helpMessageBox;
        helpMessageBox = nullptr;
    }
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
    if ( ui->m_localDriveFolder->text().isEmpty() )
    {
        QMessageBox::critical( nullptr, "Local drive folder not set", "Local drive folder not set" );
        return;
    }

    std::error_code ec;
    if ( fs::is_directory( ui->m_localDriveFolder->text().toStdString(), ec ) || ! ec )
    {
        QMessageBox::critical( nullptr, "Folder not exists", ui->m_localDriveFolder->text() );
        return;
    }
    
    auto callback = [model = mp_model,
                     name = ui->m_driveName->text().toStdString(),
                     count = ui->m_replicatorNumber->text().toInt(),
                     size = ui->m_size->text().toInt(),
                     folder = ui->m_localDriveFolder->text().toStdString()](auto hash) {
        Drive drive;
        drive.setName(name);
        drive.setKey(hash);
        drive.setReplicatorsCount(count);
        drive.setSize(size);
        drive.setLocalFolder(folder);
        drive.setLocalFolderExists(true);

        model->addDrive(drive);
        model->setCurrentDriveKey( drive.getKey() );
        model->saveSettings();
        auto currentDrive = model->currentDrive();
        if (currentDrive) {
            currentDrive->updateDriveState(creating);
        }
    };

    mp_onChainClient->addDrive( ui->m_size->text().toULongLong(), ui->m_replicatorNumber->text().toULongLong(), callback);
    QDialog::accept();
}

void AddDriveDialog::reject()
{
    QDialog::reject();
}

void AddDriveDialog::displayInfo()
{
    if(helpMessageBox) {
        helpMessageBox->hide();
        helpMessageBox = nullptr;
    }
    QString message = "<html>"
                      "<b>Drive name</b> is a preferred name for the drive. "
                      "It can contain capital and small latin letters as well as digits.<br><br>"
                      "<b>Replicator number</b> is the desired number of computers to store your data. "
                      "If you aren't sure, set <b>Replicator number</b> to <b>5</b>.<br><br>"
                      "<b>Max Drive Size</b> is the maximum size of the drive. "
                      "You can't sore more data than that. <br><br>"
                      "<b>Local Drive folder</b> is the folder associated with the drive. "
                      "Changes in this folder are reflected in the Drives tab (right panel)."
                      "</html>";
    helpMessageBox = new QMessageBox(this);
    helpMessageBox->setWindowTitle("Help");
    helpMessageBox->setText(message);
    helpMessageBox->setWindowModality(Qt::NonModal); // Set the NonModal flag
    helpMessageBox->show();
    helpMessageBox->move(this->x() + this->width()/2-this->helpMessageBox->width()/2, this->y() + this->height() + 60);

}

