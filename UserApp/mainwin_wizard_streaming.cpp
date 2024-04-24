#include "Dialogs/AddDriveDialog.h"
#include "Dialogs/AskDriveWizardDialog.h"
#include "Dialogs/WizardAddStreamAnnounceDialog.h"
#include "Entities/Account.h"
#include "Models/Model.h"
#include "mainwin.h"
#include "./ui_mainwin.h"
//
#include "Entities/StreamInfo.h"
#include "qstandardpaths.h"

void MainWin::initWizardStreaming()
{
    ui->m_wizardStreamAnnouncementTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->m_wizardStreamAnnouncementTable->setSelectionMode( QAbstractItemView::SingleSelection );

    ui->m_wizardStreamAnnouncementTable->hide();

    ui->m_wizardStartSelectedStreamBtn->hide();
    ui->m_wizardRemoveAnnouncementBtn->hide();

    connect(ui->m_wizardStreamAnnouncementTable->model()
            , &QAbstractItemModel::rowsRemoved, this
            , &MainWin::onRowsRemovedAnnouncements);

    connect(ui->m_wizardStreamAnnouncementTable->model()
            , &QAbstractItemModel::rowsInserted, this
            , &MainWin::onRowsInsertedAnnouncements);
    
    connect(ui->m_wizardAddStreamAnnouncementBtn, &QPushButton::released, this, [this] ()
        {
            if ( ui->m_driveCBox->count() == 0 )
            {
                AskDriveWizardDialog dial(ui->m_wizardAddStreamAnnouncementBtn->text(),
                                          m_onChainClient, m_model, this);
                auto rc = dial.exec();
                if (rc == QMessageBox::Ok)
                {
                    AddDriveDialog dialog( m_onChainClient,m_model, this );
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
            if( !ObsProfileData().isObsInstalled() )
            {
                displayError( "OBS not found in system (maybe not installed)." );
                return;
            }

            if( !ObsProfileData().isObsProfileAvailable() )
            {
                displayError( "There is no 'Sirius-stream' OBS profile." );
                return;
            }

            if( !ObsProfileData().isOutputModeCorrect() )
            {
                displayError( "'Output Mode' in 'Sirius-stream' profile must be set to "
                                + ObsProfileData().m_outputMode.toStdString()
                            , "(OBS Settings -> 'Output' tab -> 'Output Mode')" );
                return;
            }

            if( !ObsProfileData().isSiriusProfileSet() )
            {
                displayError( "The 'Sirius-stream' profile must be set as current OBS profile." );
                return;
            }

            if( !ObsProfileData().isRecordingPathSet() )
            {
                displayError( "'Recording Path' for stream must be set in 'Sirius-stream' profile."
                             ,"(OBS Settings -> 'Output' tab -> Advanced mode -> "
                              "'Recording' subtab -> 'Recording Path')");
                return;
            }

            if ( auto streamInfo = wizardSelectedStreamInfo(ui->m_wizardStreamAnnouncementTable); !streamInfo.has_value() )
            {
                displayError( "Select announcement!" );
                return;
            }

            std::string driveKey = selectedDriveKeyInWizardTable(ui->m_wizardStreamAnnouncementTable).toStdString();
            auto* drive = m_model->findDrive(driveKey);
            if ( drive == nullptr )
            {
                qWarning() << LOG_SOURCE << "bad driveKey: " << driveKey.c_str();
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
            if( !streamInfo )
            {
                displayError( "Select stream!" );
                return;
            }

            QMessageBox msgBox;
            const QString message = QString::fromStdString("'" + streamInfo->m_title + "' will be removed.");
            msgBox.setText(message);
            msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
            auto reply = msgBox.exec();

            if ( reply == QMessageBox::Ok )
            {
                auto* drive = selectedDriveInWizardTable(ui->m_wizardStreamAnnouncementTable);

                if ( drive != nullptr )
                {
                    auto streamFolder = fs::path( drive->getLocalFolder() + "/" + STREAM_ROOT_FOLDER_NAME + "/" + streamInfo->m_uniqueFolderName );
                    std::error_code ec;
                    fs::remove_all( streamFolder, ec );
                    qWarning() << "remove: " << streamFolder.string().c_str() << " ec:" << ec.message().c_str();

                    if ( !ec )
                    {
                        sirius::drive::ActionList actionList;
                        auto streamFolder = fs::path( std::string(STREAM_ROOT_FOLDER_NAME) + "/" + streamInfo->m_uniqueFolderName);
                        actionList.push_back( sirius::drive::Action::remove( streamFolder.string() ) );

                        //
                        // Start modification
                        //
                        auto driveKeyHex = rawHashFromHex(drive->getKey().c_str());
                        m_onChainClient->applyDataModification(driveKeyHex, actionList);
                        drive->updateDriveState(registering);
                    }
                }
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
    auto driveKey = selectedDriveKeyInWizardTable(table).toStdString();
    Drive* drive = m_model->findDrive( driveKey );
    if ( drive == nullptr && (ui->m_streamDriveCBox->count() == 0) )
    {
        qWarning() << LOG_SOURCE << "! internal error: bad drive key";
    }
    return drive;
}

void MainWin::wizardUpdateStreamAnnouncementTable()
{
    auto streamInfoList = wizardReadStreamInfoList();

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
            item->setData( Qt::DisplayRole, QString::fromStdString( streamInfo.m_title ) );
            ui->m_wizardStreamAnnouncementTable->setItem( (int)rowIndex, 1, item );
        }
        {
            auto* item = new QTableWidgetItem( QString::fromStdString( m_model->findDrive(streamInfo.m_driveKey)->getName() ));
            item->setData( Qt::UserRole, QString::fromStdString( streamInfo.m_driveKey ) );
            ui->m_wizardStreamAnnouncementTable->setItem( (int)rowIndex, 2, item );
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
            QString streamTitle = table->item(rowIndex, 0)->text(); // stream name
            auto streamInfoList = wizardReadStreamInfoList();
            for(auto& e : streamInfoList)
            {
                if(e.m_title == streamTitle.toStdString())
                {
                    return e;
                }
            }
            // int rowIndex = rowList.constFirst().row();
            // return streamInfoList.at(rowIndex);
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
    // read from disc (.videos/*/info)
    {
        auto path = fs::path( driveInfo.second.getLocalFolder() + "/" + STREAM_ROOT_FOLDER_NAME);

        std::error_code ec;
        if ( ! fs::is_directory(path,ec) )
        {
            return streamInfoVector;
        }

        for( const auto& entry : std::filesystem::directory_iterator( path ) )
        {
            const auto entryName = entry.path().filename().string();
            if ( entry.is_directory() )
            {
                auto fileName = fs::path(path.string() + "/" + entryName + "/" + STREAM_INFO_FILE_NAME);
                try
                {
                    std::ifstream is( fileName, std::ios::binary );
                    cereal::PortableBinaryInputArchive iarchive(is);
                    StreamInfo streamInfo;
                    iarchive( streamInfo );
                    streamInfoVector.push_back( streamInfo );
                }
                catch (...) {
                    qWarning() << "Internal error: cannot read stream annotation: " << fileName.c_str();
                }
            }
        }
    }

    // read from FsTree
    {
        auto fsTree = driveInfo.second.getFsTree();
        auto* folder = fsTree.getFolderPtr( STREAM_ROOT_FOLDER_NAME );
        if ( folder != nullptr )
        {
            const auto& streamFolders = folder->childs();
            for( const auto& [key,child] : streamFolders )
            {
                if ( sirius::drive::isFolder(child) )
                {
                    const auto& streamFolder = sirius::drive::getFolder( child );
                    auto it = std::find_if( streamInfoVector.begin()
                                           , streamInfoVector.end()
                                           , [&streamFolder] (const StreamInfo& streamInfo)
                                           {
                                               return streamFolder.name() == streamInfo.m_uniqueFolderName;
                                           });
                    if ( it != streamInfoVector.end() )
                    {
                        auto* child = fsTree.getEntryPtr( STREAM_ROOT_FOLDER_NAME "/" + streamFolder.name() + "/HLS/deleted" );
                        if ( child!=nullptr )
                        {
                            if ( sirius::drive::isFile(*child) )
                            {
                                it->m_streamingStatus = StreamInfo::ss_deleted;
                            }
                            else
                            {
                                displayError( "Internal error: " + streamFolder.name() + "'HLS/deleted' is a folder", "Must be a file." );
                            }
                        }
                        else
                        {
                            fsTree.dbgPrint();
                            qDebug() << QString::fromStdString( STREAM_ROOT_FOLDER_NAME "/" + streamFolder.name() + "/HLS/" PLAYLIST_FILE_NAME );
                            auto* child = fsTree.getEntryPtr( STREAM_ROOT_FOLDER_NAME "/" + streamFolder.name() + "/HLS/" PLAYLIST_FILE_NAME );
                            if ( child!=nullptr )
                            {
                                if ( sirius::drive::isFile(*child) )
                                {
                                    it->m_streamingStatus = StreamInfo::ss_finished;
                                }
                                else
                                {
                                    displayError( "Internal error: " + streamFolder.name() + "'HLS/" PLAYLIST_FILE_NAME "' is a folder", "Must be a file." );
                                }
                            }
                            else
                            {
                                it->m_streamingStatus = StreamInfo::ss_announced;
                            }
                        }
                    }
                }
            }
        }
    }

    std::sort( streamInfoVector.begin(), streamInfoVector.end(),
              [] ( const StreamInfo& a, const StreamInfo& b ) -> bool
              {
                  return a.m_secsSinceEpoch > b.m_secsSinceEpoch;
              });
    }
    return streamInfoVector;
}

void MainWin::onRowsRemovedAnnouncements()
{
    if (ui->m_wizardStreamAnnouncementTable->rowCount() == 0)
    {
        ui->m_wizardStreamAnnouncementTable->hide();
        ui->m_wizardStartSelectedStreamBtn->hide();
        ui->m_wizardRemoveAnnouncementBtn->hide();
    }
    else
    {
        ui->m_wizardStreamAnnouncementTable->show();
        ui->m_wizardRemoveAnnouncementBtn->show();
    }
}

void MainWin::onRowsInsertedAnnouncements()
{
    ui->m_wizardStreamAnnouncementTable->show();
    ui->m_wizardStartSelectedStreamBtn->show();
    ui->m_wizardRemoveAnnouncementBtn->show();
}
