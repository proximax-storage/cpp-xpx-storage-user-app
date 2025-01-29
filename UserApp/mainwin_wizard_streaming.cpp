#include "Dialogs/AddDriveStreamDialog.h"
#include "Dialogs/AskDriveWizardDialog.h"
#include "Dialogs/WizardAddStreamAnnounceDialog.h"
#include "DialogsStreaming/StreamingPanel.h"
#include "Entities/Account.h"
#include "Models/Model.h"
#include "mainwin.h"
#include "./ui_mainwin.h"
//
#include "Entities/StreamInfo.h"
#include "qclipboard.h"

bool MainWin::checkObsConfiguration()
{
    if( !ObsProfileData().isObsInstalled() )
    {
        displayError( "OBS not found in system (maybe not installed)." );
        return false;
    }

    if( !ObsProfileData().isObsProfileAvailable() )
    {
        displayError( "There is no 'Sirius-stream' OBS profile." );
        return false;
    }

    if( !ObsProfileData().isOutputModeCorrect() )
    {
        displayError( "'Output Mode' in 'Sirius-stream' profile must be set to "
                        + ObsProfileData().m_outputMode
                    , "(OBS Settings -> 'Output' tab -> 'Output Mode')" );
        return false;
    }

    if( !ObsProfileData().isSiriusProfileSet() )
    {
        displayError( "The 'Sirius-stream' profile must be set as current OBS profile." );
        return false;
    }

    if( !ObsProfileData().isRecordingPathSet() )
    {
        displayError( "'Recording Path' for stream must be set in 'Sirius-stream' profile."
                     ,"(OBS Settings -> 'Output' tab -> Advanced mode -> "
                      "'Recording' subtab -> 'Recording Path')");
        return false;
    }
    
    return true;
}

