#include "Settings.h"
#include "AddDriveDialog.h"
#include "ManageChannelsDialog.h"
#include "./ui_ManageChannelsDialog.h"

#include <QIntValidator>
#include <QClipboard>

#include <random>

ManageChannelsDialog::ManageChannelsDialog( QWidget *parent ) :
    QDialog( parent ),
    ui( new Ui::ManageChannelsDialog() )
{

    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Close");

    setModal(true);

    ui->m_errorText->setText("");

    ui->m_table->setColumnCount(2);

    ui->m_table->setShowGrid(true);

    ui->m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    QStringList headers;
       headers.append("name");
       headers.append("drive hash");
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
        auto index = ui->m_table->selectionModel()->currentIndex();

        if ( index.row() >= 0 && index.row() < Model::dnChannels().size() )
        {
            //RenameChannelDialog dialog( this, Model::dnChannels()[index.row()] );
//            if ( dialog.exec() == QDialog::Accepted )
//            {
//                emit updateChannels();
//            }
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

        const auto& channels = gSettings.config().m_dnChannels;

        int i=0;
        for( const auto& channel : channels )
        {
            ui->m_table->insertRow(i);
            ui->m_table->setItem( i,0, new QTableWidgetItem( QString::fromStdString(channel.m_name)) );
            ui->m_table->setItem( i,1, new QTableWidgetItem( QString::fromStdString(channel.m_driveHash)) );
        }

        ui->m_table->resizeColumnsToContents();
        ui->m_table->setSelectionMode( QAbstractItemView::SingleSelection );
        ui->m_table->setSelectionBehavior( QAbstractItemView::SelectRows );
    }
}

ManageChannelsDialog::~ManageChannelsDialog()
{
    delete ui;
}

void ManageChannelsDialog::accept()
{
    if ( verify() )
    {
        //TODO
        QDialog::accept();
    }
}

void ManageChannelsDialog::reject()
{
    QDialog::reject();
}

bool ManageChannelsDialog::verify()
{
//    if ( verifyDriveName() && verifyLocalDriveFolder() && verifyKey() )
//    {
//        ui->m_errorText->setText( QString::fromStdString("") );
//        return true;
//    }
    return false;
}
