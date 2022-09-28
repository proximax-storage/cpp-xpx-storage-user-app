#include "mainwin.h"
#include "./ui_mainwin.h"
#include "FsTreeTableModel.h"
#include "DownloadsTableModel.h"

#include "Settings.h"
#include "SettingsDialog.h"
#include "PrivKeyDialog.h"
#include "StorageEngine.h"

#include "crypto/Signer.h"
#include "drive/Utils.h"
#include "utils/HexParser.h"

#include <QFileIconProvider>
#include <QScreen>
#include <QScroller>
#include <QComboBox>
#include <QMessageBox>

#include <random>
#include <thread>

MainWin* MainWin::m_instanse = nullptr;

MainWin::MainWin(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWin)
{
    assert( m_instanse == nullptr );
    m_instanse = this;

    ui->setupUi(this);

    connect( ui->m_settingsBtn, SIGNAL (released()), this, SLOT( onSettingsBtn() ));

    if ( ! gSettings.load("") )
    {
        if ( ! requestPrivateKey() )
        {
            m_mustExit = true;
        }
    }

    if ( m_mustExit )
    {
        return;
    }

    setupDownloadsTab();

    gStorageEngine = std::make_shared<StorageEngine>();
    gStorageEngine->start();

    if ( gSettings.config().m_currentChannelIndex >= 0 )
    {
        ui->m_channels->setCurrentIndex( gSettings.config().m_currentChannelIndex );
        onChannelChanged(gSettings.config().m_currentChannelIndex);
    }

    m_downloadUpdateTimer = new QTimer();
    connect( m_downloadUpdateTimer, SIGNAL(timeout()), this, SLOT( onDownloadUpdateTimer() ));
    m_downloadUpdateTimer->start(500); // 2 times per second
}

MainWin::~MainWin()
{
    delete ui;
}

void MainWin::onDownloadUpdateTimer()
{
    m_downloadsTableModel->updateProgress();
}

void MainWin::onDownloadCompleted( QString hash )
{
    auto infoHash = sirius::drive::stringToByteArray<sirius::drive::InfoHash>( hash.toStdString() );

    m_downloadsTableModel->onDownloadCompleted( infoHash.array() );
}

void MainWin::setupDownloadsTab()
{
    if ( STANDALONE_TEST )
    {
        gSettings.config().m_channels.clear();
        gSettings.config().m_channels.push_back( Settings::ChannelInfo{
                                                    "my_channel",
                                                    "0101010100000000000000000000000000000000000000000000000000000000",
                                                    "0100000000050607080900010203040506070809000102030405060708090001"
                                                     } );
        gSettings.config().m_channels.push_back( Settings::ChannelInfo{
                                                    "my_channel2",
                                                    "0202020200000000000000000000000000000000000000000000000000000000",
                                                    "0200000000050607080900010203040506070809000102030405060708090001"
                                                     } );
        gSettings.config().m_currentChannelIndex = 0;
    }

    setupFsTreeTable();
    setupDownloadsTable();
    updateChannelsCBox();

    connect( ui->m_channels, &QComboBox::currentIndexChanged, this, [this] (int index)
    {
        qDebug() << index;
        onChannelChanged( index );
    });

    connect( ui->m_downloadBtn, &QPushButton::released, this, [this] ()
    {
        onDownloadBtn();
    });
}

void MainWin::setupFsTreeTable()
{
    connect( ui->m_fsTreeTableView, &QTableView::doubleClicked, this, [this] (const QModelIndex &index)
    {
        //qDebug() << index;
        int toBeSelectedRow = m_fsTreeTableModel->onDoubleClick( index.row() );
        ui->m_fsTreeTableView->selectRow( toBeSelectedRow );
        ui->m_path->setText( "Path: " + QString::fromStdString(m_fsTreeTableModel->currentPath()));
    });

    connect( ui->m_fsTreeTableView, &QTableView::pressed, this, [this] (const QModelIndex &index)
    {
        selectFsTreeItem( index.row() );
    });

    connect( ui->m_fsTreeTableView, &QTableView::clicked, this, [this] (const QModelIndex &index)
    {
        selectFsTreeItem( index.row() );
    });

    m_fsTreeTableModel = new FsTreeTableModel();

    ui->m_fsTreeTableView->setModel( m_fsTreeTableModel );
    ui->m_fsTreeTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_fsTreeTableView->horizontalHeader()->hide();
    ui->m_fsTreeTableView->setGridStyle( Qt::NoPen );

    ui->m_fsTreeTableView->update();
    ui->m_path->setText( "Path: " + QString::fromStdString(m_fsTreeTableModel->currentPath()));
}

