#include "Settings.h"
#include "SettingsDialog.h"
#include "PrivKeyDialog.h"
#include "./ui_SettingsDialog.h"

#include <QScreen>
#include <QIntValidator>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QToolTip>
#include <QRegularExpression>


SettingsDialog::SettingsDialog( Settings* settings, QWidget *parent, bool initSettings ) :
    QDialog( parent ),
    ui( new Ui::SettingsDialog() ),
    mpSettings(settings)
{
    mpSettingsDraft = new Settings(this);
    mpSettingsDraft = mpSettings;

    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Save");

    setModal(true);

    fillAccountCbox( initSettings );
    updateAccountFields();

    ui->m_transactionFeeMultiplier->setText(QString::number(mpSettings->m_feeMultiplier));

    QRegularExpression addressTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\:[0-9]{1,5})")));
    connect(ui->m_restBootAddrField, &QLineEdit::textChanged, this, [this,addressTemplate] (auto text)
    {
        if (!addressTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_restBootAddrField->mapToGlobal(QPoint(0, 15)), tr("Invalid address!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_restBootAddrField->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_restBootAddrField->setProperty("is_valid", true);
            validate();
        }
    });

    connect(ui->m_replicatorBootAddrField, &QLineEdit::textChanged, this, [this, addressTemplate] (auto text)
    {
        if (!addressTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_replicatorBootAddrField->mapToGlobal(QPoint(0, 15)), tr("Invalid address!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_replicatorBootAddrField->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_replicatorBootAddrField->setProperty("is_valid", true);
            validate();
        }
    });

    QRegularExpression portTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([0-9]{1,5})")));
    connect(ui->m_portField, &QLineEdit::textChanged, this, [this, portTemplate] (auto text)
    {
        if (!portTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_portField->mapToGlobal(QPoint(0, 15)), tr("Invalid port!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_portField->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_portField->setProperty("is_valid", true);
            validate();
        }
    });

    connect(ui->m_accountCbox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] (int index)
    {
        qDebug() << "Settings::QComboBox::currentIndexChanged: " << index;
        if ( mpSettingsDraft->accountList().size() > size_t(index) )
        {
            mpSettingsDraft->setCurrentAccountIndex(index);
            qDebug() << "SettingsDialog::SettingsDialog. Selected name: " << QString::fromStdString( mpSettingsDraft->config().m_accountName );
            qDebug() << "SettingsDialog::SettingsDialog. Selected key: " << QString::fromStdString( mpSettingsDraft->config().m_publicKeyStr );
            qDebug() << "SettingsDialog::SettingsDialog. Selected privateKey: " << QString::fromStdString( mpSettingsDraft->config().m_privateKeyStr );
            updateAccountFields();
        }
    });

    connect(ui->m_dnFolderBtn, &QPushButton::released, this, [this]()
    {
        QFlags<QFileDialog::Option> options = QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks;
#ifdef Q_OS_LINUX
        options |= QFileDialog::DontUseNativeDialog;
#endif
        const QString path = QFileDialog::getExistingDirectory(this, tr("Choose directory"), "/", options);
        ui->m_dnFolderField->setText(path.trimmed());
    });

    connect(ui->m_newAccountBtn, SIGNAL (released()), this, SLOT( onNewAccountBtn() ) );

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    if ( initSettings )
    {
        ui->m_newAccountBtn->setEnabled(false);
        ui->m_newAccountBtn->setStyleSheet("QPushButton { color: grey;}" );
    }

    connect(ui->m_copyKeyBtn, &QPushButton::released, this, [this](){
        QClipboard* clipboard = QApplication::clipboard();
        if (!clipboard) {
            qWarning() << LOG_SOURCE << "bad clipboard";
            return;
        }

        const QString publicKey = mpSettingsDraft->config().m_publicKeyStr.c_str();
        clipboard->setText(publicKey, QClipboard::Clipboard);
        if (clipboard->supportsSelection()) {
            clipboard->setText(publicKey, QClipboard::Selection);
        }
    });

    connect(ui->m_dnFolderField, &QLineEdit::textChanged, this, [this](auto text){
        bool isDirExists = QDir(text).exists();
        if (text.trimmed().isEmpty() || !isDirExists) {
            QToolTip::showText(ui->m_dnFolderField->mapToGlobal(QPoint(0, 15)), tr(isDirExists ? "Invalid path!" : "Folder does not exists!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_dnFolderField->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_dnFolderField->setProperty("is_valid", true);
            validate();
        }
    });

    QRegularExpression feeMultiplierTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([0-9]{1,100})")));
    connect(ui->m_transactionFeeMultiplier, &QLineEdit::textChanged, this, [this, feeMultiplierTemplate] (auto text)
    {
        if (!feeMultiplierTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_transactionFeeMultiplier->mapToGlobal(QPoint(0, 15)), tr("Invalid fee multiplier!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_transactionFeeMultiplier->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_transactionFeeMultiplier->setProperty("is_valid", true);
            validate();
        }
    });

    if (!addressTemplate.match(ui->m_restBootAddrField->text()).hasMatch()) {
        QToolTip::showText(ui->m_restBootAddrField->mapToGlobal(QPoint(0, 15)), tr("Invalid address!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_restBootAddrField->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_restBootAddrField->setProperty("is_valid", true);
        validate();
    }

    if (!addressTemplate.match(ui->m_replicatorBootAddrField->text()).hasMatch()) {
        QToolTip::showText(ui->m_replicatorBootAddrField->mapToGlobal(QPoint(0, 15)), tr("Invalid address!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_replicatorBootAddrField->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_replicatorBootAddrField->setProperty("is_valid", true);
        validate();
    }

    if (!portTemplate.match(ui->m_portField->text()).hasMatch()) {
        QToolTip::showText(ui->m_portField->mapToGlobal(QPoint(0, 15)), tr("Invalid address!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_portField->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_portField->setProperty("is_valid", true);
        validate();
    }

    if (!feeMultiplierTemplate.match(ui->m_transactionFeeMultiplier->text()).hasMatch()) {
        QToolTip::showText(ui->m_transactionFeeMultiplier->mapToGlobal(QPoint(0, 15)), tr("Invalid fee multiplier!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_transactionFeeMultiplier->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_transactionFeeMultiplier->setProperty("is_valid", true);
        validate();
    }

    bool isDirExists = QDir(ui->m_dnFolderField->text().trimmed()).exists();
    if (ui->m_dnFolderField->text().trimmed().isEmpty() || !isDirExists) {
        QToolTip::showText(ui->m_dnFolderField->mapToGlobal(QPoint(0, 15)), tr(isDirExists ? "Invalid path!" : "Folder does not exists!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->m_dnFolderField->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->m_dnFolderField->setProperty("is_valid", true);
        validate();
    }

    if (initSettings) {
        auto basicGeometry = QGuiApplication::primaryScreen()->geometry();
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), basicGeometry));
    }

    setToolTipDuration(10);
    setWindowTitle("Settings");
    setFocus();
    ui->m_restBootAddrField->setFocus();

    setTabOrder(ui->m_restBootAddrField, ui->m_replicatorBootAddrField);
    setTabOrder(ui->m_replicatorBootAddrField, ui->m_portField);
    setTabOrder(ui->m_portField, ui->m_accountCbox);
    setTabOrder(ui->m_accountCbox, ui->m_newAccountBtn);
    setTabOrder(ui->m_newAccountBtn, ui->m_copyKeyBtn);
    setTabOrder(ui->m_copyKeyBtn, ui->m_dnFolderField);
    setTabOrder(ui->m_dnFolderField, ui->m_dnFolderBtn);
    setTabOrder(ui->m_dnFolderBtn, ui->m_transactionFeeMultiplier);
    setTabOrder(ui->m_transactionFeeMultiplier, ui->m_driveStructureAsTree);
    setTabOrder(ui->m_driveStructureAsTree, ui->buttonBox);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::updateAccountFields()
{
    ui->m_restBootAddrField->setText( QString::fromStdString( mpSettingsDraft->m_restBootstrap ));
    ui->m_replicatorBootAddrField->setText( QString::fromStdString( mpSettingsDraft->m_replicatorBootstrap ));
    ui->m_portField->setValidator( new QIntValidator(1025, 65535, this) );
    ui->m_portField->setText( QString::fromStdString( mpSettingsDraft->m_udpPort ));
    ui->m_dnFolderField->setText( QString::fromStdString( mpSettingsDraft->downloadFolder().string() ));
}

void SettingsDialog::accept()
{
    mpSettingsDraft->m_restBootstrap           = ui->m_restBootAddrField->text().toStdString();
    mpSettingsDraft->m_replicatorBootstrap     = ui->m_replicatorBootAddrField->text().toStdString();
    mpSettingsDraft->m_udpPort                 = ui->m_portField->text().toStdString();
    mpSettingsDraft->m_feeMultiplier           = ui->m_transactionFeeMultiplier->text().toDouble();
    mpSettingsDraft->config().m_downloadFolder = ui->m_dnFolderField->text().toStdString();
    mpSettingsDraft->m_isDriveStructureAsTree  = ui->m_driveStructureAsTree->isChecked();

    bool ltSessionMustRestart = ( mpSettings->m_replicatorBootstrap           !=   mpSettingsDraft->m_replicatorBootstrap )
                                ||  ( mpSettings->m_udpPort                   !=   mpSettingsDraft->m_udpPort )
                                ||  ( mpSettings->config().m_privateKeyStr    !=   mpSettingsDraft->config().m_privateKeyStr );

    if ( ltSessionMustRestart && mpSettings->loaded() )
    {
        QMessageBox msgBox;
        msgBox.setText("Account changed.\nApplication should be restarted");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        int reply = msgBox.exec();
        if ( reply == QMessageBox::Cancel )
        {
            return;
        }
    }

    mpSettings->m_restBootstrap           = mpSettingsDraft->m_restBootstrap;
    mpSettings->m_replicatorBootstrap     = mpSettingsDraft->m_replicatorBootstrap;
    mpSettings->config().m_downloadFolder = mpSettingsDraft->config().m_downloadFolder;
    mpSettings->m_accounts                = mpSettingsDraft->m_accounts;
    mpSettings->m_isDriveStructureAsTree  = mpSettingsDraft->m_isDriveStructureAsTree;
    mpSettings->setCurrentAccountIndex( mpSettingsDraft->m_currentAccountIndex );
    mpSettings->save();

    qDebug() << LOG_SOURCE << "accept name: " << QString::fromStdString( mpSettings->config().m_accountName );
    qDebug() << LOG_SOURCE << "accept key: " << QString::fromStdString( mpSettings->config().m_publicKeyStr );
    qDebug() << LOG_SOURCE << "accept privateKey: " << QString::fromStdString( mpSettings->config().m_privateKeyStr );

    emit closeLibtorrentPorts();
    QDialog::accept();

//    QCoreApplication::exit(1024);
//    QDialog::accept();
}

void SettingsDialog::reject()
{
    QDialog::reject();
}

void SettingsDialog::fillAccountCbox( bool initSettings )
{
    if ( initSettings )
    {
        auto name = mpSettingsDraft->config().m_accountName;
        ui->m_accountCbox->addItem( QString::fromStdString(name) );
    }
    else
    {
        auto list = mpSettingsDraft->accountList();

        for( const auto& element: list )
        {
            ui->m_accountCbox->addItem( QString::fromStdString(element) );
        }
    }

    int index = mpSettingsDraft->m_currentAccountIndex;
    ASSERT( index >= 0 );
    ui->m_accountCbox->setCurrentIndex(index);
}

void SettingsDialog::onNewAccountBtn()
{
    auto pKeyDialog = new PrivKeyDialog( mpSettingsDraft, this );

    connect(pKeyDialog, &PrivKeyDialog::accepted, this, [this](){
        ui->m_accountCbox->addItem( QString::fromStdString( mpSettingsDraft->config().m_accountName ));
        int index = mpSettingsDraft->m_currentAccountIndex;
        ui->m_accountCbox->setCurrentIndex(index);
    });

    pKeyDialog->open();
}

void SettingsDialog::validate() {
    if (ui->m_restBootAddrField->property("is_valid").toBool() &&
        ui->m_replicatorBootAddrField->property("is_valid").toBool() &&
        ui->m_portField->property("is_valid").toBool() &&
        ui->m_transactionFeeMultiplier->property("is_valid").toBool() &&
        ui->m_dnFolderField->property("is_valid").toBool()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}
