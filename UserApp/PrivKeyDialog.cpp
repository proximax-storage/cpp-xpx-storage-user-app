#include "PrivKeyDialog.h"
#include "Settings.h"
#include "mainwin.h"
#include "ui_PrivKeyDialog.h"

#include "crypto/Signer.h"
#include "drive/Utils.h"

#include <random>
#include <stdio.h>

#include <QMessageBox>
#include <QFileDialog>

PrivKeyDialog::PrivKeyDialog( QWidget *parent ) :
    QDialog(parent),
    ui(new Ui::PrivKeyDialog),
    m_settings(gSettings)
{
    init();
}

PrivKeyDialog::PrivKeyDialog( QWidget *parent, Settings& settings) :
    QDialog(parent),
    ui(new Ui::PrivKeyDialog),
    m_settings(settings)
{
    init();
}

void PrivKeyDialog::init()
{
    ui->setupUi(this);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Save");

    setModal(true);

    connect( ui->m_generateKeysBtn, SIGNAL (released()), this, SLOT( onGenerateKeysBtn() ));
    connect( ui->m_loadFromFileBtn, SIGNAL (released()), this, SLOT( onLoadFromFileBtn() ));

    connect( ui->m_pkField, &QLineEdit::textChanged, this, [this] ()
    {
        ui->m_errorText->setText("");
        verify();
    });

    ui->m_errorText->setText("");
    verify();
}

PrivKeyDialog::~PrivKeyDialog()
{
    delete ui;
}

void PrivKeyDialog::verify()
{
    auto accountName = ui->m_accountName->text().toStdString();

    // Verify account name
    //
    if ( accountName.size() == 0 )
    {
        ui->m_errorText->setText("Account name is not set");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    auto pkString = ui->m_pkField->text().toStdString();

    // Verify private key
    //
    if ( pkString.size() == 0 )
    {
        ui->m_errorText->setText("Private key is not set");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

//    qDebug() << LOG_SOURCE << "pkString.size(): " << pkString.size();
//    qDebug() << LOG_SOURCE << "pkString.find_first_not_of: " << pkString.find_first_not_of("0123456789abcdefABCDEF");

    if ( pkString.size() != 64 || pkString.find_first_not_of("0123456789abcdefABCDEF") < pkString.size() )
    {
        ui->m_errorText->setText("Invalid private key");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    auto it = std::find_if( m_settings.m_accounts.begin(), m_settings.m_accounts.end(), [&]( const auto& account )
    {
        return account.m_privateKeyStr == pkString;
    } );

    if ( it != m_settings.m_accounts.end() )
    {
        ui->m_errorText->setText("Account with same private key already exists");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }


    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(false);
}

void PrivKeyDialog::onGenerateKeysBtn()
{
    std::random_device   dev;
    std::seed_seq        seed({dev(), dev(), dev(), dev()});
    std::mt19937         rng(seed);

    std::array<uint8_t,32> buffer;

    std::generate( buffer.begin(), buffer.end(), [&]
    {
        return std::uniform_int_distribution<std::uint32_t>(0,0xff) ( rng );
    });

    ui->m_pkField->setText( QString::fromStdString( sirius::drive::toString( buffer ) ));
}

void PrivKeyDialog::onLoadFromFileBtn()
{

    auto fileName = QFileDialog::getOpenFileName( this, "Open File", "~", "");

    if ( !fileName.isNull() )
    {
        FILE* file = fopen( fileName.toStdString().c_str(), "rb" );
        if ( file == nullptr )
        {
            QMessageBox msgBox;
            msgBox.setText( QString::fromStdString( "Cannot open file") );
            msgBox.setInformativeText( fileName );
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
        }
        std::array<uint8_t,32> buffer;
        auto size = fread( buffer.data(), 1, 32, file );
        if ( size != 32)
        {
            QMessageBox msgBox;
            msgBox.setText( QString::fromStdString( "Invalid file length") );
            msgBox.setInformativeText( fileName );
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
        }

        ui->m_pkField->setText( QString::fromStdString( sirius::drive::toString( buffer ) ));
    }
}

void PrivKeyDialog::accept()
{
    std::string privateKeyStr = ui->m_pkField->text().toStdString();
    std::string accountName   = ui->m_accountName->text().toStdString();

    // Check unique key
    //
    auto it = std::find_if( m_settings.m_accounts.begin(), m_settings.m_accounts.end(), [&]( const auto& account )
    {
        return account.m_privateKeyStr == privateKeyStr;
    } );

    if ( it != m_settings.m_accounts.end() )
    {
        QMessageBox msgBox;
        msgBox.setText("Account with same private key\n already exists");
        msgBox.setStandardButtons(QMessageBox::Close);
        msgBox.exec();
        return;
    }

    // Check unique name
    //
    it = std::find_if( m_settings.m_accounts.begin(), m_settings.m_accounts.end(), [&]( const auto& account )
    {
        return account.m_accountName == accountName;
    } );

    if ( it != m_settings.m_accounts.end() )
    {
        QMessageBox msgBox;
        msgBox.setText("Account with same name\n already exists");
        msgBox.setStandardButtons(QMessageBox::Close);
        msgBox.exec();
        return;
    }

    m_settings.m_accounts.emplace_back( Settings::Account{} );
    qDebug() << "m_settings.m_accounts.size()-1: " << m_settings.m_accounts.size()-1;
    m_settings.setCurrentAccountIndex( m_settings.m_accounts.size()-1 );
    m_settings.config().initAccount( accountName, privateKeyStr );

    QDialog::accept();
}

void PrivKeyDialog::reject()
{
    QDialog::reject();
}