void MainWin::selectFsTreeItem( int index )
{
    if ( index < 0 && index >= m_fsTreeTableModel->m_rows.size() )
    {
        return;
    }

    ui->m_fsTreeTableView->selectRow( index );

    ui->m_downloadBtn->setEnabled( ! m_fsTreeTableModel->m_rows[index].m_isFolder );
}

void MainWin::onDownloadBtn()
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto& downloads = gSettings.config().m_downloads;

    bool overrideFileForAll = false;

    auto selectedIndexes = ui->m_fsTreeTableView->selectionModel()->selectedRows();
    for( auto index: selectedIndexes )
    {
        int row = index.row();
        if ( row < 1 || row >= m_fsTreeTableModel->m_rows.size() )
        {
            continue;
        }

        const auto& hash = m_fsTreeTableModel->m_rows[row].m_hash;
        const auto& name = m_fsTreeTableModel->m_rows[row].m_name;
//        qDebug() << "onDownloadBtn: " << index << " " << row;
//        qDebug() << "onDownloadBtn: " << name.c_str();

        auto it = std::find_if( downloads.begin(), downloads.end(), [&name]( const auto& item )
        {
            return item.m_fileName == name;
        });

        fs::path filePath = gSettings.downloadFolder() / name;
        std::error_code ec;
        bool fileExist = fs::exists( filePath, ec );

        if ( overrideFileForAll )
        {
            //todo
        }
        else
        {
            if ( it != downloads.end() && ! it->isCompleted() )
            {
                Settings::DownloadInfo dnInfoCopy = *it; // item could be removed?
                lock.unlock();

                QMessageBox msgBox;
                msgBox.setText( QString::fromStdString( "File with same name '" + name + "' is currently downloading" ) );
                msgBox.setInformativeText( "Do you want to replace this download with the current request" );
                msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
                int reply = msgBox.exec();

                if ( reply == QMessageBox::Cancel )
                {
                    return;
                }
                else
                {
                    gSettings.removeFromDownloads( dnInfoCopy );
                }
                lock.lock();
            }
            else if ( it != downloads.end() )
            {
                Settings::DownloadInfo dnInfoCopy = *it; // item could be removed?
                lock.unlock();

                QMessageBox msgBox;
                msgBox.setText( QString::fromStdString( "File with same name '" + name + "' is already downloaded" ) );
                msgBox.setInformativeText( "Do you want to override it" );
                msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
                int reply = msgBox.exec();

                if ( reply == QMessageBox::Cancel )
                {
                    return;
                }
                else
                {
                    gSettings.removeFromDownloads( dnInfoCopy );
                }
                lock.lock();
            }
            else if ( fileExist )
            {
                lock.unlock();
                QMessageBox msgBox;
                msgBox.setText( QString::fromStdString( "File with same name '" + name + "' is already exist" ) );
                msgBox.setInformativeText( "Do you want to override it" );
                msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
                int reply = msgBox.exec();

                if ( reply == QMessageBox::Cancel )
                {
                    return;
                }
                else
                {
                    //???
                }
                lock.lock();
            }


        }

        auto channelId = gSettings.currentChannelInfoPtr();
        if ( channelId != nullptr )
        {
            auto ltHandle = gStorageEngine->downloadFile( *channelId,  hash, name );
            m_downloadsTableModel->beginResetModel();
            gSettings.config().m_downloads.emplace_back( Settings::DownloadInfo{ hash, name,
                                                                                 gSettings.downloadFolder(),
                                                                                 0, ltHandle } );
            m_downloadsTableModel->endResetModel();
        }
    }
}

void MainWin::setupDownloadsTable()
{
    connect( ui->m_downloadsTableView, &QTableView::doubleClicked, this, [this] (const QModelIndex &index)
    {
        qDebug() << index;
        ui->m_downloadsTableView->selectRow( index.row() );
    });

//    connect( ui->m_downloadsTableView, &QTableView::pressed, this, [this] (const QModelIndex &index)
//    {
//        ui->m_downloadsTableView->selectRow( index.row() );
//    });

//    connect( ui->m_downloadsTableView, &QTableView::clicked, this, [this] (const QModelIndex &index)
//    {
//        ui->m_downloadsTableView->selectRow( index.row() );
//    });

    m_downloadsTableModel = new DownloadsTableModel();

    ui->m_downloadsTableView->setModel( m_downloadsTableModel );
    ui->m_downloadsTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_downloadsTableView->horizontalHeader()->hide();
    ui->m_downloadsTableView->setGridStyle( Qt::NoPen );
    ui->m_downloadsTableView->setSelectionMode( QAbstractItemView::NoSelection );

    ui->m_downloadsTableView->update();
}