void MainWin::initWizardStreaming()
{
    ui->m_wizardStreamAnnouncementTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->m_wizardStreamAnnouncementTable->setSelectionMode( QAbstractItemView::SingleSelection );
    hideWizardUi();

    connect(ui->m_wizardStreamAnnouncementTable->model()
            , &QAbstractItemModel::rowsRemoved, this
            , &MainWin::onRowsRemovedAnnouncements);

    connect(ui->m_wizardStreamAnnouncementTable->model()
            , &QAbstractItemModel::rowsInserted, this
            , &MainWin::onRowsInsertedAnnouncements);

    connect(ui->m_wizardStartStreamNow, &QPushButton::released, this, [this] ()
        {
            if ( !checkObsConfiguration() )
            {
                return;
            }
        
            AddDriveStreamDialog dialog( m_onChainClient,m_model, this );
            auto rc = dialog.exec();
            if (rc == QDialog::Accepted)
            {
                // drive creation is started
                m_streamingPanel->onStartStreamingWoAnnouncement();
                //create drive
                //drive created
                //1-st stream modification
                //modification approved
                // start streaming
            }
        }, Qt::QueuedConnection);
    
    connect(ui->m_wizardAddStreamAnnouncementBtn, &QPushButton::released, this, [this] ()
        {
            if ( ui->m_driveCBox->count() == 0 )
            {
                if ( !checkObsConfiguration() )
                {
                    return;
                }

                AskDriveWizardDialog dial(ui->m_wizardAddStreamAnnouncementBtn->text(),
                                          m_onChainClient, m_model, this);
                auto rc = dial.exec();
                if (rc == QMessageBox::Ok)
                {
                    AddDriveStreamDialog dialog( m_onChainClient,m_model, this );
                    auto rc = dialog.exec();
                    if (rc == QDialog::Accepted)
                    {
                        m_modalDialog = new ModalDialog( "Information", "Drive is creating...", [this]
                        {
                            delete m_modalDialog;
                            m_modalDialog = nullptr;
                            
                            WizardAddStreamAnnounceDialog wizardAddStreamAnnounceDialog( m_onChainClient,
                                                                                         m_model,
                                                                                         m_model->getDrives().begin()->first,
                                                                                         this );
                            wizardAddStreamAnnounceDialog.exec();
                        });
                        m_modalDialog->exec();
                    }
                }
            }
            else
            {
                WizardAddStreamAnnounceDialog wizardAddStreamAnnounceDialog( m_onChainClient,
                                                                             m_model,
                                                                             m_model->getDrives().begin()->first,
                                                                             this );
                wizardAddStreamAnnounceDialog.exec();
            }
        }, Qt::QueuedConnection);

    connect(ui->m_wizardStartSelectedStreamBtn, &QPushButton::released, this, [this] ()
        {
            if ( !checkObsConfiguration() )
            {
                return;
            }

            if ( auto streamInfo = wizardSelectedStreamInfo(ui->m_wizardStreamAnnouncementTable); !streamInfo.has_value() )
            {
                displayError( "Select announcement!" );
                return;
            }

            if ( auto rowList = ui->m_wizardStreamAnnouncementTable->selectionModel()->selectedRows();
                 rowList.count() == 0 )
            {
                displayError( "No announcements!" );
                return;
            }

            const QString driveKey = selectedDriveKeyInWizardTable(ui->m_wizardStreamAnnouncementTable);
            auto* drive = m_model->findDrive(driveKey);
            if ( drive == nullptr )
            {
                qWarning() << LOG_SOURCE << "bad driveKey: " << driveKey;
                displayError( "Cannot found drive: " );
                return;
            }

            if ( drive->isStreaming() )
            {
                qWarning() << LOG_SOURCE << "drive already 'is streaming'";
                displayError( "Current stream is not finished" );
                return;
            }

            if ( auto streamInfo = wizardSelectedStreamInfo(ui->m_wizardStreamAnnouncementTable); streamInfo )
            {
                startStreamingProcess( *streamInfo );
            }
            else
            {
                qWarning() << LOG_SOURCE << "Internal error: no streaInfo";
                displayError( "Internal error: no streaInfo" );
                return;
            }
        }, Qt::QueuedConnection);

    connect(ui->m_wizardRemoveAnnouncementBtn, &QPushButton::released, this, [this] ()
        {
            auto streamInfo = wizardSelectedStreamInfo(ui->m_wizardStreamAnnouncementTable);
            if (!streamInfo.has_value() )
            {
                displayError( "Select announcement!" );
                return;
            }

            if ( auto rowList = ui->m_wizardStreamAnnouncementTable->selectionModel()->selectedRows();
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
                auto* drive = selectedDriveInWizardTable(ui->m_wizardStreamAnnouncementTable);

                if ( drive != nullptr )
                {
                    auto streamFolder = fs::path(qStringToStdStringUTF8(drive->getLocalFolder()) + "/" + STREAM_ROOT_FOLDER_NAME + "/" + qStringToStdStringUTF8(streamInfo->m_uniqueFolderName) );
                    std::error_code ec;
                    fs::remove_all( streamFolder, ec );
                    qWarning() << "remove: " << streamFolder.string().c_str() << " ec:" << ec.message().c_str();

                    if ( !ec )
                    {
                        sirius::drive::ActionList actionList;
                        auto streamFolder = fs::path( std::string(STREAM_ROOT_FOLDER_NAME) + "/" + qStringToStdStringUTF8(streamInfo->m_uniqueFolderName));
                        actionList.push_back( sirius::drive::Action::remove( streamFolder.string() ) );

                        //
                        // Start modification
                        //
                        auto confirmationCallback = [&drive,this](auto fee)
                        {
                            if (showConfirmationDialog(fee)) {
                                updateDriveState(drive,registering);
                                return true;
                            }

                            return false;
                        };

                        auto driveKeyHex = rawHashFromHex(drive->getKey());
                        m_onChainClient->applyDataModification(driveKeyHex, actionList, confirmationCallback);
                    }
                }
            }
        }, Qt::QueuedConnection);

    connect(ui->m_wizardCopyStreamLinkBtn, &QPushButton::released, this, [this] ()
        {
            auto streamInfo = wizardSelectedStreamInfo(ui->m_wizardStreamAnnouncementTable);
            if (!streamInfo.has_value() )
            {
                displayError( "Select announcement!" );
                return;
            }

            if ( auto rowList = ui->m_wizardStreamAnnouncementTable->selectionModel()->selectedRows();
                rowList.count() == 0 )
            {
                displayError( "No announcements!" );
                return;
            }

            QString link = streamInfo->getLink();
            QClipboard* clipboard = QApplication::clipboard();
            if ( !clipboard ) {
                qWarning() << LOG_SOURCE << "bad clipboard";
                return;
            }

            clipboard->setText( link, QClipboard::Clipboard );
            if ( clipboard->supportsSelection() ) {
                clipboard->setText( link, QClipboard::Selection );
            }

        }, Qt::QueuedConnection);

}

QString MainWin::selectedDriveKeyInWizardTable(QTableWidget* table) const
{
    auto rowList = table->selectionModel()->selectedRows();
    if ( rowList.count() > 0 )
    {
        int columnCount = table->columnCount();
        int driveColumn = 0;
        for (int column = 0; column < columnCount; ++column)
        {
            QTableWidgetItem* headerItem = table->horizontalHeaderItem(column);
            if (headerItem && headerItem->text() == "Drive")
            {
                driveColumn = column;
            }
        }
        return table->item( rowList.first().row(), driveColumn )->data(Qt::UserRole).toString();
    }
    return "";
}

