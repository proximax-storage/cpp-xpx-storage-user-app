#include "mainwin.h"
#include "./ui_mainwin.h"

#include "Dialogs/AddStreamAnnouncementDialog.h"
#include "Dialogs/AddDownloadChannelDialog.h"

#include "Models/Model.h"
#include "Entities/StreamInfo.h"
#include "Dialogs/ModifyProgressPanel.h"
#include "Dialogs/StreamingView.h"
#include "Dialogs/StreamingPanel.h"
#include "Dialogs/ModalDialog.h"
#include "Engines/StorageEngine.h"
#include "Entities/Account.h"
#include <qdebug.h>
#include <QFileIconProvider>
#include <QScreen>
#include <QComboBox>
#include <QProcess>
#include <QMessageBox>
#include <QDesktopServices>
#include <thread>
#include <QListWidget>
#include <QAction>
#include <QToolTip>
#include <QFileDialog>
#include <QProcess>
#include <QClipboard>

#include <boost/algorithm/string.hpp>

// Audio clicks / cracklings with ffmpeg audio recording on mac os
// https://stackoverflow.com/questions/66691504/audio-clicks-cracklings-with-ffmpeg-audio-recording-on-mac-os
// https://evermeet.cx/pub/ffmpeg/ffmpeg-4.2.3.zip
//
// ./ffmpeg -f avfoundation -i ":0" test-output.aiff
// ./ffmpeg -f avfoundation -framerate 30 -r 30 -pixel_format uyvy422 -i "0:0" -c:v h264 -c:a aac -hls_time 10 -hls_list_size 0 -hls_segment_filename "output_%03d.ts" output.m3u8

//todo - now we do not use ffmpeg
const bool gUseFfmpeg = false;

QString MainWin::selectedDriveKeyInTable() const
{
    return ui->m_streamDriveCBox->currentData().toString();
}

Drive* MainWin::selectedDriveInTable() const
{
    auto driveKey = ui->m_streamDriveCBox->currentData().toString().toStdString();
    Drive* drive = m_model->findDrive( driveKey );
    if ( drive == nullptr && (ui->m_streamDriveCBox->count() == 0) )
    {
        qWarning() << LOG_SOURCE << "! internal error: bad drive key";
    }
    return drive;
}

Drive* MainWin::currentStreamingDrive() const
{
    return m_model->currentStreamingDrive();
}

std::optional<StreamInfo> MainWin::selectedStreamInfo()
{
    auto rowList = ui->m_wizardStreamAnnouncementTable->selectionModel()->selectedRows();
    if ( rowList.count() > 0 )
    {
        try
        {
            const auto driveKey = ui->m_streamDriveCBox->currentData().toString();
            Drive* drive = m_model->findDriveByNameOrPublicKey( driveKey.toStdString() );
            auto streamInfoList = readStreamingAnnotations(*drive);
            int rowIndex = rowList.constFirst().row();
            return streamInfoList.at(rowIndex);
        }
        catch (...) {}
    }
    return {};
}

