#include "Settings.h"
#include "AddDriveDialog.h"
#include "./ui_AddDriveDialog.h"

//#include "drive/Utils.h"

#include <QIntValidator>
//#include <QMessageBox>

//#include <boost/algorithm/string.hpp>
//#include <boost/asio.hpp>


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
        ui->m_errorText->setText("");
    });

    connect( ui->m_replicatorNumber, &QLineEdit::textChanged, this, [this] ()
    {
        ui->m_errorText->setText("");
    });

    connect( ui->m_maxDriveSize, &QLineEdit::textChanged, this, [this] ()
    {
        ui->m_errorText->setText("");
    });

    connect( ui->m_localDriveFolder, &QLineEdit::textChanged, this, [this] ()
    {
        ui->m_errorText->setText("");
    });

    connect( ui->m_key, &QLineEdit::textChanged, this, [this] ()
    {
        ui->m_errorText->setText("");
    });


    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddDriveDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AddDriveDialog::reject);

    ui->m_replicatorNumber->setValidator( new QIntValidator( 5, 20, this) );
    ui->m_replicatorNumber->setText( QString::fromStdString( "5" ));

    ui->m_maxDriveSize->setValidator( new QIntValidator( 1, 100*1024, this) );
    ui->m_maxDriveSize->setText( QString::fromStdString( "100" ));

    setWindowTitle("Create Drive");
    setFocus();
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
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    const auto& drives = gSettings.config().m_drives;

    // check drive name
    auto it = std::find_if( drives.begin(), drives.end(), [name = ui->m_driveName->text().toStdString()] (const auto& driveInfo)
    {
        return driveInfo.m_name == name;
    });
    if ( it != drives.end() )
    {
        ui->m_errorText->setText( QString::fromStdString("Drive with the same name already exists" ));
        return false;
    }

    // check local folder path
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
    }

    if ( ! fs::is_directory( localFolderPath, ec ) )
    {
        ui->m_errorText->setText( QString::fromStdString("Invalid path of local drive folder" ));
        return false;
    }

    return true;
}

