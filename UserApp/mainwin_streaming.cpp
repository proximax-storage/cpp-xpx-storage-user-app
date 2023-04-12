#include "mainwin.h"
#include "./ui_mainwin.h"

#include "AddStreamAnnouncementDialog.h"
#include "AddStreamRefDialog.h"
#include "Model.h"
#include "StreamInfo.h"
#include "ModifyProgressPanel.h"
#include "StorageEngine.h"

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
        streamInfo.parseLink(link);
        m_model->addStreamRef(streamInfo);
        updateViewerCBox();
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

void MainWin::updateStreamerTable()
{
    ui->m_streamAnnouncementTable->clearContents();
    const auto& announcements = m_model->streamerAnnouncements();

    qDebug() << "announcements: " << announcements.size();

    for( const auto& announcement: announcements )
    {
        size_t rowIndex = ui->m_streamAnnouncementTable->rowCount();
        ui->m_streamAnnouncementTable->insertRow( rowIndex );
        {
            QDateTime dateTime = QDateTime::currentDateTime();
            dateTime.setSecsSinceEpoch( announcement.m_secsSinceEpoch );
            ui->m_streamAnnouncementTable->setItem( rowIndex, 0, new QTableWidgetItem( dateTime.toString() ) );
        }
        {
            if ( auto* drive = m_model->findDrive( announcement.m_driveKey ); drive == nullptr )
            {
                ui->m_streamAnnouncementTable->setItem( rowIndex, 1, new QTableWidgetItem( "Removed drive" ) );
            }
            else
            {
                ui->m_streamAnnouncementTable->setItem( rowIndex, 1, new QTableWidgetItem( QString::fromStdString( announcement.m_localFolder ) ) );
            }
        }
        {
            QTableWidgetItem* item = new QTableWidgetItem();
            item->setData( Qt::DisplayRole, QString::fromStdString( announcement.m_title ) );
            ui->m_streamAnnouncementTable->setItem( rowIndex, 2, item );
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
        row = dateTime.toString() + " ❘ " + QString::fromStdString( streamRef.m_title );
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
    if ( m_model->m_viewerStatus != vs_no_viewing )
    {
        return;
    }
    
    const StreamInfo* streamRef = m_model->getStreamRef( ui->m_streamRefCBox->currentIndex() );
    if ( streamRef == nullptr )
    {
        return;
    }

    m_model->requestStreamStatus( *streamRef, [this] ( const sirius::drive::DriveKey& driveKey,
                                                       bool                           isStreaming,
                                                       const std::array<uint8_t,32>&  streamId )
    {
        onStreamStatusResponse( driveKey, isStreaming, streamId );
    });
    
    m_model->m_viewerStatus = vs_waiting_stream_start;
    m_startViewingProgressPanel->setWaitingStreamStart();
    m_startViewingProgressPanel->setVisible( true );
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

//        if ( Model::homeFolder() == "/Users/alex" )
//        {
//            QTimer::singleShot(10, this, [] {});
//        }
