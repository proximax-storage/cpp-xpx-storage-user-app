#include "mainwin.h"
#include "./ui_mainwin.h"

#include "Dialogs/AddStreamAnnouncementDialog.h"
#include "Dialogs/AddDownloadChannelDialog.h"

#include "Models/Model.h"
#include "Entities/StreamInfo.h"
#include "Dialogs/ModifyProgressPanel.h"
#include "Dialogs/StreamingView.h"
#include "Dialogs/StreamingPanel.h"
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

StreamInfo* MainWin::selectedStreamInfo() const
{
    auto rowList = ui->m_streamAnnouncementTable->selectionModel()->selectedRows();
    if ( rowList.count() > 0 )
    {
        try
        {
            int rowIndex = rowList.constFirst().row();
            return &m_model->streamerAnnouncements().at(rowIndex);
        }
        catch (...) {}
    }
    return nullptr;
}

void MainWin::initStreaming()
{
    //todo remove streamerAnnouncements from serializing
    m_model->streamerAnnouncements().clear();

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
    connect(ui->m_delStreamAnnouncementBtn, &QPushButton::released, this, [this] ()
    {
        if ( StreamInfo* streamInfo = selectedStreamInfo(); streamInfo != nullptr )
        {
            QMessageBox msgBox;
            const QString message = QString::fromStdString("'" + streamInfo->m_title + "' will be removed.");
            msgBox.setText(message);
            msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
            auto reply = msgBox.exec();

            if ( reply == QMessageBox::Ok )
            {
                auto* drive = currentStreamingDrive();

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
        if ( StreamInfo* streamInfo = selectedStreamInfo(); streamInfo != nullptr )
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
        if ( StreamInfo* streamInfo = selectedStreamInfo(); streamInfo != nullptr )
        {
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

void MainWin::readStreamingAnnotations( const Drive&  driveInfo )
{
    bool todoShouldBeSync = false;
    std::vector<StreamInfo>& streamInfoVector = m_model->streamerAnnouncements();

    streamInfoVector.clear();

    // read from disc (.videos/*/info)
    {
        auto path = fs::path( driveInfo.getLocalFolder() + "/" + STREAM_ROOT_FOLDER_NAME);

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
        ui->m_streamAnnouncementTable->insertRow( (int)rowIndex );
        {
            QDateTime dateTime = QDateTime::currentDateTime();
            dateTime.setSecsSinceEpoch( streamInfo.m_secsSinceEpoch );
            ui->m_streamAnnouncementTable->setItem( (int)rowIndex, 0, new QTableWidgetItem( dateTime.toString() ) );
        }
        {
            auto* item = new QTableWidgetItem();
//            else if ( streamInfo.m_streamingStatus == StreamInfo::ss_running )
//            {
//                item->setData( Qt::DisplayRole, QString::fromStdString( streamInfo.m_title + " (running...)") );
//            }
//            else
//            {
                item->setData( Qt::DisplayRole, QString::fromStdString( streamInfo.m_title ) );
//            }
            ui->m_streamAnnouncementTable->setItem( (int)rowIndex, 1, item );
        }
        {
            ui->m_streamAnnouncementTable->setItem( (int)rowIndex, 2, new QTableWidgetItem( QString::fromStdString( streamInfo.m_uniqueFolderName ) ) );
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

        AddDownloadChannelDialog dialog( m_onChainClient, m_model, this, streamInfo->m_driveKey );
        connect( m_onChainClient->getDialogSignalsEmitter(), &DialogSignals::addDownloadChannel, this, &MainWin::addChannel );
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

#pragma mark --Chain-Interface--
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

void MainWin::startStreamingProcess( const StreamInfo& streamInfo )
{
    try
    {
        qDebug() << LOG_SOURCE << "🔵 streamInfo: " << streamInfo.m_annotation;

        auto driveKey = currentStreamingDriveKey().toStdString();

        //
        // Check drive key
        //
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

        //
        // Get 'drive'
        //
        auto* drive = m_model->findDrive( driveKey );
        if ( drive == nullptr )
        {
            qWarning() << LOG_SOURCE << "🔴 ! internal error: not found drive: " << driveKey;
            return;
        }

        //
        // Get 'm3u8StreamFolder' - where OBS saves chancks and playlist
        //
        auto m3u8StreamFolder = fs::path( ObsProfileData().m_recordingPath );

        if ( m3u8StreamFolder.empty() )
        {
            qCritical() << LOG_SOURCE << "🔴 ! m3u8StreamFolder.empty()";

            QMessageBox msgBox;
            msgBox.setText( QString::fromStdString( "Invalid OBS profile 'Sirius-stream'" ) );
            msgBox.setInformativeText( "'Recording Path' is not set" );
            msgBox.setStandardButtons( QMessageBox::Ok );
            msgBox.exec();
            return;
        }

        std::error_code ec;
        if ( ! fs::exists( m3u8StreamFolder, ec ) )
        {
            qDebug() << LOG_SOURCE << "🔴 Stream folder does not exist" << streamInfo.m_driveKey;
            qDebug() << LOG_SOURCE << "🔴 Stream folder does not exist" << m3u8StreamFolder.string();

//                QMessageBox msgBox;
//                msgBox.setText( QString::fromStdString( "Stream folder does not exist" ) );
//                msgBox.setInformativeText( "Create folder ?" );
//                msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
//                int reply = msgBox.exec();
//                if ( reply == QMessageBox::Cancel )
//                {
//                    return;
//                }

            fs::create_directories( m3u8StreamFolder, ec );
            if ( ec )
            {
                qCritical() << LOG_SOURCE << "🔴 ! cannot create folder : " << m3u8StreamFolder.string() << " error:" << ec.message();

                QMessageBox msgBox;
                msgBox.setText( QString::fromStdString( "Cannot create folder: " + m3u8StreamFolder.string() ) );
                msgBox.setInformativeText( QString::fromStdString( ec.message() ) );
                msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
                msgBox.exec();
                return;
            }
        }

        auto driveKeyHex = rawHashFromHex( driveKey.c_str() );

        uint64_t  expectedUploadSizeMegabytes = 200; // could be extended
        uint64_t feedbackFeeAmount = 100; // now, not used, it is amount of token for replicator
        auto uniqueStreamFolder  = fs::path( drive->getLocalFolder() + "/" + STREAM_ROOT_FOLDER_NAME + "/" + streamInfo.m_uniqueFolderName);
        auto chuncksFolder = uniqueStreamFolder / "chunks";
        auto torrentsFolder = m3u8StreamFolder / "torrents";

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

        endpoint_list endPointList = drive->getEndpointReplicatorList();
        if ( endPointList.empty() )
        {
            qWarning() << LOG_SOURCE << "🔴 ! endPointList.empty() !";
            return;
        }

        fs::path m3u8Playlist = fs::path(m3u8StreamFolder.string()) / PLAYLIST_FILE_NAME;
        sirius::drive::ReplicatorList replicatorList = drive->getReplicators();

        m_model->setCurrentStreamInfo( streamInfo );

        //TODO!!!
//            {
//                delete m_streamingView;
//                m_streamingView = new StreamingView( [this] {cancelStreaming();}, [this] {finishStreaming();}, this );
//                m_streamingView->setWindowModality(Qt::WindowModal);
//                m_streamingView->show();
//                //startFfmpegStreamingProcess();
//                return;
//            }

#if 1
        auto callback = [=,model = m_model](std::string txHashString) {
            auto* drive = model->findDrive( driveKey );
            if ( drive == nullptr )
            {
                qWarning() << LOG_SOURCE << "🔴 ! internal error: not found drive: " << driveKey;
                return;
            }

            qDebug () << "streamStart: txHashString: " << txHashString.c_str();
            auto txHash = rawHashFromHex( txHashString.c_str() );
#else
        // OFF-CHAIN
            std::array<uint8_t, 32> txHash;
#endif

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
                                           uniqueStreamFolder.string(),
                                           driveKeyHex,
                                           m3u8Playlist,
                                           drive->getLocalFolder(),
                                           torrentsFolder,
                                           [](const std::string&){},
                                           endPointList );

            drive->setModificationHash( txHash, true );
//              drive->updateDriveState(canceled); //todo
//              drive->updateDriveState(no_modifications); //todo
            drive->updateDriveState(registering);
        };

        m_onChainClient->streamStart( driveKeyHex, streamInfo.m_uniqueFolderName, expectedUploadSizeMegabytes, feedbackFeeAmount, callback );
    }
    catch (...) {
        return;
    }
}

void MainWin::finishStreaming()
{
    auto streamInfo = m_model->currentStreamInfo();
    assert( streamInfo );
    
    auto driveKey = streamInfo->m_driveKey;

    gStorageEngine->finishStreaming( driveKey, [=,this]( const sirius::Key& driveKey, const sirius::drive::InfoHash& streamId, const sirius::drive::InfoHash& actionListHash, uint64_t streamBytes )
    {
        // Execute code on the main thread
        QTimer::singleShot( 0, this, [=,this]
        {
            qDebug() << "info.streamSizeBytes: " << streamBytes;
            uint64_t actualUploadSizeMegabytes = (streamBytes+(1024*1024-1))/(1024*1024);
            qDebug() << "actualUploadSizeMegabytes: " << actualUploadSizeMegabytes;
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
    assert( drive );

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
        if ( auto drive = m_model->findDriveByModificationId( tx ); drive != nullptr )
        {
            drive->updateDriveState(failed);
            drive->setIsStreaming(false);
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
        drive->setIsStreaming(false);
        drive->updateDriveState(canceled);

    }, Qt::QueuedConnection);

    // cancelModificationTransactionFailed
    connect( m_onChainClient, &OnChainClient::cancelModificationTransactionFailed, this,
        [this] ( const std::array<uint8_t, 32> &driveId, const QString &modificationId )
    {
        m_model->setCurrentStreamInfo({});
    }, Qt::QueuedConnection);

    // streamFinishTransactionConfirmed
    connect( m_onChainClient, &OnChainClient::streamFinishTransactionConfirmed, this,
        [this] ( const std::array<uint8_t, 32> &streamId )
    {
    }, Qt::QueuedConnection);

    // streamFinishTransactionFailed
    connect( m_onChainClient, &OnChainClient::streamFinishTransactionFailed, this,
        [this] ( const std::array<uint8_t, 32> &streamId, const QString& errorText )
    {
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
