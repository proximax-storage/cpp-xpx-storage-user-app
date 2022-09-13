#include "Settings.h"
#include "SettingsDialog.h"
#include "PrivKeyDialog.h"
#include "./ui_SettingsDialog.h"

#include "drive/Utils.h"

#include <QIntValidator>
#include <QMessageBox>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>


SettingsDialog::SettingsDialog( QWidget *parent, bool initSettings ) :
    QDialog( parent ),
    ui( new Ui::SettingsDialog() )
{
    m_settingsCopy = gSettings;

    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Save");

    setModal(true);

    ui->m_errorText->setText("");

    connect(ui->m_restBootAddrField, &QLineEdit::textChanged, this, [this] ()
    {
        ui->m_errorText->setText("");
    });
    connect(ui->m_replicatorBootAddrField, &QLineEdit::textChanged, this, [this] ()
    {
        ui->m_errorText->setText("");
    });
    connect(ui->m_portField, &QLineEdit::textChanged, this, [this] ()
    {
        ui->m_errorText->setText("");
    });
    connect(ui->m_accountCbox, &QComboBox::currentIndexChanged, this, [this] (int index)
    {
        //todo
        qDebug() << "Settings::QComboBox::currentIndexChanged: " << index;
        if ( m_settingsCopy.accountList().size() > size_t(index) )
        {
            qDebug() << "selected: " << QString::fromStdString( m_settingsCopy.accountList()[index] );
            qDebug() << "2 selected: " << QString::fromStdString( m_settingsCopy.config().m_publicKeyStr );
            m_settingsCopy.setCurrentAccountIndex(index);
            updateAccountFields();
        }
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

    fillAccountCbox( initSettings );


    setWindowTitle("Settings");
    setFocus();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::updateAccountFields()
{
    ui->m_restBootAddrField->setText( QString::fromStdString( m_settingsCopy.config().m_restBootstrap ));
    ui->m_replicatorBootAddrField->setText( QString::fromStdString( m_settingsCopy.config().m_replicatorBootstrap ));
    ui->m_portField->setValidator( new QIntValidator(1025, 65535, this) );
    ui->m_portField->setText( QString::fromStdString( m_settingsCopy.config().m_udpPort ));
    ui->m_dnFolderField->setText( QString::fromStdString( m_settingsCopy.config().m_downloadFolder ));
}

void SettingsDialog::accept()
{
    if ( verify() )
    {

        bool ltSessionMustRestart = ( gSettings.config().m_replicatorBootstrap !=   m_settingsCopy.config().m_replicatorBootstrap )
                                ||  ( gSettings.config().m_udpPort             !=   m_settingsCopy.config().m_udpPort )
                                ||  ( gSettings.config().m_privateKeyStr       !=   m_settingsCopy.config().m_privateKeyStr );

        if ( ltSessionMustRestart && gSettings.loaded() )
        {
            QMessageBox msgBox;
            msgBox.setText("Changed\nApplication will be restarted");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            int reply = msgBox.exec();
            if ( reply == QMessageBox::Cancel )
            {
                return;
            }
        }

        gSettings.config().m_restBootstrap          = m_settingsCopy.config().m_restBootstrap;
        gSettings.config().m_replicatorBootstrap    = m_settingsCopy.config().m_replicatorBootstrap;
        gSettings.config().m_downloadFolder         = m_settingsCopy.config().m_downloadFolder;
        qDebug() << "m_downloadFolder: " << gSettings.config().m_downloadFolder.c_str();
        gSettings.m_accounts                        = m_settingsCopy.m_accounts;
        gSettings.setCurrentAccountIndex( m_settingsCopy.m_currentAccountIndex );
        gSettings.save();

        QDialog::accept();
    }
}

void SettingsDialog::reject()
{
    QDialog::reject();

//    if ( ! gSettings.loaded() )
//    {
//        qApp->exit(0);
//    }
}

void SettingsDialog::fillAccountCbox( bool initSettings )
{
    if ( initSettings )
    {
        auto currentAccountKeyStr = m_settingsCopy.config().m_publicKeyStr;
        ui->m_accountCbox->addItem( QString::fromStdString(currentAccountKeyStr) );
    }
    else
    {
        auto list = m_settingsCopy.accountList();

        for( const auto& element: list )
        {
            ui->m_accountCbox->addItem( QString::fromStdString(element) );
        }
    }

    int index = m_settingsCopy.m_currentAccountIndex;
    assert( index >= 0 );
    ui->m_accountCbox->setCurrentIndex(index);
}

bool SettingsDialog::verify()
{
    m_settingsCopy.config().m_restBootstrap       = ui->m_restBootAddrField->text().toStdString();
    m_settingsCopy.config().m_replicatorBootstrap = ui->m_replicatorBootAddrField->text().toStdString();
    m_settingsCopy.config().m_udpPort             = ui->m_portField->text().toStdString();
    m_settingsCopy.config().m_downloadFolder      = ui->m_dnFolderField->text().toStdString();

    //
    // Verify restBootstrap endpoint
    //
    std::vector<std::string> addressAndPort;
    boost::split( addressAndPort, m_settingsCopy.config().m_restBootstrap, [](char c) { return c==':'; } );
    if ( addressAndPort.size() != 2 )
    {
        ui->m_errorText->setText( QString::fromStdString("Invalid REST Server Address" ));
        return false;
    }

    //
    // Verify replicatorBootstrap endpoint
    //
    boost::split( addressAndPort, m_settingsCopy.config().m_replicatorBootstrap, [](char c) { return c==':'; } );
    if ( addressAndPort.size() != 2 )
    {
        ui->m_errorText->setText("Invalid Replicator Booststrap Address");
        return false;
    }

    return true;
}

void SettingsDialog::onNewAccountBtn()
{
    PrivKeyDialog pKeyDialog( this, m_settingsCopy );
    pKeyDialog.exec();

    if ( pKeyDialog.result() == QDialog::Accepted )
    {
        ui->m_accountCbox->addItem( QString::fromStdString( m_settingsCopy.config().m_publicKeyStr ));
        int index = m_settingsCopy.m_currentAccountIndex;
        ui->m_accountCbox->setCurrentIndex(index);
    }
}
