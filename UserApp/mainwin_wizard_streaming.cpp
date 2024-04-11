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
    ui->m_wizardStreamAnnouncementTable->hide();
    ui->m_wizardStartSelectedStreamBtn->hide();
    ui->m_wizardRemoveAnnouncementBtn->hide();

    connect(ui->m_wizardStreamAnnouncementTable->model()
            , &QAbstractItemModel::rowsRemoved, this
            , &MainWin::onRowsRemoved);

    connect(ui->m_wizardStreamAnnouncementTable->model()
            , &QAbstractItemModel::rowsInserted, this
            , &MainWin::onRowsInserted);

    
    connect(ui->m_wizardAddStreamAnnouncementBtn, &QPushButton::released, this, [this] ()
        {
            if ( ui->m_driveCBox->count() == 0 )
            {
                AskDriveWizardDialog dial(ui->m_wizardAddStreamAnnouncementBtn->text(),
                                          m_onChainClient, m_model, this);
                auto rc = dial.exec();
                if (rc == QMessageBox::Ok)
                {
                    AddDriveDialog dialog(m_onChainClient,
                                          m_model,
                                          this,
                                          [this](std::string driveHash)
                    {
                        m_wizardAddStreamAnnounceDialog
                          = new WizardAddStreamAnnounceDialog(m_onChainClient,
                                                              m_model,
                                                              driveHash,
                                                              this);
                        m_wizardAddStreamAnnounceDialog->exec();
                        delete m_wizardAddStreamAnnounceDialog;
                        m_wizardAddStreamAnnounceDialog = nullptr;
                    });
                    dialog.exec();
                }
            }
            else
            {
                m_wizardAddStreamAnnounceDialog
                    = new WizardAddStreamAnnounceDialog(m_onChainClient,
                                                        m_model,
                                                        m_model->getDrives().begin()->first,
                                                        this);
                m_wizardAddStreamAnnounceDialog->exec();
                delete m_wizardAddStreamAnnounceDialog;
                m_wizardAddStreamAnnounceDialog = nullptr;
            }
        }, Qt::QueuedConnection);


    connect(ui->m_wizardStartSelectedStreamBtn, &QPushButton::released, this, [this] ()
        {
            if( bool isInstalled = checkObsState(); !isInstalled )
            {
                displayError( "OBS not installed in system" );
                return;
            }

            if( bool profileExists = checkObsProfileState(); !profileExists )
            {
                displayError( "There is no Sirius-stream OBS profile" );
                return;
            }

            if ( StreamInfo* streamInfo = wizardSelectedStreamInfo(); streamInfo == nullptr )
            {
                displayError( "Select stream!" );
                return;
            }

            std::string driveKey = selectedDriveKeyInWizardTable().toStdString();
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

            if ( StreamInfo* streamInfo = wizardSelectedStreamInfo(); streamInfo != nullptr )
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
            StreamInfo* streamInfo = wizardSelectedStreamInfo();
            if( streamInfo == nullptr)
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
                auto* drive = selectedDriveInWizardTable();

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

QString MainWin::selectedDriveKeyInWizardTable() const
{
    auto rowList = ui->m_wizardStreamAnnouncementTable->selectionModel()->selectedRows();
    if ( rowList.count() > 0 )
    {
        return ui->m_wizardStreamAnnouncementTable->item( rowList.first().row(), 2 )->data(Qt::UserRole).toString();
    }
    return "";
}

Drive* MainWin::selectedDriveInWizardTable() const
{
    auto driveKey = selectedDriveKeyInWizardTable().toStdString();
    Drive* drive = m_model->findDrive( driveKey );
    if ( drive == nullptr && (ui->m_streamDriveCBox->count() == 0) )
    {
        qWarning() << LOG_SOURCE << "! internal error: bad drive key";
    }
    return drive;
}

void MainWin::wizardUpdateStreamerTable( const Drive& drive )
{
    wizardReadStreamingAnnotations();

    qDebug() << "announcements: " << m_model->getStreams().size();

    while( ui->m_wizardStreamAnnouncementTable->rowCount() > 0 )
    {
        ui->m_wizardStreamAnnouncementTable->removeRow(0);
    }

    for( const auto& streamInfo: m_model->getStreams() )
    {
        if ( streamInfo.m_streamingStatus == StreamInfo::ss_finished )
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


StreamInfo* MainWin::wizardSelectedStreamInfo() const
{
    auto rowList = ui->m_wizardStreamAnnouncementTable->selectionModel()->selectedRows();
    if ( rowList.count() > 0 )
    {
        try
        {
            int rowIndex = rowList.constFirst().row();
            return &m_model->getStreams().at(rowIndex);
        }
        catch (...) {}
    }
    return nullptr;
}

void MainWin::wizardReadStreamingAnnotations()
{
    bool todoShouldBeSync = false;
    std::vector<StreamInfo>& streamInfoVector = m_model->getStreams();

    streamInfoVector.clear();

    auto drives = m_model->getDrives();
    for( auto& driveInfo : drives )
    {

        // read from disc (.videos/*/info)
        {
            auto path = fs::path( driveInfo.second.getLocalFolder() + "/" + STREAM_ROOT_FOLDER_NAME);

            std::error_code ec;
            if ( ! fs::is_directory(path,ec) )
            {
                return;
            }

            for( const auto& entry : fs::directory_iterator( path ) )
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
                            it->m_streamingStatus = StreamInfo::ss_created;
                        }
                        else
                        {
                            todoShouldBeSync = true;
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

        auto it = std::find_if( streamInfoVector.begin(), streamInfoVector.end(), [] (const StreamInfo& streamInfo) {
            return streamInfo.m_streamingStatus == StreamInfo::ss_regestring;
        });

        todoShouldBeSync = (it != streamInfoVector.end());
    }
}

void MainWin::onRowsRemoved()
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

void MainWin::onRowsInserted()
{
    ui->m_wizardStreamAnnouncementTable->show();
    ui->m_wizardStartSelectedStreamBtn->show();
    ui->m_wizardRemoveAnnouncementBtn->show();
}

bool MainWin::checkObsState()
{
    QProcess process;
    process.start("bash", QStringList() << "-c" << "which obs");
    process.waitForFinished();
    QString output = process.readAllStandardOutput();

    if(output.isEmpty())
    {
        qDebug() << "OBS is not installed.";
        return false;
    }

    qDebug() << "OBS is installed at path: " << output.trimmed();
    return true;
}

bool MainWin::checkObsProfileState()
{
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    qDebug() << "Home directory of the current user is: " << homeDir;
    QString dirName = homeDir + "/.config/obs-studio/basic/profiles/Siriusstream";
    QDir directory(dirName);

    if(!directory.exists())
    {
        qDebug() << "The 'Sirius-stream' profile does not exist in OBS.";
        return false;
    }

    qDebug() << "The 'Sirius-stream' profile exists in OBS.";
    return true;
}
