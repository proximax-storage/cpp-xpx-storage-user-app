#include "Settings.h"
#include "ManageDrivesDialog.h"
#include "./ui_ManageDrivesDialog.h"

#include <QIntValidator>

#include <random>

ManageDrivesDialog::ManageDrivesDialog( QWidget *parent, bool initSettings ) :
    QDialog( parent ),
    ui( new Ui::ManageDrivesDialog() )
{

    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Save");

    setModal(true);

    ui->m_errorText->setText("");

    ui->m_table->setColumnCount(2);

    ui->m_table->setShowGrid(true);

    ui->m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    QStringList headers;
       headers.append("name");
       headers.append("max size");
    ui->m_table->setHorizontalHeaderLabels(headers);
    ui->m_table->horizontalHeader()->setStretchLastSection(true);
    ui->m_table->setEditTriggers(QTableWidget::NoEditTriggers);

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    const auto& drives = gSettings.config().m_drives;

    int i=0;
    for( const auto& drive : drives )
    {
        ui->m_table->insertRow(i);
        ui->m_table->setItem(i,0, new QTableWidgetItem( QString::fromStdString(drive.m_name)));
        auto maxSize = std::to_string( drive.m_maxDriveSize ) + " MB";
        ui->m_table->setItem(i,1, new QTableWidgetItem( QString::fromStdString(maxSize)));
    }

//    // добавим в ячейки данные
//    ui->m_table->insertRow(0);
//    ui->m_table->setItem(0,0, new QTableWidgetItem("Ваши новые тикеты"));
//    ui->m_table->setItem(0,1, new QTableWidgetItem("0"));
//    ui->m_table->insertRow(1);
//    ui->m_table->setItem(1,0, new QTableWidgetItem("-- не завершенные (Менеджер)"));
//    ui->m_table->setItem(1,1, new QTableWidgetItem("0"));
//    ui->m_table->insertRow(2);
//    ui->m_table->setItem(2,0, new QTableWidgetItem("-- не завершенные (Клиент)"));
//    ui->m_table->setItem(2,1, new QTableWidgetItem("0"));
//    ui->m_table->insertRow(3);
//    ui->m_table->setItem(3,0, new QTableWidgetItem("Новых в подразделении"));
//    ui->m_table->setItem(3,1, new QTableWidgetItem("0"));
//    ui->m_table->insertRow(4);
//    ui->m_table->setItem(4,0, new QTableWidgetItem("-- не закрытых"));
//    ui->m_table->setItem(4,1, new QTableWidgetItem("0"));
//    ui->m_table->insertRow(5);
//    ui->m_table->setItem(5,0, new QTableWidgetItem("Заявок на подключения"));
//    ui->m_table->setItem(5,1, new QTableWidgetItem("0"));
    ui->m_table->resizeColumnsToContents();

//    connect( ui->m_driveName, &QLineEdit::textChanged, this, [this] ()
//    {
//        verify();
//    });

//    connect( ui->m_replicatorNumber, &QLineEdit::textChanged, this, [this] ()
//    {
////        ui->m_errorText->setText("");
//    });

//    connect( ui->m_maxDriveSize, &QLineEdit::textChanged, this, [this] ()
//    {
////        ui->m_errorText->setText("");
//    });

//    connect( ui->m_localDriveFolder, &QLineEdit::textChanged, this, [this] ()
//    {
//        verify();
//    });

//    connect( ui->m_key, &QLineEdit::textChanged, this, [this] ()
//    {
//        verify();
//    });


//    ui->buttonBox->disconnect(this);
//    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ManageDrivesDialog::accept);
//    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ManageDrivesDialog::reject);

//    ui->m_replicatorNumber->setValidator( new QIntValidator( 5, 20, this) );
//    ui->m_replicatorNumber->setText( QString::fromStdString( "5" ));

//    ui->m_maxDriveSize->setValidator( new QIntValidator( 1, 100*1024*1024, this) );
//    ui->m_maxDriveSize->setText( QString::fromStdString( "100" ));

//    {
//        std::random_device   dev;
//        std::seed_seq        seed({dev(), dev(), dev(), dev()});
//        std::mt19937         rng(seed);

//        std::array<uint8_t,32> buffer;

//        std::generate( buffer.begin(), buffer.end(), [&]
//        {
//            return std::uniform_int_distribution<std::uint32_t>(0,0xff) ( rng );
//        });

//        ui->m_key->setText( QString::fromStdString( sirius::drive::toString( buffer ) ));

//    }

//    setWindowTitle("Create Drive");
//    setFocus();

//    verify();
}

ManageDrivesDialog::~ManageDrivesDialog()
{
    delete ui;
}

void ManageDrivesDialog::accept()
{
    if ( verify() )
    {
        //TODO
        QDialog::accept();
    }
}

void ManageDrivesDialog::reject()
{
    QDialog::reject();
}

bool ManageDrivesDialog::verify()
{
//    if ( verifyDriveName() && verifyLocalDriveFolder() && verifyKey() )
//    {
//        ui->m_errorText->setText( QString::fromStdString("") );
//        return true;
//    }
    return false;
}
