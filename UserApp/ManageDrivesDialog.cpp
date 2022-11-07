#include "Settings.h"
#include "AddDriveDialog.h"
#include "ManageDrivesDialog.h"
#include "./ui_ManageDrivesDialog.h"

#include <QIntValidator>
#include <QClipboard>

#include <random>

ManageDrivesDialog::ManageDrivesDialog( QWidget *parent ) :
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

    connect(ui->m_addBtn, &QPushButton::released, this, [this] ()
    {
//        AddDriveDialog dialog(this);
//        dialog.exec();
    });

    connect(ui->m_editBtn, &QPushButton::released, this, [this] ()
    {
        auto rows = ui->m_table->selectionModel()->selectedRows();

        if ( rows.size() > 0 )
        {
//            AddDriveDialog dialog( this, rows[0].row() );
//            dialog.exec();
        }
    });

    connect(ui->m_removeBtn, &QPushButton::released, this, [this] () {
        //TODO
    });

    connect(ui->m_copyHashBtn, &QPushButton::released, this, [this] ()
    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        const auto& drives = gSettings.config().m_drives;

        int currentRow = ui->m_table->currentRow();

        qDebug() << LOG_SOURCE << "currentRow: " << currentRow;

        if ( currentRow >= 0 && currentRow < drives.size()  )
        {
            QClipboard* clipboard = QApplication::clipboard();
            clipboard->setText( QString::fromStdString(drives[currentRow].m_driveKey) );
            qDebug() << LOG_SOURCE << "clipboard->setText: " << QString::fromStdString(drives[currentRow].m_driveKey);
        }
    });

    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        const auto& drives = gSettings.config().m_drives;
        qDebug() << LOG_SOURCE << "drives.size: " << drives.size();

        int i=0;
        for( const auto& drive : drives )
        {
            qDebug() << LOG_SOURCE << "driveKey: " << drive.m_driveKey.c_str();
            ui->m_table->insertRow(i);
            ui->m_table->setItem(i,0, new QTableWidgetItem( QString::fromStdString(drive.m_name)));
            ui->m_table->setItem(i,1, new QTableWidgetItem( QString::fromStdString(drive.m_driveKey)));
        }

        ui->m_table->resizeColumnsToContents();
        ui->m_table->setSelectionMode( QAbstractItemView::SingleSelection );
        ui->m_table->setSelectionBehavior( QAbstractItemView::SelectRows );
    }
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