Drive* MainWin::selectedDriveInWizardTable(QTableWidget* table) const
{
    auto driveKey = selectedDriveKeyInWizardTable(table);
    Drive* drive = m_model->findDrive( driveKey );
    if ( drive == nullptr && (ui->m_streamDriveCBox->count() == 0) )
    {
        qWarning() << LOG_SOURCE << "! internal error: bad drive key";
    }
    return drive;
}

void MainWin::wizardUpdateStreamAnnouncementTable()
{
    auto drives = m_model->getDrives();
    std::vector<StreamInfo> streamInfoList;
    for( auto& driveInfo : drives )
    {
        auto list = readStreamInfoList(driveInfo.second);
        streamInfoList.insert( streamInfoList.end(), list.begin(), list.end() );
    }

    qDebug() << "announcements: " << streamInfoList.size();

    while( ui->m_wizardStreamAnnouncementTable->rowCount() > 0 )
    {
        ui->m_wizardStreamAnnouncementTable->removeRow(0);
    }

    for( const auto& streamInfo: streamInfoList )
    {
        if ( streamInfo.m_streamingStatus != StreamInfo::ss_announced )
        {
            continue;
        }

        size_t rowIndex = ui->m_wizardStreamAnnouncementTable->rowCount();
        ui->m_wizardStreamAnnouncementTable->insertRow( (int)rowIndex );
        {
            QDateTime dateTime = QDateTime::currentDateTime();
            dateTime.setSecsSinceEpoch( streamInfo.m_secsSinceEpoch );
            ui->m_wizardStreamAnnouncementTable->setItem( (int)rowIndex, 0, new QTableWidgetItem( dateTime.toString() ) );
        }
        {
            auto* item = new QTableWidgetItem();
            item->setData( Qt::DisplayRole, streamInfo.m_title );
            ui->m_wizardStreamAnnouncementTable->setItem( (int)rowIndex, 1, item );
        }
        {
            if ( m_model->findDrive(streamInfo.m_driveKey) != nullptr )
            {
                auto* item = new QTableWidgetItem( m_model->findDrive(streamInfo.m_driveKey)->getName());
                item->setData( Qt::UserRole, streamInfo.m_driveKey );
                ui->m_wizardStreamAnnouncementTable->setItem( (int)rowIndex, 2, item );
            }
        }
    }
}

std::optional<StreamInfo> MainWin::wizardSelectedStreamInfo(QTableWidget* table)
{
    auto rowList = table->selectionModel()->selectedRows();
    if ( rowList.count() > 0 )
    {
        try
        {
            int rowIndex = rowList.constFirst().row();
            int columnIndex = -1;
            for (int i = 0; i < table->columnCount(); ++i) {
                QString columnName = table->horizontalHeaderItem(i)->text();
                if (columnName == "Stream Title") {
                    columnIndex = i;
                    break;
                }
            }

            QString streamTitle = table->item(rowIndex, columnIndex)->text(); // stream name
            auto streamInfoList = wizardReadStreamInfoList();
            for(auto& e : streamInfoList)
            {
                if(e.m_title == streamTitle)
                {
                    return e;
                }
            }
        }
        catch (...) {}
    }
    return {};
}

std::vector<StreamInfo> MainWin::wizardReadStreamInfoList()
{
    std::vector<StreamInfo> streamInfoVector;

    auto drives = m_model->getDrives();
    for( auto& driveInfo : drives )
    {
        auto infoVector = readStreamInfoList(driveInfo.second);
        streamInfoVector.insert(streamInfoVector.end(), infoVector.begin(), infoVector.end());
    }
    return streamInfoVector;
}

void MainWin::hideWizardUi()
{
    ui->m_wizardStreamAnnouncementTable->hide();
    ui->m_wizardStartSelectedStreamBtn->hide();
    ui->m_wizardRemoveAnnouncementBtn->hide();
    ui->m_wizardCopyStreamLinkBtn->hide();
}

void MainWin::onRowsRemovedAnnouncements()
{
    if (ui->m_wizardStreamAnnouncementTable->rowCount() == 0)
    {
        hideWizardUi();
    }
    else
    {
        onRowsInsertedAnnouncements();
    }
}

void MainWin::onRowsInsertedAnnouncements()
{
    ui->m_wizardStreamAnnouncementTable->show();
    ui->m_wizardStartSelectedStreamBtn->show();
    ui->m_wizardRemoveAnnouncementBtn->show();
    ui->m_wizardCopyStreamLinkBtn->show();
}
