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

#include <boost/algorithm/string.hpp>

void MainWin::initStreaming()
{
    // tabWidget_2
    connect(ui->tabWidget_2, &QTabWidget::currentChanged, this, [this](int index)
    {
        updateViewerProgressPanel( index );
        updateStreamerProgressPanel( index );
    });
            
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
        AddStreamAnnouncementDialog dialog(m_onChainClient, m_model, this);
        auto rc = dialog.exec();
        if ( rc )
        {
            auto streamFolderName = dialog.streamFolderName();
            updateStreamerTable();
        }
    }, Qt::QueuedConnection);

    // m_delStreamAnnouncementBtn
    connect(ui->m_delStreamAnnouncementBtn, &QPushButton::released, this, [this] () {
        auto rowList = ui->m_streamAnnouncementTable->selectionModel()->selectedRows();
        if ( rowList.count() > 0 )
        {
            int rowIndex = rowList.constFirst().row();
            std::string streamTitle;
            try {
                streamTitle = m_model->streamerAnnouncements().at(rowIndex).m_title;
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
                m_model->deleteStreamerAnnouncement( rowIndex );
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
        if ( rowIndex > 0 )
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
            }
        }
    }, Qt::QueuedConnection);

    // m_startViewingBtn
    connect(ui->m_startViewingBtn, &QPushButton::released, this, [this] () {
        startViewingStream();
    }, Qt::QueuedConnection);

    updateStreamerTable();
    updateViewerCBox();
}

void MainWin::readStreamingAnnotaions( std::vector<StreamInfo>& streamInfoVector )
{
    streamInfoVector.clear();
    
    const std::map<std::string, Drive>& drives = m_model->getDrives();
    for( const auto [key,driveInfo] : drives )
    {
        auto path = fs::path( driveInfo.getLocalFolder() ) / STREAM_ROOT_FOLDER_NAME;
        
        std::error_code ec;
        if ( ! fs::is_directory(path,ec) )
        {
            continue;
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
                    qWarning() << "Internal error: cannot read stream annotation: " << fileName;
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

void MainWin::updateStreamerTable( const std::string& streamFolderName )
{
    std::vector<StreamInfo> streamAnnotaions;
    readStreamingAnnotaions( streamAnnotaions );
    
    ui->m_streamAnnouncementTable->clearContents();

    qDebug() << "announcements: " << streamAnnotaions.size();

    for( const auto& streamInfo: streamAnnotaions )
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
            if ( streamInfo.m_streamingStatus != StreamInfo::ss_creating )
            {
                item->setData( Qt::DisplayRole, QString::fromStdString( streamInfo.m_title + " (creating...)") );
            }
            else if ( streamInfo.m_streamingStatus != StreamInfo::ss_registring )
            {
                item->setData( Qt::DisplayRole, QString::fromStdString( streamInfo.m_title + " (registring...)") );
            }
            else if ( streamInfo.m_streamingStatus != StreamInfo::ss_started )
            {
                item->setData( Qt::DisplayRole, QString::fromStdString( streamInfo.m_title + " (running...)") );
            }
            else
            {
                item->setData( Qt::DisplayRole, QString::fromStdString( streamInfo.m_title ) );
            }
            ui->m_streamAnnouncementTable->setItem( rowIndex, 1, item );
        }
        {
            ui->m_streamAnnouncementTable->setItem( rowIndex, 2, new QTableWidgetItem( QString::fromStdString( streamInfo.m_streamFolder ) ) );
        }
    }
    QHeaderView* header = ui->m_streamAnnouncementTable->horizontalHeader();
    header->setSectionResizeMode(0,QHeaderView::Stretch);
    header->setSectionResizeMode(1,QHeaderView::Stretch);
    header->setSectionResizeMode(2,QHeaderView::Stretch);
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
        m_startStreamingProgressPanel->setVisible(false);
        return;
    }
    
    if ( ui->m_streamingTabView->currentIndex() == 1 )
    {
        m_startStreamingProgressPanel->setVisible(true);
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

void MainWin::cancelStreaming()
{
    m_model->m_streamerStatus = ss_no_streaming;
    m_startStreamingProgressPanel->setVisible(false);
}

void MainWin::onFsTreeReceivedForStreamAnnotaions( const std::string& driveKey,
                                          const std::array<uint8_t,32>& fsTreeHash,
                                          const sirius::drive::FsTree& )
{
    
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
