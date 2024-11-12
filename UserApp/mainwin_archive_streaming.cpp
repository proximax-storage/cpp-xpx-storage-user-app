#include "Dialogs/AddDriveDialog.h"
#include "Entities/Account.h"
#include "Models/Model.h"
#include "mainwin.h"
#include "./ui_mainwin.h"
#include <QDebug>
#include "Entities/StreamInfo.h"

void MainWin::initWizardArchiveStreaming()
{
    ui->m_wizardArchiveTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->m_wizardArchiveTable->setSelectionMode( QAbstractItemView::SingleSelection );

    ui->m_wizardArchiveTable->hide();
    ui->m_deleteArchivedStreamBtn->hide();

    connect(ui->m_wizardArchiveTable->model()
            , &QAbstractItemModel::rowsRemoved, this
            , &MainWin::onRowsRemovedArchive);

    connect(ui->m_wizardArchiveTable->model()
            , &QAbstractItemModel::rowsInserted, this
            , &MainWin::onRowsInsertedArchive);

    connect(ui->m_deleteArchivedStreamBtn, &QPushButton::released, this, [this] ()
        {
            auto streamInfo = wizardSelectedStreamInfo(ui->m_wizardArchiveTable);
            if( !streamInfo )
            {
                displayError( "Select stream!" );
                return;
            }

            if ( auto rowList = ui->m_wizardArchiveTable->selectionModel()->selectedRows();
                rowList.count() == 0 )
            {
                displayError( "No announcements!" );
                return;
            }

            QMessageBox msgBox;
            const QString message = "'" + streamInfo->m_title + "' will be removed.";
            msgBox.setText(message);
            msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
            auto reply = msgBox.exec();

            if ( reply == QMessageBox::Ok )
            {
                auto* drive = selectedDriveInWizardTable(ui->m_wizardArchiveTable);

                if ( drive != nullptr )
                {
                    auto streamFolder = fs::path(qStringToStdStringUTF8(drive->getLocalFolder() + "/" + STREAM_ROOT_FOLDER_NAME + "/" + streamInfo->m_uniqueFolderName));
                    std::error_code ec;
                    fs::remove_all( streamFolder, ec );
                    qWarning() << "remove: " << streamFolder.string().c_str() << " ec:" << ec.message().c_str();

                    if ( !ec )
                    {
                        sirius::drive::ActionList actionList;
                        auto streamFolder = fs::path(qStringToStdStringUTF8(QString(STREAM_ROOT_FOLDER_NAME) + "/" + streamInfo->m_uniqueFolderName));
                        actionList.push_back( sirius::drive::Action::remove( streamFolder.string() ) );
                        //
                        // Start modification
                        //
                        auto confirmationCallback = [](auto fee)
                        {
                            return showConfirmationDialog(fee);
                        };

                        auto driveKeyHex = rawHashFromHex(drive->getKey());
                        m_onChainClient->applyDataModification(driveKeyHex, actionList, confirmationCallback);
                        drive->updateDriveState(registering);
                    }
                }
            }
        }, Qt::QueuedConnection);

}

void MainWin::wizardUpdateArchiveTable()
{
    auto streamInfoList = wizardReadStreamInfoList();

    qDebug() << "announcements: " << streamInfoList.size();

    while( ui->m_wizardArchiveTable->rowCount() > 0 )
    {
        ui->m_wizardArchiveTable->removeRow(0);
    }

    for( const auto& streamInfo: streamInfoList )
    {
        if ( streamInfo.m_streamingStatus != StreamInfo::ss_finished )
        {
            continue;
        }

        size_t rowIndex = ui->m_wizardArchiveTable->rowCount();
        ui->m_wizardArchiveTable->insertRow( (int)rowIndex );
        {
            auto* item = new QTableWidgetItem();
            item->setData( Qt::DisplayRole, streamInfo.m_title );
            ui->m_wizardArchiveTable->setItem( (int)rowIndex, 0, item );
        }
        {
            auto* item = new QTableWidgetItem( m_model->findDrive(streamInfo.m_driveKey)->getName() );
            item->setData( Qt::UserRole, streamInfo.m_driveKey );
            ui->m_wizardArchiveTable->setItem( (int)rowIndex, 1, item );
        }
    }
}


void MainWin::onRowsRemovedArchive()
{
    if (ui->m_wizardArchiveTable->rowCount() == 0)
    {
        ui->m_wizardArchiveTable->hide();
        ui->m_deleteArchivedStreamBtn->hide();
    }
    else
    {
        ui->m_wizardArchiveTable->show();
        ui->m_deleteArchivedStreamBtn->show();
    }
}

void MainWin::onRowsInsertedArchive()
{
    ui->m_wizardArchiveTable->show();
    ui->m_deleteArchivedStreamBtn->show();
}
