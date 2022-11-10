#include "PrivKeyDialog.h"
#include "Settings.h"
#include "mainwin.h"
#include "ui_PrivKeyDialog.h"
#include "drive/Utils.h"

#include <random>
#include <cstdio>

#include <QMessageBox>
#include <QFileDialog>
#include <QToolTip>

#include <QtGlobal>
#if QT_VERSION >= 0x050000
#include <QRegularExpression>
#else
#include <QRegExp>
#endif

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

#if QT_VERSION >= 0x050000
    QRegularExpression nameTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9_]{1,40})")));
#else
    QRegExp nameTemplate(R"([a-zA-Z0-9_]{1,40})");
    nameTemplate.setCaseSensitivity(Qt::CaseInsensitive);
    nameTemplate.setPatternSyntax(QRegExp::RegExp);
#endif

    connect(ui->m_accountName, &QLineEdit::textChanged, this, [this, nameTemplate] (auto text)
    {
#if QT_VERSION >= 0x050000
        if (!nameTemplate.match(text).hasMatch()) {
#else
        if (!nameTemplate.exactMatch(text)) {
#endif
            QToolTip::showText(ui->m_accountName->mapToGlobal(QPoint()), tr("Invalid name!"));
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_accountName->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_accountName->setProperty("is_valid", true);
            validate();
        }
    });

#if QT_VERSION >= 0x050000
    QRegularExpression keyTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9]{64})")));
#else
    QRegExp keyTemplate(R"([a-zA-Z0-9]{64})");
    keyTemplate.setCaseSensitivity(Qt::CaseInsensitive);
    keyTemplate.setPatternSyntax(QRegExp::RegExp);
#endif

    connect(ui->m_pkField, &QLineEdit::textChanged, this, [this, keyTemplate] (auto text)
    {
#if QT_VERSION >= 0x050000
        if (!keyTemplate.match(text).hasMatch()) {
#else
        if (!keyTemplate.exactMatch(text)) {
#endif
            QToolTip::showText(ui->m_pkField->mapToGlobal(QPoint()), tr("Invalid key!"));
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_pkField->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_pkField->setProperty("is_valid", true);
            validate();
        }
    });

#if QT_VERSION >= 0x050000
    if (!keyTemplate.match(ui->m_accountName->text()).hasMatch()) {
#else
    if (!nameTemplate.exactMatch(ui->m_accountName->text())) {
#endif
        QToolTip::showText(ui->m_accountName->mapToGlobal(QPoint()), tr("Invalid name!"));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_accountName->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_accountName->setProperty("is_valid", true);
        validate();
    }

#if QT_VERSION >= 0x050000
    if (!keyTemplate.match(ui->m_pkField->text()).hasMatch()) {
#else
    if (!keyTemplate.exactMatch(ui->m_pkField->text())) {
#endif
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->m_pkField->setProperty("is_valid", false);
        QToolTip::showText(ui->m_pkField->mapToGlobal(QPoint()), tr("Invalid key!"));
    } else {
        QToolTip::hideText();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->m_pkField->setProperty("is_valid", true);
        validate();
    }
}

PrivKeyDialog::~PrivKeyDialog()
{
    delete ui;
}

void PrivKeyDialog::validate() {
    if (ui->m_accountName->property("is_valid").toBool() &&
        ui->m_pkField->property("is_valid").toBool() &&
        !isAccountExists()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}

bool PrivKeyDialog::isAccountExists() {
    auto nameIterator = std::find_if( m_settings.m_accounts.begin(), m_settings.m_accounts.end(), [this]( auto account )
    {
        return account.m_accountName == ui->m_accountName->text().toStdString();
    });

    if ( nameIterator != m_settings.m_accounts.end() )
    {
        QToolTip::showText(ui->m_accountName->mapToGlobal(QPoint()), tr("Account with same name already exists!"));
        return true;
    }

    auto keyIterator = std::find_if( m_settings.m_accounts.begin(), m_settings.m_accounts.end(), [this]( auto account )
    {
        return account.m_privateKeyStr == ui->m_pkField->text().toStdString();
    });

    if ( keyIterator != m_settings.m_accounts.end() )
    {
        QToolTip::showText(ui->m_pkField->mapToGlobal(QPoint()), tr("Account with same private key already exists!"));
        return true;
    }

    return false;
}

void PrivKeyDialog::onGenerateKeysBtn()
{
    std::random_device   dev;
    std::seed_seq        seed({dev(), dev(), dev(), dev()});
    std::mt19937         rng(seed);

    std::array<uint8_t,32> buffer{};

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
        std::array<uint8_t,32> buffer{};
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
    qDebug() << LOG_SOURCE << "m_settings.m_accounts.size()-1: " << m_settings.m_accounts.size()-1;
    m_settings.setCurrentAccountIndex( (int)m_settings.m_accounts.size() - 1 );
    m_settings.config().initAccount( accountName, privateKeyStr );

    QDialog::accept();
}

void PrivKeyDialog::reject()
{
    QDialog::reject();
}
