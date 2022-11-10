#include "Settings.h"
#include "SettingsDialog.h"
#include "PrivKeyDialog.h"
#include "./ui_SettingsDialog.h"

#include <QIntValidator>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QToolTip>
#include <QRegExp>

// not saved properties
Settings gSettingsCopy;

SettingsDialog::SettingsDialog( QWidget *parent, bool initSettings ) :
    QDialog( parent ),
    ui( new Ui::SettingsDialog() )
{
    gSettingsCopy = gSettings;
    qDebug() << "gSettings.m_currentAccountIndex: " << gSettings.m_currentAccountIndex << " " << gSettingsCopy.m_currentAccountIndex;

    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Save");

    setModal(true);

    QRegExp addressTemplate(R"([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\:[0-9]{1,5})");
    addressTemplate.setCaseSensitivity(Qt::CaseInsensitive);
    addressTemplate.setPatternSyntax(QRegExp::RegExp);

    fillAccountCbox( initSettings );
    updateAccountFields();

    connect(ui->m_restBootAddrField, &QLineEdit::textChanged, this, [this,addressTemplate] (auto text)
    {
        if (!addressTemplate.exactMatch(text)) {
            QToolTip::showText(ui->m_restBootAddrField->mapToGlobal(QPoint()), tr("Invalid address!"));
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
        if (!addressTemplate.exactMatch(text)) {
            QToolTip::showText(ui->m_replicatorBootAddrField->mapToGlobal(QPoint()), tr("Invalid address!"));
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_replicatorBootAddrField->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_replicatorBootAddrField->setProperty("is_valid", true);
            validate();
        }
    });

    QRegExp portTemplate(R"([0-9]{1,5})");
    portTemplate.setCaseSensitivity(Qt::CaseInsensitive);
    portTemplate.setPatternSyntax(QRegExp::RegExp);

    connect(ui->m_portField, &QLineEdit::textChanged, this, [this, portTemplate] (auto text)
    {
        if (!portTemplate.exactMatch(text)) {
            QToolTip::showText(ui->m_portField->mapToGlobal(QPoint()), tr("Invalid port!"));
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
        //todo
        qDebug() << LOG_SOURCE << "Settings::QComboBox::currentIndexChanged: " << index;
        if ( gSettingsCopy.accountList().size() > size_t(index) )
        {
            gSettingsCopy.setCurrentAccountIndex(index);
            qDebug() << LOG_SOURCE << "selected name: " << QString::fromStdString( gSettingsCopy.config().m_accountName );
            qDebug() << LOG_SOURCE << "selected key: " << QString::fromStdString( gSettingsCopy.config().m_publicKeyStr );
            qDebug() << LOG_SOURCE << "selected privateKey: " << QString::fromStdString( gSettingsCopy.config().m_privateKeyStr );
            updateAccountFields();
        }
    });

    connect(ui->m_dnFolderBtn, &QPushButton::released, this, [this]()
    {
        const QString path = QFileDialog::getExistingDirectory(this,
                                                               tr("Choose directory"), "/",
                                                               QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        ui->m_dnFolderField->setText(path.isEmpty() ? gSettingsCopy.config().m_downloadFolder.c_str() : path);
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

    connect(ui->m_copyKeyBtn, &QPushButton::released, this, [](){
        QClipboard* clipboard = QApplication::clipboard();
        if (!clipboard) {
            qWarning() << LOG_SOURCE << "bad clipboard";
            return;
        }

        const QString publicKey = gSettingsCopy.config().m_publicKeyStr.c_str();
        clipboard->setText(publicKey, QClipboard::Clipboard);
        if (clipboard->supportsSelection()) {
            clipboard->setText(publicKey, QClipboard::Selection);
        }
    });

    connect(ui->m_dnFolderField, &QLineEdit::textChanged, this, [this](auto text){
        if (text.trimmed().isEmpty()) {
            QToolTip::showText(ui->m_dnFolderField->mapToGlobal(QPoint()), tr("Invalid path!"));
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_dnFolderField->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_dnFolderField->setProperty("is_valid", true);
            validate();
        }
    });

    if (!addressTemplate.exactMatch(ui->m_restBootAddrField->text())) {
        QToolTip::showText(ui->m_restBootAddrField->mapToGlobal(QPoint()), tr("Invalid address!"));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_restBootAddrField->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_restBootAddrField->setProperty("is_valid", true);
        validate();
    }

    if (!addressTemplate.exactMatch(ui->m_replicatorBootAddrField->text())) {
        QToolTip::showText(ui->m_replicatorBootAddrField->mapToGlobal(QPoint()), tr("Invalid address!"));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_replicatorBootAddrField->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_replicatorBootAddrField->setProperty("is_valid", true);
        validate();
    }

    if (!portTemplate.exactMatch(ui->m_portField->text())) {
        QToolTip::showText(ui->m_portField->mapToGlobal(QPoint()), tr("Invalid address!"));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_portField->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_portField->setProperty("is_valid", true);
        validate();
    }

    if (ui->m_dnFolderField->text().trimmed().isEmpty()) {
        QToolTip::showText(ui->m_dnFolderField->mapToGlobal(QPoint()), tr("Invalid download folder path!"));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->m_dnFolderField->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->m_dnFolderField->setProperty("is_valid", true);
        validate();
    }

    setToolTipDuration(10);
    setWindowTitle("Settings");
    setFocus();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::updateAccountFields()
{
    ui->m_restBootAddrField->setText( QString::fromStdString( gSettingsCopy.m_restBootstrap ));
    ui->m_replicatorBootAddrField->setText( QString::fromStdString( gSettingsCopy.m_replicatorBootstrap ));
    ui->m_portField->setValidator( new QIntValidator(1025, 65535, this) );
    ui->m_portField->setText( QString::fromStdString( gSettingsCopy.m_udpPort ));
    ui->m_dnFolderField->setText( QString::fromStdString( gSettingsCopy.downloadFolder() ));
}

void SettingsDialog::accept()
{
    gSettingsCopy.m_restBootstrap           = ui->m_restBootAddrField->text().toStdString();
    gSettingsCopy.m_replicatorBootstrap     = ui->m_replicatorBootAddrField->text().toStdString();
    gSettingsCopy.m_udpPort                 = ui->m_portField->text().toStdString();
    gSettingsCopy.config().m_downloadFolder = ui->m_dnFolderField->text().toStdString();

    bool ltSessionMustRestart = ( gSettings.m_replicatorBootstrap       !=   gSettingsCopy.m_replicatorBootstrap )
                                ||  ( gSettings.m_udpPort                   !=   gSettingsCopy.m_udpPort )
                                ||  ( gSettings.config().m_privateKeyStr    !=   gSettingsCopy.config().m_privateKeyStr );

    if ( ltSessionMustRestart && gSettings.loaded() )
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

    gSettings.m_restBootstrap           = gSettingsCopy.m_restBootstrap;
    gSettings.m_replicatorBootstrap     = gSettingsCopy.m_replicatorBootstrap;
    gSettings.config().m_downloadFolder = gSettingsCopy.config().m_downloadFolder;
    gSettings.m_accounts                = gSettingsCopy.m_accounts;
    gSettings.setCurrentAccountIndex( gSettingsCopy.m_currentAccountIndex );
    gSettings.save();

    qDebug() << LOG_SOURCE << "accept name: " << QString::fromStdString( gSettings.config().m_accountName );
    qDebug() << LOG_SOURCE << "accept key: " << QString::fromStdString( gSettings.config().m_publicKeyStr );
    qDebug() << LOG_SOURCE << "accept privateKey: " << QString::fromStdString( gSettings.config().m_privateKeyStr );

    QCoreApplication::exit(1024);
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
        auto name = gSettingsCopy.config().m_accountName;
        ui->m_accountCbox->addItem( QString::fromStdString(name) );
    }
    else
    {
        auto list = gSettingsCopy.accountList();

        for( const auto& element: list )
        {
            ui->m_accountCbox->addItem( QString::fromStdString(element) );
        }
    }

    int index = gSettingsCopy.m_currentAccountIndex;
    assert( index >= 0 );
    qDebug() << "ui->m_accountCbox->setCurrentIndex: " << index;
    ui->m_accountCbox->setCurrentIndex(index);
}

void SettingsDialog::onNewAccountBtn()
{
    PrivKeyDialog pKeyDialog( this, gSettingsCopy );
    pKeyDialog.exec();

    if ( pKeyDialog.result() == QDialog::Accepted )
    {
        ui->m_accountCbox->addItem( QString::fromStdString( gSettingsCopy.config().m_publicKeyStr ));
        int index = gSettingsCopy.m_currentAccountIndex;
        ui->m_accountCbox->setCurrentIndex(index);
    }
}

void SettingsDialog::validate() {
    if (ui->m_restBootAddrField->property("is_valid").toBool() &&
        ui->m_replicatorBootAddrField->property("is_valid").toBool() &&
        ui->m_portField->property("is_valid").toBool() &&
        ui->m_dnFolderField->property("is_valid").toBool()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}
