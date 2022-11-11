#include "Settings.h"
#include "SelectEntityDialog.h"
#include "ui_SelectEntityDialog.h"

SelectEntityDialog::SelectEntityDialog(Entity entity, std::function<void(const QString&)> onSelect, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SelectEntityDialog),
    m_onSelect(onSelect)
{
    ui->setupUi(this);

    setWindowTitle( "Select drive" );

    ui->m_table->setColumnCount(2);
    ui->m_table->setShowGrid(true);

    ui->m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    QStringList headers;
    headers.append("Name");
    headers.append("Key");
    ui->m_table->setHorizontalHeaderLabels(headers);
    ui->m_table->horizontalHeader()->setStretchLastSection(true);
    ui->m_table->setEditTriggers(QTableWidget::NoEditTriggers);

    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        int entitiesCount;
        if (Entity::Channel == entity) {
            const auto& channels = gSettings.config().m_dnChannels;
            entitiesCount = (int)channels.size();
            qDebug() << LOG_SOURCE << "channels.size: " << entitiesCount;

            int i = 0;
            for( const auto& channel : channels )
            {
                qDebug() << LOG_SOURCE << "channelKey: " << channel.m_hash.c_str();
                ui->m_table->insertRow(i);
                ui->m_table->setItem(i,0, new QTableWidgetItem( QString::fromStdString( channel.m_name )));
                ui->m_table->setItem(i,1, new QTableWidgetItem( QString::fromStdString( channel.m_hash )));
            }
        } else {
            const auto& drives = gSettings.config().m_drives;
            entitiesCount = (int)drives.size();
            qDebug() << LOG_SOURCE << "drives.size: " << entitiesCount;

            int i = 0;
            for( const auto& drive : drives )
            {
                qDebug() << LOG_SOURCE << "driveKey: " << drive.m_driveKey.c_str();
                ui->m_table->insertRow(i);
                ui->m_table->setItem(i,0, new QTableWidgetItem( QString::fromStdString( drive.m_name )));
                ui->m_table->setItem(i,1, new QTableWidgetItem( QString::fromStdString( drive.m_driveKey )));
            }
        }

        ui->m_table->resizeColumnsToContents();
        ui->m_table->setSelectionMode( QAbstractItemView::SingleSelection );
        ui->m_table->setSelectionBehavior( QAbstractItemView::SelectRows );

        if ( entitiesCount > 0 )
        {
            ui->m_table->selectRow(0);
        }
    }
}

SelectEntityDialog::~SelectEntityDialog()
{
    delete ui;
}

void SelectEntityDialog::accept()
{
    auto selectedRows = ui->m_table->selectionModel()->selectedRows();
    if ( (int)selectedRows.size() > 0 )
    {
        auto hash = ui->m_table->takeItem( selectedRows[0].row(), 1 )->text();
        m_onSelect( hash );
    }

    QDialog::accept();
}
