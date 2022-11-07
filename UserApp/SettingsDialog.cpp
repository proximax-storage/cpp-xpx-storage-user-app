#include "Settings.h"
#include "SettingsDialog.h"
#include "PrivKeyDialog.h"
#include "./ui_SettingsDialog.h"

#include "drive/Utils.h"

#include <QIntValidator>
#include <QMessageBox>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>


// not saved properties
Settings gSettingsCopy;

SettingsDialog::SettingsDialog( QWidget *parent, bool initSettings ) :
    QDialog( parent ),
    ui( new Ui::SettingsDialog() )
{
    gSettingsCopy = gSettings;

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

    connect(ui->m_accountCbox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] (int index)
    {
        //todo
        qDebug() << LOG_SOURCE << "Settings::QComboBox::currentIndexChanged: " << index;
        if ( gSettingsCopy.accountList().size() > size_t(index) )
        {
            qDebug() << LOG_SOURCE << "selected: " << QString::fromStdString( gSettingsCopy.accountList()[index] );
            qDebug() << LOG_SOURCE << "2 selected: " << QString::fromStdString( gSettingsCopy.config().m_publicKeyStr );
            gSettingsCopy.setCurrentAccountIndex(index);
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
    ui->m_restBootAddrField->setText( QString::fromStdString( gSettingsCopy.config().m_restBootstrap ));
    ui->m_replicatorBootAddrField->setText( QString::fromStdString( gSettingsCopy.config().m_replicatorBootstrap ));
    ui->m_portField->setValidator( new QIntValidator(1025, 65535, this) );
    ui->m_portField->setText( QString::fromStdString( gSettingsCopy.config().m_udpPort ));
    ui->m_dnFolderField->setText( QString::fromStdString( gSettingsCopy.downloadFolder() ));
}

void SettingsDialog::accept()
{
    if ( verify() )
    {

        bool ltSessionMustRestart = ( gSettings.config().m_replicatorBootstrap !=   gSettingsCopy.config().m_replicatorBootstrap )
                                ||  ( gSettings.config().m_udpPort             !=   gSettingsCopy.config().m_udpPort )
                                ||  ( gSettings.config().m_privateKeyStr       !=   gSettingsCopy.config().m_privateKeyStr );

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

        gSettings.config().m_restBootstrap          = gSettingsCopy.config().m_restBootstrap;
        gSettings.config().m_replicatorBootstrap    = gSettingsCopy.config().m_replicatorBootstrap;
        gSettings.config().m_downloadFolder         = gSettingsCopy.config().m_downloadFolder;
        gSettings.m_accounts                        = gSettingsCopy.m_accounts;
        gSettings.setCurrentAccountIndex( gSettingsCopy.m_currentAccountIndex );
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
        auto currentAccountKeyStr = gSettingsCopy.config().m_publicKeyStr;
        ui->m_accountCbox->addItem( QString::fromStdString(currentAccountKeyStr) );
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
    ui->m_accountCbox->setCurrentIndex(index);
}

bool SettingsDialog::verify()
{
    gSettingsCopy.config().m_restBootstrap       = ui->m_restBootAddrField->text().toStdString();
    gSettingsCopy.config().m_replicatorBootstrap = ui->m_replicatorBootAddrField->text().toStdString();
    gSettingsCopy.config().m_udpPort             = ui->m_portField->text().toStdString();
    gSettingsCopy.config().m_downloadFolder      = ui->m_dnFolderField->text().toStdString();

    //
    // Verify restBootstrap endpoint
    //
    std::vector<std::string> addressAndPort;
    boost::split( addressAndPort, gSettingsCopy.config().m_restBootstrap, [](char c) { return c==':'; } );
    if ( addressAndPort.size() != 2 )
    {
        ui->m_errorText->setText( QString::fromStdString("Invalid REST Server Address" ));
        return false;
    }

    //
    // Verify replicatorBootstrap endpoint
    //
    boost::split( addressAndPort, gSettingsCopy.config().m_replicatorBootstrap, [](char c) { return c==':'; } );
    if ( addressAndPort.size() != 2 )
    {
        ui->m_errorText->setText("Invalid Replicator Booststrap Address");
        return false;
    }

    return true;
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
