#include "Entities/Settings.h"
#include "Dialogs/SettingsDialog.h"
#include "PrivKeyDialog.h"
#include "xpxchaincpp/sdk.h"
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
    *mpSettingsDraft = *mpSettings;

    ui->setupUi(this);
    mpReset = ui->buttonBox->addButton("Reset", QDialogButtonBox::ButtonRole::ResetRole);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Save");

    setModal(true);
    initTooltips();
    fillAccountCbox( initSettings );
    updateAccountFields();

    ui->m_transactionFeeMultiplier->setText(QString::number(mpSettings->m_feeMultiplier));

    ui->m_replicatorBootAddrField->setEditable(true);
    ui->m_restBootAddrFieldCbox->setEditable(true);
    ui->m_restBootAddrFieldCbox->addItem(mpSettings->SIRIUS_MAINNET);

    const QString mainnetHost = std::get<0>(mpSettings->MAINNET_API_NODES[0]) + ":" + std::get<1>(mpSettings->MAINNET_API_NODES[0]);
    ui->m_restBootAddrFieldCbox->setItemData(ui->m_restBootAddrFieldCbox->count() - 1, mainnetHost, Qt::UserRole);
    ui->m_restBootAddrFieldCbox->addItem(mpSettings->SIRIUS_TESTNET);

    const QString testnetHost = std::get<0>(mpSettings->TESTNET_API_NODES[0]) + ":" + std::get<1>(mpSettings->TESTNET_API_NODES[0]);
    ui->m_restBootAddrFieldCbox->setItemData(ui->m_restBootAddrFieldCbox->count() - 1, testnetHost, Qt::UserRole);

    QRegularExpression addressTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"(((Sirius Mainnet)|(Sirius Testnet 2)|(([a-zA-Z0-9-]+\.)+[a-zA-Z]{2,}|([0-9]{1,3}\.){3}[0-9]{1,3}):[0-9]{1,5}))")));
    connect(ui->m_restBootAddrFieldCbox, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, [this,addressTemplate] (auto text)
    {
        if (text == mpSettings->SIRIUS_MAINNET) {
            ui->m_replicatorBootAddrField->addItem(mpSettings->SIRIUS_MAINNET);
            const QString mainnetReplicator = std::get<0>(mpSettings->MAINNET_REPLICATORS[0]) + ":" + std::get<1>(mpSettings->MAINNET_REPLICATORS[0]);
            ui->m_replicatorBootAddrField->setItemData(ui->m_replicatorBootAddrField->count() - 1, mainnetReplicator, Qt::UserRole);
            ui->m_replicatorBootAddrField->setCurrentIndex(ui->m_replicatorBootAddrField->count() - 1);
            ui->m_replicatorBootAddrField->setDisabled(true);
        } else if (text == mpSettings->SIRIUS_TESTNET) {
            ui->m_replicatorBootAddrField->addItem(mpSettings->SIRIUS_TESTNET);
            const QString testnetReplicator = std::get<0>(mpSettings->TESTNET_REPLICATORS[0]) + ":" + std::get<1>(mpSettings->TESTNET_REPLICATORS[0]);
            ui->m_replicatorBootAddrField->setItemData(ui->m_replicatorBootAddrField->count() - 1, testnetReplicator, Qt::UserRole);
            ui->m_replicatorBootAddrField->setCurrentIndex(ui->m_replicatorBootAddrField->count() - 1);
            ui->m_replicatorBootAddrField->setDisabled(true);
        } else {
            for (int i = ui->m_replicatorBootAddrField->count() - 1; i >= 0; --i) {
                if ((ui->m_replicatorBootAddrField->itemText(i) == mpSettings->SIRIUS_MAINNET) ||
                    (ui->m_replicatorBootAddrField->itemText(i) == mpSettings->SIRIUS_TESTNET)) {
                    ui->m_replicatorBootAddrField->removeItem(i);
                }
            }

            ui->m_replicatorBootAddrField->setEnabled(true);
        }

        if (!addressTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_restBootAddrFieldCbox->mapToGlobal(QPoint(0, 15)), tr("Invalid address!"), nullptr, {},3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_restBootAddrFieldCbox->setProperty("is_valid", false);
        } else if ((text != mpSettings->SIRIUS_MAINNET && text != mpSettings->SIRIUS_TESTNET) && !isResolvedToIpAddress(text)) {
            const QString error = "Host not found: " + text;
            QToolTip::showText(ui->m_restBootAddrFieldCbox->mapToGlobal(QPoint(0, 15)), tr(error.toStdString().c_str()), nullptr, {},3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_restBootAddrFieldCbox->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_restBootAddrFieldCbox->setProperty("is_valid", true);
            validate();
        }
    });

    connect(ui->m_replicatorBootAddrField, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, [this, addressTemplate] (auto text)
    {
       if (!addressTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->m_replicatorBootAddrField->mapToGlobal(QPoint(0, 15)), tr("Invalid address!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_replicatorBootAddrField->setProperty("is_valid", false);
        } else if ((text != mpSettings->SIRIUS_MAINNET && text != mpSettings->SIRIUS_TESTNET) && !isResolvedToIpAddress(text)) {
            const QString error = "Host not found: " + text;
            QToolTip::showText(ui->m_replicatorBootAddrField->mapToGlobal(QPoint(0, 15)), tr(error.toStdString().c_str()), nullptr, {}, 3000);
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
            qDebug() << "SettingsDialog::SettingsDialog. Selected name: " << QString::fromStdString( mpSettingsDraft->accountConfig().m_accountName );
            qDebug() << "SettingsDialog::SettingsDialog. Selected key: " << QString::fromStdString( mpSettingsDraft->accountConfig().m_publicKeyStr );
            qDebug() << "SettingsDialog::SettingsDialog. Selected privateKey: " << QString::fromStdString( mpSettingsDraft->accountConfig().m_privateKeyStr );
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

    connect(ui->m_newAccountBtn, &QPushButton::released, this, &SettingsDialog::onNewAccount );
    connect(ui->m_removeAccount, &QPushButton::released, this, &SettingsDialog::onRemoveAccount );
    connect(mpReset, &QPushButton::released, this, &SettingsDialog::onReset );

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    if ( initSettings )
    {
        ui->m_newAccountBtn->setEnabled(false);
        ui->m_copyAddress->setEnabled(false);
        ui->m_newAccountBtn->setStyleSheet("QPushButton { color: grey;}" );
    }

    connect(ui->m_copyKeyBtn, &QPushButton::released, this, [this](){
        QClipboard* clipboard = QApplication::clipboard();
        if (!clipboard) {
            qWarning() << LOG_SOURCE << "bad clipboard";
            return;
        }

        const QString publicKey = mpSettingsDraft->accountConfig().m_publicKeyStr.c_str();
        clipboard->setText(publicKey, QClipboard::Clipboard);
        if (clipboard->supportsSelection()) {
            clipboard->setText(publicKey, QClipboard::Selection);
        }
    });

    connect(ui->m_copyAddress, &QPushButton::released, this, [this](){
        QClipboard* clipboard = QApplication::clipboard();
        if (!clipboard) {
            qWarning() << "bad clipboard";
            return;
        }

        xpx_chain_sdk::Key publicKey;
        xpx_chain_sdk::ParseHexStringIntoContainer(mpSettingsDraft->accountConfig().m_publicKeyStr.c_str(), mpSettingsDraft->accountConfig().m_publicKeyStr.size(), publicKey);
        xpx_chain_sdk::PublicAccount publicAccount(publicKey, xpx_chain_sdk::GetConfig().NetworkId);

        clipboard->setText(publicAccount.address().encoded().c_str(), QClipboard::Clipboard);
        if (clipboard->supportsSelection()) {
            clipboard->setText(publicAccount.address().encoded().c_str(), QClipboard::Selection);
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


    auto restBootAddrFieldRaw = ui->m_restBootAddrFieldCbox->currentText();
    if (!addressTemplate.match(restBootAddrFieldRaw).hasMatch()) {
        QToolTip::showText(ui->m_restBootAddrFieldCbox->mapToGlobal(QPoint(0, 15)), tr("Invalid address!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_restBootAddrFieldCbox->setProperty("is_valid", false);
    } else if ((restBootAddrFieldRaw != mpSettings->SIRIUS_MAINNET || restBootAddrFieldRaw != mpSettings->SIRIUS_TESTNET) && !isResolvedToIpAddress(restBootAddrFieldRaw)) {
        const QString error = "Host not found: " + restBootAddrFieldRaw;
        QToolTip::showText(ui->m_restBootAddrFieldCbox->mapToGlobal(QPoint(0, 15)), tr(error.toStdString().c_str()), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_restBootAddrFieldCbox->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_restBootAddrFieldCbox->setProperty("is_valid", true);
        validate();
    }

    auto replicatorBootAddrFieldRaw = ui->m_replicatorBootAddrField->currentText();
    if (!addressTemplate.match(replicatorBootAddrFieldRaw).hasMatch()) {
        QToolTip::showText(ui->m_replicatorBootAddrField->mapToGlobal(QPoint(0, 15)), tr("Invalid address!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_replicatorBootAddrField->setProperty("is_valid", false);
    } else if ((replicatorBootAddrFieldRaw != mpSettings->SIRIUS_MAINNET || replicatorBootAddrFieldRaw != mpSettings->SIRIUS_TESTNET) && !isResolvedToIpAddress(replicatorBootAddrFieldRaw)) {
        const QString error = "Host not found: " + replicatorBootAddrFieldRaw;
        QToolTip::showText(ui->m_replicatorBootAddrField->mapToGlobal(QPoint(0, 15)), tr(error.toStdString().c_str()), nullptr, {}, 3000);
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
    ui->m_restBootAddrFieldCbox->setFocus();

    setTabOrder(ui->m_restBootAddrFieldCbox, ui->m_replicatorBootAddrField);
    setTabOrder(ui->m_replicatorBootAddrField, ui->m_portField);
    setTabOrder(ui->m_portField, ui->m_accountCbox);
    setTabOrder(ui->m_accountCbox, ui->m_newAccountBtn);
    setTabOrder(ui->m_newAccountBtn, ui->m_copyKeyBtn);
    setTabOrder(ui->m_copyKeyBtn, ui->m_dnFolderField);
    setTabOrder(ui->m_dnFolderField, ui->m_dnFolderBtn);
    setTabOrder(ui->m_dnFolderBtn, ui->m_transactionFeeMultiplier);
    setTabOrder(ui->m_transactionFeeMultiplier, ui->buttonBox);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::updateAccountFields()
{
    int index = ui->m_restBootAddrFieldCbox->findText(QString::fromStdString( mpSettingsDraft->m_restBootstrap ));
    if (index != -1) {
        ui->m_restBootAddrFieldCbox->setCurrentIndex(index);
    } else {
        ui->m_restBootAddrFieldCbox->addItem(QString::fromStdString( mpSettingsDraft->m_restBootstrap ));
    }

    index = ui->m_replicatorBootAddrField->findText(QString::fromStdString( mpSettingsDraft->m_replicatorBootstrap ));
    if (index != -1) {
        ui->m_replicatorBootAddrField->setCurrentIndex(index);
    } else {
        ui->m_replicatorBootAddrField->addItem(QString::fromStdString( mpSettingsDraft->m_replicatorBootstrap ));
    }

    ui->m_portField->setValidator( new QIntValidator(1025, 65535, this) );
    ui->m_portField->setText( QString::fromStdString( mpSettingsDraft->m_udpPort ));
    ui->m_dnFolderField->setText( QString::fromStdString( mpSettingsDraft->downloadFolder().string() ));
}

void SettingsDialog::accept()
{
    if (ui->m_restBootAddrFieldCbox->currentText() == mpSettings->SIRIUS_MAINNET ||
        ui->m_restBootAddrFieldCbox->currentText() == mpSettings->SIRIUS_TESTNET) {
        mpSettingsDraft->m_restBootstrap = extractEndpointFromComboBox(ui->m_restBootAddrFieldCbox);
    } else {
        mpSettingsDraft->m_restBootstrap = ui->m_restBootAddrFieldCbox->currentText().toStdString();
    }

    if (ui->m_replicatorBootAddrField->currentText() == mpSettings->SIRIUS_MAINNET ||
        ui->m_replicatorBootAddrField->currentText() == mpSettings->SIRIUS_TESTNET) {
        mpSettingsDraft->m_replicatorBootstrap = extractEndpointFromComboBox(ui->m_replicatorBootAddrField);
    } else {
        mpSettingsDraft->m_replicatorBootstrap = ui->m_replicatorBootAddrField->currentText().toStdString();
    }

    mpSettingsDraft->m_udpPort                 = ui->m_portField->text().toStdString();
    mpSettingsDraft->m_feeMultiplier           = ui->m_transactionFeeMultiplier->text().toDouble();
    mpSettingsDraft->accountConfig().m_downloadFolder = ui->m_dnFolderField->text().toStdString();

    bool ltSessionMustRestart = ( mpSettings->m_replicatorBootstrap           !=   mpSettingsDraft->m_replicatorBootstrap )
                                ||  ( mpSettings->m_udpPort                   !=   mpSettingsDraft->m_udpPort )
                                ||  ( mpSettings->accountConfig().m_privateKeyStr    !=   mpSettingsDraft->accountConfig().m_privateKeyStr );

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
    mpSettings->m_udpPort                 = mpSettingsDraft->m_udpPort;
    mpSettings->accountConfig().m_downloadFolder = mpSettingsDraft->accountConfig().m_downloadFolder;
    mpSettings->m_accounts                = mpSettingsDraft->m_accounts;
    mpSettings->m_isDriveStructureAsTree  = mpSettingsDraft->m_isDriveStructureAsTree;
    mpSettings->setCurrentAccountIndex( mpSettingsDraft->m_currentAccountIndex );
    mpSettings->saveSettings();

    qDebug() << "SettingsDialog::accept: accept name: " << QString::fromStdString( mpSettings->accountConfig().m_accountName );
    qDebug() << "SettingsDialog::accept: accept key: " << QString::fromStdString( mpSettings->accountConfig().m_publicKeyStr );
    qDebug() << "SettingsDialog::accept: accept privateKey: " << QString::fromStdString( mpSettings->accountConfig().m_privateKeyStr );
    qDebug() << "SettingsDialog::accept: REST bootstrap address: " << mpSettings->m_restBootstrap;
    qDebug() << "SettingsDialog::accept: REPLICATORS bootstrap addresses: " << mpSettings->m_replicatorBootstrap;

    emit closeLibtorrentPorts();
    QDialog::accept();
}

void SettingsDialog::reject()
{
    QDialog::reject();
}

void SettingsDialog::fillAccountCbox( bool initSettings )
{
    if ( initSettings )
    {
        auto name = mpSettingsDraft->accountConfig().m_accountName;
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

void SettingsDialog::onNewAccount()
{
    auto pKeyDialog = new PrivKeyDialog( mpSettingsDraft, this );

    connect(pKeyDialog, &PrivKeyDialog::accepted, this, [this](){
        ui->m_accountCbox->addItem( QString::fromStdString( mpSettingsDraft->accountConfig().m_accountName ));
        int index = mpSettingsDraft->m_currentAccountIndex;
        ui->m_accountCbox->setCurrentIndex(index);
        ui->m_removeAccount->setEnabled(true);
    });

    pKeyDialog->open();
}

void SettingsDialog::onRemoveAccount()
{
    QMessageBox msgBox;
    const auto currentAccountName = ui->m_accountCbox->currentText();
    const QString message = "Are you sure you want to permanently delete account' " + currentAccountName + "'?";
    msgBox.setText(message);
    msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
    msgBox.setDefaultButton(QMessageBox::StandardButton::Cancel);
    if (msgBox.exec() == QMessageBox::Cancel) {
        return;
    }

    int index = mpSettingsDraft->m_currentAccountIndex;
    mpSettingsDraft->m_currentAccountIndex = 0;
    // erase_if(mpSettingsDraft->m_accounts, [&currentAccountName](auto account)
    // {
    //     return account.m_accountName == currentAccountName.toStdString();
    // });
    auto it = std::find_if( mpSettingsDraft->m_accounts.begin(), mpSettingsDraft->m_accounts.end(), [&currentAccountName](auto account)
    {
        return account.m_accountName == currentAccountName.toStdString();
    });
    if ( it != mpSettingsDraft->m_accounts.end() )
    {
        mpSettingsDraft->m_accounts.erase(it);
    }

    ui->m_accountCbox->removeItem(index);
    if (ui->m_accountCbox->count() > 0) {
        ui->m_accountCbox->setCurrentIndex(0);
    } else {
        ui->m_removeAccount->setDisabled(true);
    }
}

void SettingsDialog::onReset()
{
    QMessageBox msgBox;
    QString message = "Are you sure you want to PERMANENTLY delete all data(private keys, metadata etc.)?";
    msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
    msgBox.setText(message);
    msgBox.setDefaultButton(QMessageBox::StandardButton::Cancel);
    if (msgBox.exec() == QMessageBox::Cancel) {
        return;
    }

    QDir dir(getSettingsFolder().string().c_str());
    if (!dir.exists())
    {
        message = "Settings folder does not exists: " + dir.absolutePath();
        msgBox.setText(message);
        msgBox.exec();
        qWarning() << "SettingsDialog::onReset: settings folder does not exists: " << dir.path();
        return;
    }

    if (dir.removeRecursively())
    {
        emit closeLibtorrentPorts();
    } else
    {
        qWarning() << "SettingsDialog::onReset: cannot remove settings folder: " << dir.path();
    }
}

void SettingsDialog::validate() {
    if (ui->m_restBootAddrFieldCbox->property("is_valid").toBool() &&
        ui->m_replicatorBootAddrField->property("is_valid").toBool() &&
        ui->m_portField->property("is_valid").toBool() &&
        ui->m_transactionFeeMultiplier->property("is_valid").toBool() &&
        ui->m_dnFolderField->property("is_valid").toBool()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}

void SettingsDialog::initTooltips() {
    const int tooltipsDuration = 20000;
    const int width = 18;
    const int height = 18;
    QIcon helpIcon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);
    ui->m_restServerHelp->setPixmap(helpIcon.pixmap(width, height));
    ui->m_restServerHelp->setStyleSheet("QToolTip { font-size: 14px; }");
    ui->m_restServerHelp->setToolTip("This field is for entering the IP address or domain name of the gateway\n"
                                     "used to connect to the blockchain and interact with its nodes.");
    ui->m_restServerHelp->setToolTipDuration(tooltipsDuration);

    ui->m_replicatorBootstrapHelp->setPixmap(helpIcon.pixmap(width, height));
    ui->m_replicatorBootstrapHelp->setStyleSheet("QToolTip { font-size: 14px; }");
    ui->m_replicatorBootstrapHelp->setToolTip("This field is for entering the IP address of the gateway used to\n"
                                              "connect to replicators and interact with the storage layer.");
    ui->m_replicatorBootstrapHelp->setToolTipDuration(tooltipsDuration);

    ui->m_localUDPPortHelp->setPixmap(helpIcon.pixmap(width, height));
    ui->m_localUDPPortHelp->setStyleSheet("QToolTip { font-size: 14px; }");
    ui->m_localUDPPortHelp->setToolTip("This field is for entering the local port used for interacting with\n"
                                       "the storage via the specified protocol. The default port is 6846.");
    ui->m_localUDPPortHelp->setToolTipDuration(tooltipsDuration);

    ui->m_downloadFolderHelp->setPixmap(helpIcon.pixmap(width, height));
    ui->m_downloadFolderHelp->setStyleSheet("QToolTip { font-size: 14px; }");
    ui->m_downloadFolderHelp->setToolTip("This field is for entering the path to the download folder,\n"
                                         "where files from the disk will be saved by default.");
    ui->m_downloadFolderHelp->setToolTipDuration(tooltipsDuration);

    ui->m_feeMultiplierHelp->setPixmap(helpIcon.pixmap(width, height));
    ui->m_feeMultiplierHelp->setStyleSheet("QToolTip { font-size: 14px; }");
    ui->m_feeMultiplierHelp->setToolTip("The fee multiplier is a factor used by the blockchain network to determine the actual\n"
                                        "transaction fee. When a transaction specifies a maximum fee, the block (or validator)\n"
                                        "sets the fee multiplier, which is applied to the base fee. The actual fee paid is\n"
                                        "the base fee multiplied by this multiplier. This mechanism helps adjust fees\n"
                                        "based on network congestion or blockchain policy.");
    ui->m_feeMultiplierHelp->setToolTipDuration(tooltipsDuration);
}