void MainWin::initStreaming()
{
    //todo remove streamerAnnouncements from serializing
    m_model->getStreams().clear();

    // m_streamingTabView
    connect(ui->m_streamingTabView, &QTabWidget::currentChanged, this, [this](int index)
    {
        updateViewerProgressPanel( ui->tabWidget->currentIndex() );
        updateStreamerProgressPanel( ui->tabWidget->currentIndex() );

        if (auto drive = currentStreamingDrive();
            drive && ui->tabWidget->currentIndex() == 5 )
            //&& (ui->m_streamingTabView->currentIndex() == 1 || ui->m_streamingTabView->currentIndex() == 2 ) )
        {
            updateDriveWidgets( drive->getKey(), drive->getState(), false );
        }
        else
        {
            m_modifyProgressPanel->setVisible(false);
            m_streamingProgressPanel->setVisible(false);
        }
    });

//    QHeaderView* header = ui->m_streamAnnouncementTable->horizontalHeader();
//    header->setSectionResizeMode(0,QHeaderView::Stretch);
//    header->setSectionResizeMode(1,QHeaderView::Stretch);
//    header->setSectionResizeMode(2,QHeaderView::Stretch);


    // m_streamDriveCBox
    connect( ui->m_streamDriveCBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, [this](int index)
    {
        if ( index >= 0 )
        {
            const auto driveKey = ui->m_streamDriveCBox->currentData().toString();
            Drive* drive = m_model->findDriveByNameOrPublicKey( driveKey.toStdString() );
            if ( drive == nullptr )
            {
                return;
            }
            updateStreamerTable( *drive );
            wizardUpdateStreamAnnouncementTable();
            wizardUpdateArchiveTable();
        }
    }, Qt::QueuedConnection );


    //
    // m_streamAnnouncementTable
    //
    ui->m_streamAnnouncementTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->m_streamAnnouncementTable->setSelectionMode( QAbstractItemView::SingleSelection );
    ui->m_wizardStreamAnnouncementTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->m_wizardStreamAnnouncementTable->setSelectionMode( QAbstractItemView::SingleSelection );

    connect( ui->m_streamAnnouncementTable, &QTableView::pressed, this, [this] (const QModelIndex &index)
    {
        if (index.isValid()) {
            ui->m_streamAnnouncementTable->selectRow( index.row() );
        }
    }, Qt::QueuedConnection);

    connect( ui->m_streamAnnouncementTable, &QTableView::clicked, this, [this] (const QModelIndex &index)
    {
        if (index.isValid()) {
            ui->m_streamAnnouncementTable->selectRow( index.row() );
        }
    }, Qt::QueuedConnection);

    // m_addStreamAnnouncementBtn
    connect(ui->m_addStreamAnnouncementBtn, &QPushButton::released, this, [this] ()
    {
        std::string driveKey = selectedDriveKeyInTable().toStdString();

        auto* drive = m_model->findDrive(driveKey);
        if ( drive == nullptr )
        {
            qWarning() << LOG_SOURCE << "bad driveKey";
            return;
        }
        
        if ( drive->isStreaming() )
        {
            qWarning() << LOG_SOURCE << "drive already 'is streaming'";
            return;
        }

        AddStreamAnnouncementDialog dialog(m_onChainClient, m_model, driveKey, this);
        auto rc = dialog.exec();
        if (rc == QDialog::Accepted)
        {
            drive->setCreatingStreamAnnouncement();
        }
    }, Qt::QueuedConnection);

    // m_delStreamAnnouncementBtn
    connect(ui->m_delStreamAnnouncementBtn, &QPushButton::released, this, [this] ()
    {
        if ( auto streamInfo = selectedStreamInfo(); streamInfo )
        {
            QMessageBox msgBox;
            const QString message = QString::fromStdString("'" + streamInfo->m_title + "' will be removed.");
            msgBox.setText(message);
            msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
            auto reply = msgBox.exec();

            if ( reply == QMessageBox::Ok )
            {
                auto* drive = selectedDriveInTable();

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
        }
    }, Qt::QueuedConnection);

    // m_copyLinkToStreamBtn
    connect(ui->m_copyLinkToStreamBtn, &QPushButton::released, this, [this] ()
    {
        if ( auto streamInfo = selectedStreamInfo(); streamInfo )
        {
            std::string link = streamInfo->getLink();

            QClipboard* clipboard = QApplication::clipboard();
            if ( !clipboard ) {
                qWarning() << LOG_SOURCE << "bad clipboard";
                return;
            }

            clipboard->setText( QString::fromStdString(link), QClipboard::Clipboard );
            if ( clipboard->supportsSelection() ) {
                clipboard->setText( QString::fromStdString(link), QClipboard::Selection );
            }
        }
    }, Qt::QueuedConnection);

    // m_startStreamingBtn
    connect(ui->m_startStreamingBtn, &QPushButton::released, this, [this] ()
    {
        std::string driveKey = selectedDriveKeyInTable().toStdString();

        auto* drive = m_model->findDrive(driveKey);
        if ( drive == nullptr )
        {
            qWarning() << LOG_SOURCE << "bad driveKey";
            return;
        }
        
        if ( drive->isStreaming() )
        {
            qWarning() << LOG_SOURCE << "drive already 'is streaming'";
            return;
        }

        auto rowList = ui->m_streamAnnouncementTable->selectionModel()->selectedRows();
        if ( rowList.count() == 0 )
        {
            if ( m_model->getStreams().size() == 0 )
            {
                QMessageBox msgBox;
                msgBox.setText( QString::fromStdString( "Add stream announcement" ) );
                msgBox.setInformativeText( "Press button '+'" );
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.exec();
                return;
            }
        }
        if ( auto streamInfo = selectedStreamInfo(); streamInfo )
        {
//            drive->setIsStreaming();
//            m_streamingProgressPanel->updateStreamingMode(drive);
//            m_streamingProgressPanel->setRegistering();
//            m_streamingProgressPanel->setVisible(true);
            startStreamingProcess( *streamInfo );
        }
    }, Qt::QueuedConnection);

    //m_addStreamRefBtn
    connect(ui->m_addStreamRefBtn, &QPushButton::released, this, [this] () {
        QClipboard* clipboard = QApplication::clipboard();
        if ( !clipboard ) {
            qWarning() << LOG_SOURCE << "bad clipboard";
            return;
        }

        std::string link = clipboard->text( QClipboard::Clipboard ).toStdString();
        if ( clipboard->supportsSelection() ) {
            link = clipboard->text( QClipboard::Selection ).toStdString();
        }

        StreamInfo streamInfo;
        try
        {
            streamInfo.parseLink(link);
            m_model->addStreamRef(streamInfo);
            updateViewerCBox();
        }
        catch(...)
        {
            qWarning() << "bad stream link: " << link;
        }
    }, Qt::QueuedConnection);

    // m_delStreamRefBtn
    connect(ui->m_delStreamRefBtn, &QPushButton::released, this, [this] () {
        auto rowIndex = ui->m_streamRefCBox->currentIndex();
        if ( rowIndex >= 0 )
        {
            std::string streamTitle;
            try {
                streamTitle = m_model->streamRefs().at(rowIndex).m_title;
            } catch (...) {
                return;
            }

            QMessageBox msgBox;
            const QString message = QString::fromStdString("'" + streamTitle + "' will be removed.");
            msgBox.setText(message);
            msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
            auto reply = msgBox.exec();

            if ( reply == QMessageBox::Ok )
            {
                m_model->deleteStreamRef( rowIndex );
                updateViewerCBox();
            }
        }
    }, Qt::QueuedConnection);

    // m_startViewingBtn
    connect(ui->m_startViewingBtn, &QPushButton::released, this, [this] () {
        startViewingStream();
    }, Qt::QueuedConnection);

    connectToStreamingTransactions();

//    void streamFinishTransactionConfirmed(const std::array<uint8_t, 32> &streamId);
//    void streamFinishTransactionFailed(const std::array<uint8_t, 32> &streamId, const QString& errorText);

    m_streamingPanel = new StreamingPanel( [this]{cancelStreaming();}, [this]{finishStreaming();}, this );
    m_streamingPanel->setWindowModality(Qt::WindowModal);
    m_streamingPanel->setVisible(false);
    connect( this, &MainWin::updateStreamingStatus, m_streamingPanel, &StreamingPanel::onUpdateStatus, Qt::QueuedConnection );

    updateViewerCBox();
}

std::vector<StreamInfo> MainWin::readStreamingAnnotations( const Drive&  driveInfo )
{
    std::vector<StreamInfo> streamInfoVector;

    // read from disc (.videos/*/info)
    {
        auto path = fs::path( driveInfo.getLocalFolder() + "/" + STREAM_ROOT_FOLDER_NAME);

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
        auto fsTree = driveInfo.getFsTree();
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
    
    return streamInfoVector;
}

void MainWin::onFsTreeReceivedForStreamAnnotations( const Drive& drive )
{
    updateStreamerTable( drive );
    wizardUpdateStreamAnnouncementTable();
    wizardUpdateArchiveTable();
}

void MainWin::updateStreamerTable( const Drive& drive )
{
    auto streamAnnotations = readStreamingAnnotations( drive );

    qDebug() << "announcements: " << streamAnnotations.size();

    auto table = ui->m_streamAnnouncementTable;
    
    while( table->rowCount() > 0 )
    {
        table->removeRow(0);
    }

    for( const auto& streamInfo: streamAnnotations )
    {
//        if ( streamInfo.m_streamingStatus == StreamInfo::ss_finished )
//        {
//            continue;
//        }

        size_t rowIndex = table->rowCount();
        table->insertRow( (int)rowIndex );
        {
            QDateTime dateTime = QDateTime::currentDateTime();
            dateTime.setSecsSinceEpoch( streamInfo.m_secsSinceEpoch );
            table->setItem( (int)rowIndex, 0, new QTableWidgetItem( dateTime.toString() ) );
        }
        {
//            auto* item = new QTableWidgetItem();
//            item->setData( Qt::UserRole, QString::fromStdString( streamInfo.m_title ) );
//            table->setItem( (int)rowIndex, 1, item );
            table->setItem( (int)rowIndex, 1, new QTableWidgetItem( QString::fromStdString(streamInfo.m_title) ) );
        }
        {
            table->setItem( (int)rowIndex, 2, new QTableWidgetItem(
                            QString::fromStdString( (streamInfo.m_streamingStatus==StreamInfo::ss_finished) ? "finished" : "" )));
        }
    }
}

void MainWin::updateViewerCBox()
{
    ui->m_streamRefCBox->clear();

    const auto& streamRefs = m_model->streamRefs();
    for( const auto& streamRef: streamRefs )
    {
        QString row;

        QDateTime dateTime = QDateTime::currentDateTime();
        dateTime.setSecsSinceEpoch( streamRef.m_secsSinceEpoch );
        row = dateTime.toString("yyyy-MMM-dd HH:mm") + " ❘ " + QString::fromStdString( streamRef.m_title );
        ui->m_streamRefCBox->addItem( row );
    }
}

void MainWin::updateViewerProgressPanel( int tabIndex )
{
    if ( tabIndex != 6 )
    {
        m_startViewingProgressPanel->setVisible(false);
        return;
    }

    if ( ui->m_streamingTabView->currentIndex() == 0 &&
        (m_model->m_viewerStatus == vs_waiting_stream_start || m_model->m_viewerStatus == vs_waiting_channel_creation) )
    {
        m_startViewingProgressPanel->setVisible(true);
        return;
    }

    m_startViewingProgressPanel->setVisible(false);
}

void MainWin::updateStreamerProgressPanel( int tabIndex )
{
    if ( tabIndex != 6 )
    {
        m_streamingProgressPanel->setVisible(false);
    }
    else
    {
        if ( ui->m_streamingTabView->currentIndex() == 1 )
        {
//todo            m_startStreamingProgressPanel->setVisible(true);
        }
        else
        {
            m_streamingProgressPanel->setVisible(false);
        }
    }
}

#pragma mark --startViewingStream--
void MainWin::startViewingStream()
{
//    dbg();
//    return;

    if ( m_model->m_viewerStatus != vs_no_viewing )
    {
        return;
    }

    // set current viewing stream info
    const StreamInfo* streamInfo = m_model->getStreamRef( ui->m_streamRefCBox->currentIndex() );
    if ( streamInfo == nullptr )
    {
        qWarning() << "streamInfo == nullptr; index: " << ui->m_streamRefCBox->currentIndex();
        return;
    }
    m_model->m_currentStreamInfo = *streamInfo;

    // find channel
    auto channelMap = m_model->getDownloadChannels();
    std::vector<DownloadChannel> channels;
    for( auto [key,channelInfo] : channelMap )
    {
        if ( boost::iequals( channelInfo.getDriveKey(), streamInfo->m_driveKey ) )
        {
            channels.push_back( channelInfo );
        }
    }

    if ( channels.empty() )
    {
        // create channel
        //
        QMessageBox msgBox;
        msgBox.setText( QString::fromStdString( "No channel for selected stream" ) );
        msgBox.setInformativeText( "Do You want to create channel?" );
        msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
        int reply = msgBox.exec();
        if ( reply == QMessageBox::Cancel )
        {
            return;
        }

        auto connection = connect( m_onChainClient->getDialogSignalsEmitter(), &DialogSignals::addDownloadChannel, this, &MainWin::addChannel );
        AddDownloadChannelDialog addChannelDialog( m_onChainClient, m_model, this, streamInfo->m_driveKey );
        auto rc = addChannelDialog.exec();

        if ( rc != QDialog::Accepted )
        {
            disconnect( connection );
        }
        else
        {
            m_modalDialog = new ModalDialog( "Information", "Channel is creating..." );
            
            m_model->m_viewerStatus = vs_waiting_channel_creation;
            
            m_model->setChannelFsTreeHandler( [connection,driveKey0=streamInfo->m_driveKey,this] ( bool success, const std::string& channelKey, const std::string& driveKey )
            {
                if ( !success || driveKey != driveKey0 )
                {
                    if ( !success )
                    {
                        qDebug() << "Channel creation faled: driveKey: " << driveKey.c_str();
                    }
                    return;
                }
                
                disconnect( connection );

                auto* channel = m_model->findChannel(channelKey);
                if ( channel != nullptr && channel->isCreating() )
                {
                    // waiting creation (may be old fsTree received)
                    return;
                }

                m_model->resetChannelFsTreeHandler();
                m_modalDialog->reject();
                delete m_modalDialog;
                m_modalDialog = nullptr;
                
                if ( channel != nullptr )
                {
                    startViewingStream2( *channel );
                }
                else
                {
                    displayError( "Internal error: channel not found" );
                }
            });

            m_modalDialog->exec();
        }
        return;
    }

    if ( channels.size() > 1 )
    {
        displayError( "TODO: select channel");
    }

    startViewingStream2( channels[0] );
}

struct ChunkInfo
{
    std::string m_filename;
};
static std::vector<ChunkInfo> gChunks;

static std::string parsePlaylist( const fs::path& fsPath )
{
    gChunks.clear();
    
    std::ifstream fin( fsPath );
    std::stringstream fPlaylist;
    fPlaylist << fin.rdbuf();
    
    std::string line;
    
    if ( ! std::getline( fPlaylist, line ) || memcmp( line.c_str(), "#EXTM3U", 7 ) != 0 )
    {
        return std::string("1-st line of playlist must be '#EXTM3U'");
    }
    
    int sequenceNumber = -1;
    int mediaIndex = 0;
    
    for(;;)
    {
        if ( ! std::getline( fPlaylist, line ) )
        {
            break;
        }

        if ( line.compare(0,15,"#EXT-X-VERSION:") == 0 )
        {
            int version;
            try
            {
                version = std::stoi( line.substr(15) );
                //_LOG( "version: " << version )
            }
            catch(...)
            {
                return std::string("invalid #EXT-X-VERSION: ") + line;
            }
            
            if ( version != 3 && version != 4 )
            {
                return std::string("#EXT-X-VERSION: invalid version number: ") + line.substr(15);
            }
            continue;
        }

        if ( line.compare(0,16+6,"#EXT-X-TARGETDURATION:") == 0 )
        {
            continue;
        }

        if ( line.compare(0,16+6,"#EXT-X-MEDIA-SEQUENCE:") == 0 )
        {
            try
            {
                sequenceNumber = std::stoi( line.substr(16+6) );
                //_LOG( "sequenceNumber: " << sequenceNumber )
            }
            catch(...)
            {
                return std::string("#EXT-X-MEDIA-SEQUENCE: cannot read sequence number: ") + line;
            }
        }

        if ( line.compare(0,8,"#EXTINF:") == 0 )
        {
            float duration;
            try
            {
                duration = std::stof( line.substr(8) );
                //_LOG( "duration: " << duration )
            }
            catch(...)
            {
                return std::string("#EXTINF: cannot read duration attribute: ") + line;
            }
            
            // Read Filename!!!
            std::string filename;
            if ( ! std::getline( fPlaylist, filename ) )
            {
                break;
            }

            qDebug() << "chunk: " << filename.c_str();
            gChunks.emplace_back( ChunkInfo{filename} );
            continue;
        }
    }

    // no errors
    return "";
}

void MainWin::downloadApprovedChunk( std::string dnChannelKey, std::string savePath, int chunkIndex )
{
    if ( chunkIndex >= gChunks.size() )
    {
        return;
    }
    
    auto& chunkFilename = gChunks[chunkIndex].m_filename;
    std::array<uint8_t,32> infoHash{ 0 };
    xpx_chain_sdk::ParseHexStringIntoContainer( chunkFilename.c_str(), 64, infoHash );
    
    auto ltHandle = m_model->downloadFile( dnChannelKey, infoHash );
                                          
    DownloadInfo downloadInfo;
    downloadInfo.setHash( infoHash );
    downloadInfo.setDownloadChannelKey( dnChannelKey );
    downloadInfo.setFileName( chunkFilename );
    downloadInfo.setSaveFolder( savePath );
    downloadInfo.setDownloadFolder( m_model->getDownloadFolder().string() );
    downloadInfo.setChannelOutdated(false);
    downloadInfo.setCompleted(false);
    downloadInfo.setProgress(0);
    downloadInfo.setHandle(ltHandle);
    downloadInfo.setNotification([dnChannelKey,chunkIndex,this](const DownloadInfo& info){
        downloadApprovedChunk( dnChannelKey, info.getSaveFolder(), chunkIndex+1 );
    });

    m_model->downloads().insert( m_model->downloads().begin(), downloadInfo );
}


bool MainWin::startViewingApprovedStream( DownloadChannel& channel )
{
    // check playlist 'obs-stream.m3u8' is not downloaded
    //TODO:

    sirius::drive::FsTree fsTree = channel.getFsTree();
    auto& uniqueFolderName = m_model->m_currentStreamInfo.m_uniqueFolderName;
    std::string savePath = STREAM_ROOT_FOLDER_NAME "/" + uniqueFolderName + "/HLS";
    auto playlistPath = savePath + "/" + PLAYLIST_FILE_NAME;
    auto* child = fsTree.getEntryPtr( playlistPath );
    
    if ( child == nullptr )
    {
        return false;
    }
    
    //
    // download playlist 'obs-stream.m3u8'
    //
    auto& playlistInfo = sirius::drive::getFile(*child);
    assert( sirius::drive::isFile(playlistInfo) );

    auto ltHandle = m_model->downloadFile( channel.getKey(),  playlistInfo.hash().array() );
    
    DownloadInfo downloadInfo;
    downloadInfo.setHash( playlistInfo.hash().array() );
    downloadInfo.setDownloadChannelKey( channel.getKey() );
    downloadInfo.setFileName( playlistInfo.name() );
    downloadInfo.setSaveFolder( "/" + savePath );
    downloadInfo.setDownloadFolder( m_model->getDownloadFolder().string() );
    downloadInfo.setChannelOutdated(false);
    downloadInfo.setCompleted(false);
    downloadInfo.setProgress(0);
    downloadInfo.setHandle(ltHandle);
    downloadInfo.setNotification([channelKey=channel.getKey(),this](const DownloadInfo& info)
    {
        // parse playlist
        parsePlaylist( m_model->getDownloadFolder().string() + "/" + info.getSaveFolder() + "/" + info.getFileName() );
        
        downloadApprovedChunk( channelKey, info.getSaveFolder(), 0 );
    });

    m_model->downloads().insert( m_model->downloads().begin(), downloadInfo );
    
    return true;
}

void MainWin::startViewingStream2( DownloadChannel& channel )
{
    if ( startViewingApprovedStream( channel ) )
    {
        return;
    }
    
    m_model->requestStreamStatus( m_model->m_currentStreamInfo, [this] ( const sirius::drive::DriveKey& driveKey,
                                                                         bool                           isStreaming,
                                                                         const std::array<uint8_t,32>&  streamId )
    {
        onStreamStatusResponse( driveKey, isStreaming, streamId );
    });

    m_model->m_viewerStatus = vs_waiting_stream_start;
    m_startViewingProgressPanel->setWaitingStreamStart();
    m_startViewingProgressPanel->setVisible( true );

    QTimer::singleShot(10, this, [this]
    {
        if ( m_model->m_viewerStatus == vs_waiting_stream_start )
        {
            m_model->requestStreamStatus( m_model->m_currentStreamInfo, [this] ( const sirius::drive::DriveKey& driveKey,
                                                                                 bool                           isStreaming,
                                                                                 const std::array<uint8_t,32>&  streamId )
            {
                qWarning() << "timer: ";
                onStreamStatusResponse( driveKey, isStreaming, streamId );
            });
        }
    });
}

void MainWin::onStreamStatusResponse( const sirius::drive::DriveKey& driveKey,
                                      bool                           isStreaming,
                                      const std::array<uint8_t,32>&  streamId )
{

}

void MainWin::cancelViewingStream()
{
    m_model->m_viewerStatus = vs_no_viewing;
    m_startViewingProgressPanel->setVisible( false );
}

static void showErrorMessage( QString title, QString message )
{
    QMessageBox msgBox;
    msgBox.setText( title );
    msgBox.setInformativeText( message );
    msgBox.setStandardButtons( QMessageBox::Ok );
    msgBox.exec();
}

static fs::path ffmpegPath()
{
#if defined _WIN32
    return { getSettingsFolder().string() + "/ffmpeg" };
#elif defined __APPLE__
    auto path = std::string(getenv("HOME")) + "/.XpxSiriusFfmpeg/ffmpeg";
    return fs::path(path);
#else // LINUX
    return "/usr/bin/ffmpeg";
#endif
}


static bool checkFfmpegInstalled()
{
#if defined _WIN32
#else
    std::error_code ec;
    auto path = ffmpegPath();
    if ( ! fs::exists(path,ec) )
    {
        showErrorMessage( QString::fromStdString(path.string()), "ffmpeg is not installed" );
        return false;
    }
    return true;
#endif
}

void MainWin::startFfmpegStreamingProcess(){}
//{
//    std::optional<StreamInfo> streamInfo = m_model->currentStreamInfo();
//    if ( !streamInfo )
//    {
//        qWarning() << LOG_SOURCE << "🔴 currentStreamInfo is not set !!!";
//        return;
//    }
//    auto workingFolder = fs::path( ui->m_streamFolder->text().trimmed().toStdString() + "/" + streamInfo->m_uniqueFolderName);
//
//    std::error_code ec;
//    if ( fs::is_empty(workingFolder) )
//    {
//        QMessageBox msgBox;
//        const QString message = QString::fromStdString("Folder is not empty. All data will be removed.");
//        msgBox.setText(message);
//        msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
//        auto reply = msgBox.exec();
//
//        if ( reply != QMessageBox::Ok )
//        {
//            return;
//        }
//    }
//
//    delete m_ffmpegStreamingProcess;
//    m_ffmpegStreamingProcess = new QProcess(this);
//
//    QString program = "ffmpeg";
//    QStringList arguments;
//
//#if defined _WIN32
//#elif defined __APPLE__
//    auto path = std::string(getenv("HOME")) + "/.XpxSiriusFfmpeg/ffmpeg";
//
//    program = QString::fromStdString( path );
//    //-f avfoundation -framerate 30 -r 30 -pixel_format uyvy422 -i "0:0" -c:v h264 -c:a aac -hls_time 10 -hls_list_size 0 -hls_segment_filename "output_%03d.ts" output.m3u8
//    arguments   << "-f" << "avfoundation"
//                << "-framerate" << "30"
//                << "-r" << "30"
//                << "-pixel_format" << "uyvy422"
//                << "-i" << "0:0"
//                << "-c:v" << "h264"
//                << "-c:a" << "aac"
//                << "-hls_time" << "10"
//                << "-hls_list_size" << "0"
//                << "-fs" << "100000000" //TODO!!!
//                << "-hls_segment_filename" << "stream_%03d.ts" << "playlist.m3u8";
//#else
//#endif
//
//    connect( m_ffmpegStreamingProcess, &QProcess::errorOccurred, this, [] ( QProcess::ProcessError error ) {
//        qDebug() << LOG_SOURCE << "🟠 errorOccurred: " << error;
//    });
//    connect( m_ffmpegStreamingProcess, &QProcess::started, this, [] () {
//        qDebug() << LOG_SOURCE << "🟢 started! ";
//    });
//    connect( m_ffmpegStreamingProcess, &QProcess::stateChanged, this, [] (QProcess::ProcessState state) {
//        qDebug() << LOG_SOURCE << "🟢 state: " << state;
//    });
//    connect( m_ffmpegStreamingProcess, &QProcess::finished, this, [] (int exitCode) {
//        qDebug() << LOG_SOURCE << "🟢 finished: " << exitCode;
//    });
//    connect( m_ffmpegStreamingProcess, &QProcess::finished, this, [] (int exitCode) {
//        qDebug() << LOG_SOURCE << "🟢 finished: " << exitCode;
//    });
//    connect( m_ffmpegStreamingProcess, &QProcess::readyReadStandardOutput, this, [this] () {
//        while( m_ffmpegStreamingProcess->canReadLine()){
//               qDebug() << "🔵🔵 " << m_ffmpegStreamingProcess->readLine();
//        }
//    });
//    connect( m_ffmpegStreamingProcess, &QProcess::readyReadStandardError, this, [this] () {
//        QString err = m_ffmpegStreamingProcess->readAllStandardError();
//        qDebug() << "🟠🟠 " << err;
//    });
//
//    qDebug() << LOG_SOURCE << "🔵 Start ffmpeg; workingFolder: " << workingFolder.string();
//    qDebug() << LOG_SOURCE << "🔵 Start ffmpeg; program: " << program;
//    qDebug() << LOG_SOURCE << "🔵 Start ffmpeg; arguments: " << arguments;
//
//    m_ffmpegStreamingProcess->setWorkingDirectory( QString::fromStdString(workingFolder.string()) );
//    m_ffmpegStreamingProcess->start( program, arguments );
//}

//#pragma mark --Chain-Interface--
//
//-- methods OnChainClient
//std::string streamStart(const std::array<uint8_t, 32>& rawDrivePubKey, const std::string& folderName, uint64_t expectedUploadSizeMegabytes, uint64_t feedbackFeeAmount);
//void streamFinish(const std::array<uint8_t, 32>& rawDrivePubKey, const std::array<uint8_t, 32>& streamId, uint64_t actualUploadSizeMegabytes,const std::array<uint8_t, 32>& streamStructureCdi);
//void streamPayment(const std::array<uint8_t, 32>& rawDrivePubKey, const std::array<uint8_t, 32>& streamId, uint64_t additionalUploadSizeMegabytes);
//-- signals OnChainClient
//void streamStartTransactionConfirmed(const std::array<uint8_t, 32> &streamId);
//void streamStartTransactionFailed(const std::array<uint8_t, 32> &streamId, const QString& errorText);
//void streamFinishTransactionConfirmed(const std::array<uint8_t, 32> &streamId);
//void streamFinishTransactionFailed(const std::array<uint8_t, 32> &streamId, const QString& errorText);
//void streamPaymentTransactionConfirmed(const std::array<uint8_t, 32> &streamId);
//void streamPaymentTransactionFailed(const std::array<uint8_t, 32> &streamId, const QString& errorText);
//

#pragma mark --startStreamingProcess--
void MainWin::startStreamingProcess( const StreamInfo& streamInfo )
{
    try
    {
        qDebug() << LOG_SOURCE << "🔵 streamInfo: " << streamInfo.m_annotation;

//        QString::fromStdString(streamInfo.m_driveKey).toUpper().toStdString();
        auto driveKey = streamInfo.m_driveKey;

        //
        // Get 'drive'
        //
        auto* drive = m_model->findDrive( driveKey );
        if ( drive == nullptr )
        {
            qWarning() << LOG_SOURCE << "🔴 ! internal error: not found drive: " << driveKey;
            displayError( "not found drive: " + driveKey );
            return;
        }

        m_onChainClient->getBlockchainEngine()->getDriveById( drive->getKey(), [streamInfo=streamInfo,this] (xpx_chain_sdk::Drive xpxDrive, bool, std::string, std::string)
        {
            auto driveKey = streamInfo.m_driveKey;
            
            auto* drive = m_model->findDrive( driveKey );
            if ( drive == nullptr )
            {
                qWarning() << LOG_SOURCE << "🔴 ! internal error: not found drive: " << driveKey;
                displayError( "not found drive: " + driveKey );
                return;
            }
        auto freeSize = xpxDrive.data.size - xpxDrive.data.usedSizeBytes;
//        auto freeSize = 20;

            //
            // Get 'm3u8StreamFolder' - where OBS saves chuncks and playlist
            //
            auto m3u8StreamFolder = fs::path( ObsProfileData().m_recordingPath.toStdString() );

            if ( m3u8StreamFolder.empty() )
            {
                qCritical() << LOG_SOURCE << "🔴 ! m3u8StreamFolder.empty()";
                displayError( "Invalid OBS profile 'Sirius-stream'", "'Recording Path' is not set" );
                return;
            }

            std::error_code ec;
            if ( ! fs::exists( m3u8StreamFolder, ec ) )
            {
                qDebug() << LOG_SOURCE << "🔴 Stream folder does not exist" << streamInfo.m_driveKey;
                qDebug() << LOG_SOURCE << "🔴 Stream folder does not exist" << m3u8StreamFolder.string();

                fs::create_directories( m3u8StreamFolder, ec );
                if ( ec )
                {
                    qCritical() << LOG_SOURCE << "🔴 ! cannot create folder : " << m3u8StreamFolder.string() << " error:" << ec.message();
                    displayError( "Cannot create folder: " + m3u8StreamFolder.string(), ec.message() );
                    return;
                }
            }

            auto driveKeyHex = rawHashFromHex( driveKey.c_str() );

            //TODO: 20 GB
            //uint64_t  expectedUploadSizeMegabytes = 20*1024*1024*1024;

            uint64_t  expectedUploadSizeMegabytes = ( freeSize > 20*1024) ? 20*1024 : freeSize; // (could be extended)
            uint64_t feedbackFeeAmount = 100; // now, not used, it is amount of token for replicator
            auto uniqueStreamFolder  = fs::path( drive->getLocalFolder() + "/" + STREAM_ROOT_FOLDER_NAME + "/" + streamInfo.m_uniqueFolderName);
            auto chuncksFolder = uniqueStreamFolder / "HLS";
            auto torrentsFolder = getSettingsFolder() / driveKey / CLIENT_SANDBOX_FOLDER0;
            
            if ( ! fs::exists( chuncksFolder, ec ) )
            {
                fs::create_directories( chuncksFolder, ec );
                if ( ec )
                {
                    qCritical() << LOG_SOURCE << "🔴 ! cannot create chunk folder : " << chuncksFolder.string() << " error:" << ec.message();
                    displayError( "Cannot create chunk folder : " + chuncksFolder.string(), ec.message() );
                    return;
                }
            }
            if ( ! fs::exists( torrentsFolder, ec ) )
            {
                fs::create_directories( torrentsFolder, ec );
                if ( ec )
                {
                    qCritical() << LOG_SOURCE << "🔴 ! cannot create torrent folder : " << torrentsFolder.string() << " error:" << ec.message();
                    displayError( "Cannot create torrent folder : " + torrentsFolder.string(), ec.message() );
                    return;
                }
            }
            
            endpoint_list endPointList = drive->getEndpointReplicatorList();
            if ( endPointList.empty() )
            {
                qWarning() << LOG_SOURCE << "🔴 ! endPointList.empty() !";
                displayError( "endPointList.empty()" );
                return;
            }
            
            fs::path m3u8Playlist = fs::path(m3u8StreamFolder.string()) / PLAYLIST_FILE_NAME;
            sirius::drive::ReplicatorList replicatorList = drive->getReplicators();
            
            m_model->setCurrentStreamInfo( streamInfo );
            
            auto callback = [=,model = m_model,this](std::string txHashString)
            {
                auto* drive = model->findDrive( driveKey );
                if ( drive == nullptr )
                {
                    qWarning() << LOG_SOURCE << "🔴 ! internal error: not found drive: " << driveKey;
                    displayError( "Internal error: not found drive: " + driveKey );
                    return;
                }
                
                qDebug () << "streamStart: txHashString: " << txHashString.c_str();
                auto txHash = rawHashFromHex( txHashString.c_str() );
                
                qDebug() << "🔵 txHash: " << sirius::drive::toString(txHash);
                qDebug() << "🔵 driveKeyHex: " << sirius::drive::toString(driveKeyHex);
                qDebug() << "🔵 m3u8Playlist: " << m3u8Playlist.string();
                qDebug() << "🔵 chuncksFolder: " << chuncksFolder.string();
                qDebug() << "🔵 torrentsFolder: " << torrentsFolder.string();
                for( auto& endpoint : endPointList )
                {
                    std::cout << "🔵 endpoint: " << endpoint << "\n";
                }
                
                gStorageEngine->startStreaming( txHash,
                                               streamInfo.m_uniqueFolderName,
                                               driveKeyHex,
                                               m3u8Playlist,
                                               drive->getLocalFolder(),
                                               torrentsFolder,
                                               [](const std::string&){},
                                               endPointList );
                
                drive->setModificationHash( txHash );
                drive->setIsStreaming();
                drive->updateDriveState(registering);
                //              drive->updateDriveState(canceled); //todo
                //              drive->updateDriveState(no_modifications); //todo
            };
            
            // Send tx to REST server
            m_onChainClient->streamStart( driveKeyHex, streamInfo.m_uniqueFolderName, expectedUploadSizeMegabytes, feedbackFeeAmount, callback );
        });
    }
    catch (...) {
        return;
    }
}

#pragma mark --finishStreaming--
void MainWin::finishStreaming()
{
    auto streamInfo = m_model->currentStreamInfo();
    assert( streamInfo );
    
    m_streamingProgressPanel->setApproving();

//    if ( auto drive = m_model->findDrive( streamInfo->m_driveKey ); drive != nullptr )
//    {
//        drive->updateDriveState(uploading);
//    }
    
    auto driveKey = streamInfo->m_driveKey;

    gStorageEngine->finishStreaming( driveKey, [=,this]( const sirius::Key& driveKey, const sirius::drive::InfoHash& streamId, const sirius::drive::InfoHash& actionListHash, uint64_t streamBytes )
    {
        // Execute code on the main thread
        QTimer::singleShot( 0, this, [=,this]
        {
            qDebug() << "finishStreaming: driveKey: " << sirius::drive::toString(driveKey.array()).c_str();
            qDebug() << "finishStreaming: streamId: " << sirius::drive::toString(streamId).c_str();
            qDebug() << "finishStreaming: actionListHash: " << sirius::drive::toString(actionListHash).c_str();
            qDebug() << "finishStreaming: streamSizeBytes: " << streamBytes;
            uint64_t actualUploadSizeMegabytes = (streamBytes+(1024*1024-1))/(1024*1024);
            qDebug() << "finishStreaming: actualUploadSizeMegabytes: " << actualUploadSizeMegabytes;
            m_onChainClient->streamFinish( driveKey.array(),
                                           streamId.array(),
                                           actualUploadSizeMegabytes,
                                           actionListHash.array() );
        });
    });

    m_streamingPanel->hidePanel();
}

void MainWin::cancelStreaming()
{
    auto drive = currentStreamingDrive();
    if ( drive == nullptr )
    {
        displayError( "internal error: no streaming drive" );
    }

    QMessageBox msgBox;
    msgBox.setText( "Confirmation" );
    msgBox.setInformativeText( "Please confirm cancel streaming" );
    msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Discard );
    if ( msgBox.exec() != QMessageBox::Ok )
    {
        return;
    }

    m_onChainClient->cancelDataModification( rawHashFromHex( drive->getKey().c_str() ) );

    if ( drive->getState() == registering )
    {
        drive->updateDriveState(uploading);
    }

    drive->updateDriveState(canceling);

    m_streamingPanel->hidePanel();
}

void MainWin::connectToStreamingTransactions()
{
    // streamStartTransactionConfirmed
    connect(m_onChainClient, &OnChainClient::streamStartTransactionConfirmed, this, [this](const std::array<uint8_t,32>& streamId )
    {
        qDebug () << "streamStartTransactionConfirmed: " << sirius::drive::toString(streamId).c_str();
        if ( auto drive = m_model->findDriveByModificationId( streamId ); drive != nullptr )
        {
            drive->updateDriveState(uploading);

            if ( ! gUseFfmpeg )
            {
                m_streamingPanel->onStartStreaming();
            }
            else
            {
                assert( m_streamingView == nullptr );
                m_streamingView = new StreamingView( [this] {cancelStreaming();}, [this] {finishStreaming();}, this );
                m_streamingView->setWindowModality(Qt::WindowModal);
                m_streamingView->show();

                startFfmpegStreamingProcess();
            }
        }
        else
        {
            qWarning () << "!!! streamStartTransactionConfirmed: drive not found!";
        }

        loadBalance();
    }, Qt::QueuedConnection);

    // streamStartTransactionFailed
    connect(m_onChainClient, &OnChainClient::streamStartTransactionFailed, this, [this](const std::array<uint8_t,32>& tx, const QString& errorText )
    {
        qDebug () << "MainWin::onDataModificationTransactionFailed. Your last modification was declined: '" + rawHashToHex(tx);
        qDebug () << "MainWin::onDataModificationTransactionFailed. errorText: '" << errorText;
        displayError( "Start stream transaction failed.", errorText.toStdString());
        
        if ( auto drive = m_model->findDriveByModificationId( tx ); drive != nullptr )
        {
            drive->updateDriveState(failed);
            drive->resetStreamingStatus();
        }
        m_model->setCurrentStreamInfo({});
    }, Qt::QueuedConnection);

    // cancelModificationTransactionConfirmed
    connect( m_onChainClient, &OnChainClient::cancelModificationTransactionConfirmed, this,
        [this] ( const std::array<uint8_t, 32> &driveId, const QString &modificationId )
    {
        auto* drive = m_model->findDrive( sirius::drive::toString(driveId) );
        if ( drive == nullptr )
        {
            qWarning() << "cancelModificationTransactionConfirmed: drive not found: " << sirius::drive::toString(driveId).c_str();
            return;
        }

        if ( drive->getState() == canceled )
        {
            drive->updateDriveState(no_modifications);
            return;
        }

        m_model->setCurrentStreamInfo({});
        drive->updateDriveState(canceled);
        drive->resetStreamingStatus();

    }, Qt::QueuedConnection);

    // cancelModificationTransactionFailed
    connect( m_onChainClient, &OnChainClient::cancelModificationTransactionFailed, this,
        [this] ( const std::array<uint8_t, 32> &driveId, const QString &modificationId )
    {
        if ( auto drive = m_model->findDrive( sirius::drive::toString(driveId) ); drive != nullptr )
        {
            drive->updateDriveState(failed);
        }
        displayError( "Cancel transaction failed.");
        m_model->setCurrentStreamInfo({});
    }, Qt::QueuedConnection);

    // streamFinishTransactionConfirmed
    connect( m_onChainClient, &OnChainClient::streamFinishTransactionConfirmed, this,
        [this] ( const std::array<uint8_t, 32> &streamId )
    {
        if ( auto drive = m_model->findDriveByModificationId( streamId ); drive != nullptr )
        {
            drive->updateDriveState(approved);
        }
        updateDiffView();
    }, Qt::QueuedConnection);

    // streamFinishTransactionFailed
    connect( m_onChainClient, &OnChainClient::streamFinishTransactionFailed, this,
        [this] ( const std::array<uint8_t, 32> &streamId, const QString& errorText )
    {
        if ( auto drive = m_model->findDriveByModificationId( streamId ); drive != nullptr )
        {
            drive->updateDriveState(failed);
        }
        displayError( "Finish stream transaction failed.", errorText.toStdString());
    }, Qt::QueuedConnection);
}

void MainWin::dbg()
{
    boost::asio::ip::address a = boost::asio::ip::make_address("18.142.139.189");
    boost::asio::ip::udp::endpoint ep{a, 7904};

    m_model->requestModificationStatus( "0A0EAC0E56FE4C052B66D070434621E74793FBF1D6F45286897240681A668BB1",
                                        "892B8E3DEAB4DCE16946881405B94DF8DBAE7B80F6CC2D78DB1CC5098D253E6B",
                                        "AB0F0F0F00000000000000000000000000000000000000000000000000000000",
                                       ep );
}
