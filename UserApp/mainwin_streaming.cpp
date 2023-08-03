#include "mainwin.h"
#include "./ui_mainwin.h"

#include "AddStreamAnnouncementDialog.h"
#include "AddStreamRefDialog.h"
#include "AddDownloadChannelDialog.h"

#include "Model.h"
#include "StreamInfo.h"
#include "ModifyProgressPanel.h"
#include "StorageEngine.h"
#include "Account.h"

#include <cerrno>
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

QString MainWin::currentStreamingDriveKey() const
{
    return ui->m_streamDriveCBox->currentData().toString();
}

Drive* MainWin::currentStreamingDrive() const
{
    auto driveKey = ui->m_streamDriveCBox->currentData().toString().toStdString();
    Drive* drive = m_model->findDrive( driveKey );
    if ( drive == nullptr && (ui->m_streamDriveCBox->count() == 0) )
    {
        qWarning() << LOG_SOURCE << "! internal error: bad drive key";
    }
    return drive;
}

void MainWin::initStreaming()
{
    //todo remove streamerAnnouncements from serializing
    m_model->streamerAnnouncements().clear();
    
    // m_startStreamingBtn
    connect( ui->m_startStreamingBtn, &QPushButton::released, this, &MainWin::onStartStreamingBtn, Qt::QueuedConnection );

    // m_streamingTabView
    connect(ui->m_streamingTabView, &QTabWidget::currentChanged, this, [this](int index)
    {
        updateViewerProgressPanel( ui->tabWidget->currentIndex() );
        updateStreamerProgressPanel( ui->tabWidget->currentIndex() );
        
        if (auto drive = currentStreamingDrive(); drive && ui->tabWidget->currentIndex() == 4 && ui->m_streamingTabView->currentIndex() == 1 ) {
            updateDriveWidgets( drive->getKey(), drive->getState(), false );
        }
        else
        {
            m_modifyProgressPanel->setVisible(false);
            m_streamingProgressPanel->setVisible(false);
        }
    });
    
    QHeaderView* header = ui->m_streamAnnouncementTable->horizontalHeader();
    header->setSectionResizeMode(0,QHeaderView::Stretch);
    header->setSectionResizeMode(1,QHeaderView::Stretch);
    header->setSectionResizeMode(2,QHeaderView::Stretch);

    
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
        }
    }, Qt::QueuedConnection );

            
    //
    // m_streamAnnouncementTable
    //
    ui->m_streamAnnouncementTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->m_streamAnnouncementTable->setSelectionMode( QAbstractItemView::SingleSelection );

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
    connect(ui->m_addStreamAnnouncementBtn, &QPushButton::released, this, [this] () {
        std::string driveKey = currentStreamingDriveKey().toStdString();

        if ( m_model->findDrive(driveKey) == nullptr )
        {
            qWarning() << LOG_SOURCE << "bad driveKey";
            return;
        }
        
        AddStreamAnnouncementDialog dialog(m_onChainClient, m_model, driveKey, this);
        auto rc = dialog.exec();
    }, Qt::QueuedConnection);

    // m_delStreamAnnouncementBtn
    connect(ui->m_delStreamAnnouncementBtn, &QPushButton::released, this, [this] () {
        auto rowList = ui->m_streamAnnouncementTable->selectionModel()->selectedRows();
        if ( rowList.count() > 0 )
        {
            StreamInfo streamInfo;
            
            int rowIndex = rowList.constFirst().row();
            try {
                streamInfo = m_model->streamerAnnouncements().at(rowIndex);
            } catch (...) {
                return;
            }
        
            QMessageBox msgBox;
            const QString message = QString::fromStdString("'" + streamInfo.m_title + "' will be removed.");
            msgBox.setText(message);
            msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
            auto reply = msgBox.exec();

            if ( reply == QMessageBox::Ok )
            {
                auto* drive = currentStreamingDrive();

                if ( drive != nullptr )
                {
                    auto streamFolder = fs::path( drive->getLocalFolder() ) / STREAM_ROOT_FOLDER_NAME / streamInfo.m_uniqueFolderName;
                    std::error_code ec;
                    fs::remove_all( streamFolder, ec );
                    qWarning() << "remove: " << streamFolder.string().c_str() << " ec:" << ec.message().c_str();

                    if ( !ec )
                    {
                        sirius::drive::ActionList actionList;
                        auto streamFolder = fs::path( STREAM_ROOT_FOLDER_NAME ) / streamInfo.m_uniqueFolderName;
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
    connect(ui->m_copyLinkToStreamBtn, &QPushButton::released, this, [this] () {
        auto rowList = ui->m_streamAnnouncementTable->selectionModel()->selectedRows();
        if ( rowList.count() > 0 )
        {
            int rowIndex = rowList.constFirst().row();
            try {
                auto streamAnnouncements = m_model->streamerAnnouncements();
                std::string link = m_model->streamerAnnouncements().at(rowIndex).getLink();

                QClipboard* clipboard = QApplication::clipboard();
                if ( !clipboard ) {
                    qWarning() << LOG_SOURCE << "bad clipboard";
                    return;
                }

                clipboard->setText( QString::fromStdString(link), QClipboard::Clipboard );
                if ( clipboard->supportsSelection() ) {
                    clipboard->setText( QString::fromStdString(link), QClipboard::Selection );
                }
            } catch (...) {
                return;
            }
        }
    }, Qt::QueuedConnection);
    
    // m_addStreamAnnouncementBtn
    connect(ui->m_startStreamingBtn, &QPushButton::released, this, [this] ()
    {
        auto rowList = ui->m_streamAnnouncementTable->selectionModel()->selectedRows();
        if ( rowList.count() == 0 )
        {
            if ( m_model->streamerAnnouncements().size() == 0 )
            {
                QMessageBox msgBox;
                msgBox.setText( QString::fromStdString( "Add stream announcement" ) );
                msgBox.setInformativeText( "Press button '+'" );
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.exec();
                return;
            }
            
            
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

    
    connect(m_onChainClient, &OnChainClient::streamStartTransactionConfirmed, this, [this](const std::array<uint8_t,32>& tx) {
        qDebug () << "MainWin::streamStartTransactionConfirmed: '" + rawHashToHex(tx);
        if ( auto drive = m_model->findDriveByModificationId( tx ); drive != nullptr )
        {
            drive->updateDriveState(uploading);
        }
        else
        {
            qWarning () << "!!! MainWin::MainWin::streamStartTransactionConfirmed: drive not found!";
        }

        loadBalance();
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::streamStartTransactionFailed, this, [this](const std::array<uint8_t,32>& tx, const QString& errorText )
    {
        qDebug () << "MainWin::onDataModificationTransactionFailed. Your last modification was declined: '" + rawHashToHex(tx);
        qDebug () << "MainWin::onDataModificationTransactionFailed. errorText: '" << errorText;
        if ( auto drive = m_model->findDriveByModificationId( tx ); drive != nullptr )
        {
            drive->updateDriveState(failed);
        }
    }, Qt::QueuedConnection);

    // m_streamFolder
    ui->m_streamFolder->setText( QString::fromStdString( m_model->streamFolder() ) );
    
    connect( ui->m_streamFolder, &QLineEdit::textChanged, this, [this] (auto text)
    {
        m_model->streamFolder() = text.trimmed().toStdString();
        m_model->saveSettings();
    });

    // m_browseStreamFolderBtn
    connect( ui->m_browseStreamFolderBtn, &QPushButton::released, this, [this]()
    {
        QFlags<QFileDialog::Option> options = QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks;
#ifdef Q_OS_LINUX
        options |= QFileDialog::DontUseNativeDialog;
#endif
        const QString path = QFileDialog::getExistingDirectory(this, tr("Choose stream directory"), "/", options);
        ui->m_streamFolder->setText(path.trimmed());
    });

//    void streamFinishTransactionConfirmed(const std::array<uint8_t, 32> &streamId);
//    void streamFinishTransactionFailed(const std::array<uint8_t, 32> &streamId, const QString& errorText);

    updateViewerCBox();
}

void MainWin::readStreamingAnnotations( const Drive&  driveInfo )
{
    bool todoShouldBeSync = false;
    std::vector<StreamInfo>& streamInfoVector = m_model->streamerAnnouncements();
    
    streamInfoVector.clear();
    
    // read from disc (.videos/*/info)
    {
        auto path = fs::path( driveInfo.getLocalFolder() ) / STREAM_ROOT_FOLDER_NAME;
        
        std::error_code ec;
        if ( ! fs::is_directory(path,ec) )
        {
            return;
        }
        
        for( const auto& entry : std::filesystem::directory_iterator( path ) )
        {
            const auto entryName = entry.path().filename().string();
            if ( entry.is_directory() )
            {
                auto fileName = path / entryName / STREAM_INFO_FILE_NAME;
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
                    auto it = std::find_if( streamInfoVector.begin(), streamInfoVector.end(), [&streamFolder] (const StreamInfo& streamInfo) {
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

void MainWin::onFsTreeReceivedForStreamAnnotations( const Drive& drive )
{
    auto* currentDrive = currentStreamingDrive();
    if ( currentDrive != nullptr && boost::iequals( currentDrive->getKey(), drive.getKey() ) )
    {
        updateStreamerTable( *currentDrive );
    }
}

void MainWin::updateStreamerTable( Drive& driveInfo )
{
    readStreamingAnnotations( driveInfo );
    
//    ui->m_streamAnnouncementTable->clearContents();

    qDebug() << "announcements: " << m_model->streamerAnnouncements().size();

    while( ui->m_streamAnnouncementTable->rowCount() > 0 )
    {
        ui->m_streamAnnouncementTable->removeRow(0);
    }
    
    for( const auto& streamInfo: m_model->streamerAnnouncements() )
    {
        if ( streamInfo.m_streamingStatus == StreamInfo::ss_finished )
        {
            continue;
        }
            
        size_t rowIndex = ui->m_streamAnnouncementTable->rowCount();
        ui->m_streamAnnouncementTable->insertRow( rowIndex );
        {
            QDateTime dateTime = QDateTime::currentDateTime();
            dateTime.setSecsSinceEpoch( streamInfo.m_secsSinceEpoch );
            ui->m_streamAnnouncementTable->setItem( rowIndex, 0, new QTableWidgetItem( dateTime.toString() ) );
        }
        {
            QTableWidgetItem* item = new QTableWidgetItem();
//            else if ( streamInfo.m_streamingStatus == StreamInfo::ss_running )
//            {
//                item->setData( Qt::DisplayRole, QString::fromStdString( streamInfo.m_title + " (running...)") );
//            }
//            else
//            {
                item->setData( Qt::DisplayRole, QString::fromStdString( streamInfo.m_title ) );
//            }
            ui->m_streamAnnouncementTable->setItem( rowIndex, 1, item );
        }
        {
            ui->m_streamAnnouncementTable->setItem( rowIndex, 2, new QTableWidgetItem( QString::fromStdString( streamInfo.m_uniqueFolderName ) ) );
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
    if ( tabIndex != 4 )
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
    if ( tabIndex != 4 )
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
    
    if ( channels.size() == 0 )
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

        AddDownloadChannelDialog dialog( m_onChainClient, m_model, this, streamInfo->m_driveKey );
        connect( &dialog, &AddDownloadChannelDialog::addDownloadChannel, this, &MainWin::addChannel );
        auto rc = dialog.exec();
        if ( rc )
        {
            m_model->m_viewerStatus = vs_waiting_channel_creation;
        }
        return;
    }

    if ( channels.size() > 1 )
    {
        // select channel
        return;
    }
    
    startViewingStream2();
}
    
void MainWin::updateCreateChannelStatusForVieweing( const DownloadChannel& channel )
{
    if ( m_model->m_viewerStatus != vs_waiting_channel_creation ||
         boost::iequals( m_model->m_currentStreamInfo.m_driveKey, channel.getDriveKey() ) )
    {
        return;
    }
    
    startViewingStream2();
}

void MainWin::startViewingStream2()
{
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

void MainWin::cancelOrFinishStreaming()
{
    Drive* drive = currentStreamingDrive();
    if ( drive == nullptr )
    {
        qWarning() << LOG_SOURCE << "cancelOrFinishStreaming: drive == nullptr";
    }

//    switch ( drive->getState() ) {
//        case registering:
//            break;
//
//            creating,
//            unconfirmed,
//            deleting,
//            deleted,
//            no_modifications,
//            registering,
//            approved,
//            uploading,
//            failed,
//            canceling,
//            canceled,
//
//        default:
//            break;
//    }
    m_streamingProgressPanel->setVisible(false);
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
    auto path = std::string(getenv("HOME")) + "/.XpxSiriusFfmpeg/ffmpeg";
    return fs::path(path);
}


static bool checkFfmpegInstalled()
{
#if defined _WIN32
#elif defined __APPLE__
    std::error_code ec;
    auto path = ffmpegPath();
    if ( ! fs::exists(path,ec) )
    {
        showErrorMessage( QString::fromStdString(path.string()), "ffmpeg is not installed" );
        return false;
    }
#else
#endif
}

void MainWin::startFfmpegStreamingProcess( std::string workingFolder )
{
    delete m_ffmpegStreamingProcess;
    m_ffmpegStreamingProcess = new QProcess(this);

    QString program = "ffmpeg";
    QStringList arguments;
#if defined _WIN32
#elif defined __APPLE__
    auto path = std::string(getenv("HOME")) + "/.XpxSiriusFfmpeg/ffmpeg";
    
    program = QString::fromStdString( path );
    //-f avfoundation -framerate 30 -r 30 -pixel_format uyvy422 -i "0:0" -c:v h264 -c:a aac -hls_time 10 -hls_list_size 0 -hls_segment_filename "output_%03d.ts" output.m3u8
    arguments   << "-f" << "avfoundation"
                << "-framerate" << "30"
                << "-r" << "30"
                << "-pixel_format" << "uyvy422"
                << "-i" << "0:0"
                << "-c:v" << "h264"
                << "-c:a" << "aac"
                << "-hls_time" << "10"
                << "-hls_list_size" << "0"
                << "-hls_segment_filename" << "stream_%03d.ts" << "playlist.m3u8";
    //arguments << "-f avfoundation -framerate 30 -r 30 -pixel_format uyvy422 -i 0:0 -c:v h264 -c:a aac -hls_time 10 -hls_list_size 0 -hls_segment_filename stream_%03d.ts playlist.m3u8";
#else
#endif
    
    connect( m_ffmpegStreamingProcess, &QProcess::errorOccurred, this, [] ( QProcess::ProcessError error ) {
        qDebug() << LOG_SOURCE << "🟠 errorOccurred: " << error;
    });
    connect( m_ffmpegStreamingProcess, &QProcess::started, this, [] () {
        qDebug() << LOG_SOURCE << "🟢 started! ";
    });
    connect( m_ffmpegStreamingProcess, &QProcess::stateChanged, this, [] (QProcess::ProcessState state) {
        qDebug() << LOG_SOURCE << "🟢 state: " << state;
    });
    connect( m_ffmpegStreamingProcess, &QProcess::finished, this, [] (int exitCode) {
        qDebug() << LOG_SOURCE << "🟢 finished: " << exitCode;
    });
    connect( m_ffmpegStreamingProcess, &QProcess::finished, this, [] (int exitCode) {
        qDebug() << LOG_SOURCE << "🟢 finished: " << exitCode;
    });
    connect( m_ffmpegStreamingProcess, &QProcess::readyReadStandardOutput, this, [this] () {
        while( m_ffmpegStreamingProcess->canReadLine()){
               qDebug() << "🔵🔵 " << m_ffmpegStreamingProcess->readLine();
        }
    });
    connect( m_ffmpegStreamingProcess, &QProcess::readyReadStandardError, this, [this] () {
        QString err = m_ffmpegStreamingProcess->readAllStandardError();
        qDebug() << "🟠🟠 " << err;
    });

    qDebug() << LOG_SOURCE << "🔵 Start ffmpeg; workingFolder: " << workingFolder;
    m_ffmpegStreamingProcess->setWorkingDirectory( QString::fromStdString(workingFolder) );
    qDebug() << LOG_SOURCE << "🔵 Start ffmpeg; program: " << program;
    qDebug() << LOG_SOURCE << "🔵 Start ffmpeg; arguments: " << arguments;
    m_ffmpegStreamingProcess->start( program, arguments );
//    m_ffmpegStreamingProcess->start( "echo", arguments2 );
    return;

    auto rowList = ui->m_streamAnnouncementTable->selectionModel()->selectedRows();
    if ( rowList.count() > 0 )
    {
        StreamInfo streamInfo;
        
        int rowIndex = rowList.constFirst().row();
        try {
            streamInfo = m_model->streamerAnnouncements().at(rowIndex);
        } catch (...) {
            return;
        }
    
        QMessageBox msgBox;
        const QString message = QString::fromStdString("'" + streamInfo.m_title + "' will be started.");
        msgBox.setText(message);
        msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
        auto reply = msgBox.exec();

        if ( reply == QMessageBox::Ok )
        {
            auto* drive = currentStreamingDrive();

            if ( drive != nullptr )
            {
                auto streamFolder = fs::path( drive->getLocalFolder() ) / STREAM_ROOT_FOLDER_NAME / streamInfo.m_uniqueFolderName;
                std::error_code ec;
                fs::remove_all( streamFolder, ec );
                qWarning() << "remove: " << streamFolder.string().c_str() << " ec:" << ec.message().c_str();

                if ( !ec )
                {
                    sirius::drive::ActionList actionList;
                    auto streamFolder = fs::path( STREAM_ROOT_FOLDER_NAME ) / streamInfo.m_uniqueFolderName;
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
}

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
void MainWin::onStartStreamingBtn()
{
    auto rowList = ui->m_streamAnnouncementTable->selectionModel()->selectedRows();
    if ( rowList.count() > 0 )
    {
        int rowIndex = rowList.constFirst().row();
        try {
            auto streamAnnouncements = m_model->streamerAnnouncements();
            StreamInfo& streamInfo = m_model->streamerAnnouncements().at(rowIndex);
            qDebug() << LOG_SOURCE << "🔵 streamInfo: " << streamInfo.m_annotation;

            auto driveKey = currentStreamingDriveKey().toStdString();
            
            if ( ! boost::iequals( streamInfo.m_driveKey, driveKey ) )
            {
                qWarning() << LOG_SOURCE << "🔴 invalid streamInfo: invalid driveKey" << streamInfo.m_driveKey;

                QMessageBox msgBox;
                msgBox.setText( QString::fromStdString( "Invalid drive key" ) );
                msgBox.setInformativeText( "Continue anyway" );
                msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
                int reply = msgBox.exec();
                if ( reply == QMessageBox::Cancel )
                {
                    return;
                }
            }

            auto* drive = m_model->findDrive( driveKey );
            if ( drive == nullptr )
            {
                qWarning() << LOG_SOURCE << "🔴 ! internal error: not found drive: " << driveKey;
                return;
            }

            // m3u8StreamFolder - for all streams
            auto m3u8StreamFolder = fs::path( ui->m_streamFolder->text().trimmed().toStdString() ) / streamInfo.m_uniqueFolderName;
            
            if ( m3u8StreamFolder.empty() )
            {
                qCritical() << LOG_SOURCE << "🔴 ! m3u8StreamFolder.empty()";

                QMessageBox msgBox;
                msgBox.setText( QString::fromStdString( "Invalid stream folder is not set" ) );
                msgBox.setInformativeText( "Stream folder is not set" );
                msgBox.setStandardButtons( QMessageBox::Ok );
                msgBox.exec();
                return;
            }

            std::error_code ec;
            if ( ! fs::exists( m3u8StreamFolder, ec ) )
            {
                qDebug() << LOG_SOURCE << "🔴 Stream folder does not exist" << streamInfo.m_driveKey;

                QMessageBox msgBox;
                msgBox.setText( QString::fromStdString( "Stream folder does not exist" ) );
                msgBox.setInformativeText( "Create folder ?" );
                msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
                int reply = msgBox.exec();
                if ( reply == QMessageBox::Cancel )
                {
                    return;
                }
                
                fs::create_directories( m3u8StreamFolder, ec );
                if ( ec )
                {
                    qCritical() << LOG_SOURCE << "🔴 ! cannot create folder : " << m3u8StreamFolder << " error:" << ec.message();

                    QMessageBox msgBox;
                    msgBox.setText( QString::fromStdString( "Cannot create folder" ) );
                    msgBox.setInformativeText( QString::fromStdString( ec.message() ) );
                    msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
                    msgBox.exec();
                    return;
                }
            }

            auto driveKeyHex = rawHashFromHex( driveKey.c_str() );
            
            uint64_t  expectedUploadSizeMegabytes = 200; // could be extended
            uint64_t feedbackFeeAmount = 100; // now, not used, it is amount of token for replicator
            auto uniqueStreamFolder  = fs::path( drive->getLocalFolder() ) / STREAM_ROOT_FOLDER_NAME / streamInfo.m_uniqueFolderName;
            auto chuncksFolder = uniqueStreamFolder / "chunks";
            auto torrentsFolder = uniqueStreamFolder / "torrents";
            
            if ( ! fs::exists( chuncksFolder, ec ) )
            {
                fs::create_directories( chuncksFolder, ec );
                if ( ec )
                {
                    qCritical() << LOG_SOURCE << "🔴 ! cannot create folder : " << chuncksFolder.string() << " error:" << ec.message();
                    return;
                }
            }
            if ( ! fs::exists( torrentsFolder, ec ) )
            {
                fs::create_directories( torrentsFolder, ec );
                if ( ec )
                {
                    qCritical() << LOG_SOURCE << "🔴 ! cannot create folder : " << torrentsFolder.string() << " error:" << ec.message();
                    return;
                }
            }

            fs::path m3u8Playlist = m3u8StreamFolder / "playlist.m3u8";
            sirius::Hash256 todoStreamHash;
            sirius::drive::ReplicatorList replicatorList = drive->getReplicators();

            startFfmpegStreamingProcess( m3u8StreamFolder.string() );
            return;
            
            endpoint_list endPointList = drive->getEndpointReplicatorList();
            if ( endPointList.empty() )
            {
                qWarning() << LOG_SOURCE << "🔴 ! endPointList.empty() !";
                return;
            }

            gStorageEngine->startStreaming( todoStreamHash,
                                           driveKeyHex, m3u8Playlist,
                                           chuncksFolder,
                                           torrentsFolder,
                                           endPointList );
            
            //
            //std::string txHashString = m_onChainClient->streamStart( driveKeyHex, streamFolder.string(), expectedUploadSizeMegabytes, feedbackFeeAmount );
            std::string txHashString = drive->getKey();
            auto txHash = rawHashFromHex( txHashString.c_str() );
            //auto txHash = rawHashFromHex( drive->getKey().c_str() );
            drive->setModificationHash( txHash, true );
            drive->updateDriveState(registering);
//            drive->updateDriveState(uploading);
        }
        catch (...) {
            return;
        }
    }

//    auto drive = m_model->currentDrive();
//    if (!drive) {
//        qWarning() << "MainWin::onApplyChanges. Unknown drive (Invalid pointer to current drive)";
//        return;
//    }
//
//    auto actionList = drive->getActionsList();
//    if ( actionList.empty() )
//    {
//        qWarning() << LOG_SOURCE << "no actions to apply";
//        const QString message = "There is no difference.";
//        showNotification(message);
//        addNotification(message);
//        return;
//    }
//
//    auto driveKeyHex = rawHashFromHex(drive->getKey().c_str());
//    m_onChainClient->applyDataModification(driveKeyHex, actionList);
//    drive->updateState(registering);
}

void MainWin::dbg()
{
    m_model->requestModificationStatus( "CA428CB868CF0DBFB9896FEF3A89E8F482A0A9F90BC29DC8C5DE01CCEFE7BAD1",
                                        "0100000000050607080900010203040506070809000102030405060708090001",
                                        "AB0F0F0F00000000000000000000000000000000000000000000000000000000" );
}

//        if ( Model::homeFolder() == "/Users/alex" )
//        {
//            QTimer::singleShot(10, this, [] {});
//        }
