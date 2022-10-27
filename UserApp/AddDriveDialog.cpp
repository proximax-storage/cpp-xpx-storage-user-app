#include "Settings.h"
#include "AddDriveDialog.h"
#include "./ui_AddDriveDialog.h"

#include <QIntValidator>

#include <random>

AddDriveDialog::AddDriveDialog( QWidget *parent, bool initSettings ) :
    QDialog( parent ),
    ui( new Ui::AddDriveDialog() )
{

    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Save");

    setModal(true);

    ui->m_errorText->setText("");

    connect( ui->m_driveName, &QLineEdit::textChanged, this, [this] ()
    {
        verify();
    });

    connect( ui->m_replicatorNumber, &QLineEdit::textChanged, this, [this] ()
    {
//        ui->m_errorText->setText("");
    });

    connect( ui->m_maxDriveSize, &QLineEdit::textChanged, this, [this] ()
    {
//        ui->m_errorText->setText("");
    });

    connect( ui->m_localDriveFolder, &QLineEdit::textChanged, this, [this] ()
    {
        verify();
    });

    connect( ui->m_key, &QLineEdit::textChanged, this, [this] ()
    {
        verify();
    });


    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddDriveDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AddDriveDialog::reject);

    ui->m_replicatorNumber->setValidator( new QIntValidator( 5, 20, this) );
    ui->m_replicatorNumber->setText( QString::fromStdString( "5" ));

    ui->m_maxDriveSize->setValidator( new QIntValidator( 1, 100*1024*1024, this) );
    ui->m_maxDriveSize->setText( QString::fromStdString( "100" ));

    {
        std::random_device   dev;
        std::seed_seq        seed({dev(), dev(), dev(), dev()});
        std::mt19937         rng(seed);

        std::array<uint8_t,32> buffer;

        std::generate( buffer.begin(), buffer.end(), [&]
        {
            return std::uniform_int_distribution<std::uint32_t>(0,0xff) ( rng );
        });

        ui->m_key->setText( QString::fromStdString( sirius::drive::toString( buffer ) ));

    }

    setWindowTitle("Create Drive");
    setFocus();

    verify();
}

AddDriveDialog::~AddDriveDialog()
{
    delete ui;
}

void AddDriveDialog::accept()
{
    if ( verify() )
    {
        //TODO
        QDialog::accept();
    }
}

void AddDriveDialog::reject()
{
    QDialog::reject();
}

bool AddDriveDialog::verify()
{
    if ( verifyDriveName() && verifyLocalDriveFolder() && verifyKey() )
    {
        ui->m_errorText->setText( QString::fromStdString("") );
        return true;
    }
    return false;
}

bool AddDriveDialog::verifyDriveName()
{
    auto driveName = ui->m_driveName->text().toStdString();

    if ( driveName.empty() )
    {
        ui->m_errorText->setText( QString::fromStdString("Drive name is not set" ));
        return false;
    }

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    const auto& drives = gSettings.config().m_drives;

    // check drive name
    auto it = std::find_if( drives.begin(), drives.end(), [driveName] (const auto& driveInfo)
    {
        return driveInfo.m_name == driveName;
    });
    if ( it != drives.end() )
    {
        ui->m_errorText->setText( QString::fromStdString("Drive with the same name already exists" ));
        return false;
    }

    return true;
}

bool AddDriveDialog::verifyLocalDriveFolder()
{
    const auto& localFolderPath = ui->m_localDriveFolder->text().toStdString();

    if ( localFolderPath.empty() )
    {
        ui->m_errorText->setText( QString::fromStdString("Local drive folder is not set" ));
        return false;
    }

    std::error_code ec;
    if ( ! fs::exists( localFolderPath, ec ) )
    {
        //TODO?
        ui->m_errorText->setText( QString::fromStdString("Local drive folder not exists" ));
        return false;
    }

    if ( ! fs::is_directory( localFolderPath, ec ) )
    {
        ui->m_errorText->setText( QString::fromStdString("Invalid path of local drive folder" ));
        return false;
    }

    return true;
}

bool AddDriveDialog::verifyKey()
{
    const auto& key = ui->m_key->text().toStdString();

    try
    {
        sirius::crypto::KeyPair::FromPrivate(
            sirius::crypto::PrivateKey::FromString( key ));
    }
    catch(...)
    {
        ui->m_errorText->setText( QString::fromStdString("Invalid drive key" ));
        return false;
    }

    return true;
}