void MainWin::onSettingsBtn()
{
    SettingsDialog settingsDialog( this );
    settingsDialog.exec();
}

bool MainWin::requestPrivateKey()
{
    {
        PrivKeyDialog pKeyDialog( this, gSettings );

        pKeyDialog.exec();

        if ( pKeyDialog.result() != QDialog::Accepted )
        {
            qDebug() << "not accepted";
            return false;
        }
        qDebug() << "accepted";
    }

    SettingsDialog settingsDialog( this, true );
    settingsDialog.exec();

    if ( settingsDialog.result() != QDialog::Accepted )
    {
        return false;
    }

    return true;
}

void MainWin::initDriveTree()
{
    QFileSystemModel* m_model = new QFileSystemModel();
    m_model->setRootPath("~/");
    ui->m_driveTree->setModel(m_model);
    ui->m_driveTree->header()->resizeSection(0, 320);
    ui->m_driveTree->header()->resizeSection(1, 30);
    ui->m_driveTree->header()->setDefaultAlignment(Qt::AlignLeft);
    ui->m_driveTree->header()->hideSection(2);
    ui->m_driveTree->header()->hideSection(3);

    ui->m_diffView->setColumnWidth(0,330);

//    ui->m_driveFiles->setColumnWidth(0,420);
}

void MainWin::updateChannelsCBox()
{
    ui->m_channels->clear();
    for( const auto& channelInfo: gSettings.config().m_channels )
    {
        ui->m_channels->addItem( QString::fromStdString( channelInfo.m_name) );
    }

//    onChannelChanged(0);
}

void MainWin::onChannelChanged( int index )
{
    gSettings.config().m_currentChannelIndex = index;
    gSettings.currentChannelInfo().m_waitingFsTree = true;
    m_fsTreeTableModel->setFsTree( {}, {} );


    if ( !STANDALONE_TEST )
    {
        assert( !"todo request FsTree hash" );
    }
    else
    {
        std::thread( [this,index]
        {
            qDebug() << "onChannelChanged: " << index;

            std::array<uint8_t,32> fsTreeHash;

            if ( index == 0 )
            {
                sirius::utils::ParseHexStringIntoContainer( "ce0cf74c024aec3fddc3f8a13e0a4504f6b75197878901c238dff5a3ed55171e",
                                                            64, fsTreeHash );
            }
            else if ( index == 1 )
            {
                sirius::utils::ParseHexStringIntoContainer( "a36e0f092c1b9d63ec08e04e1ce6b008b9df5bca6926a25d90180dad52f3da90",
                                                            64, fsTreeHash );
            }

            sleep(1);
            onFsTreeHashReceived( gSettings.config().m_channels[index].m_hash, fsTreeHash );
        }).detach();
    }
}

void MainWin::onFsTreeHashReceived( const std::string& channelHash, const std::array<uint8_t,32>& fsTreeHash )
{
    std::lock_guard<std::recursive_mutex> lock( gSettingsMutex );

    auto channelInfo = gSettings.currentChannelInfoPtr();
    if ( channelInfo == nullptr )
    {
        // ignore fsTreeHash for unexisting channel
        qDebug() << "ignore fsTreeHash for unexisting channel";

        return;
    }

    gStorageEngine->downloadFsTree( *channelInfo, fsTreeHash, [this] ( const std::string&           channelHash,
                                                                       const std::array<uint8_t,32> fsTreeHash,
                                                                       const sirius::drive::FsTree& fsTree )
    {
        qDebug() << "m_fsTreeTableModel->setFsTree( fsTree, {} );";

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        auto channelInfo = gSettings.currentChannelInfoPtr();
        if ( channelInfo != nullptr )
        {
            channelInfo->m_tmpRequestingFsTreeTorrent.reset();
            channelInfo->m_waitingFsTree = false;
            lock.unlock();

            m_fsTreeTableModel->setFsTree( fsTree, {} );
        }
    });
}

