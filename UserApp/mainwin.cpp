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

    if ( ! m_mustExit )
    {
        setupDownloadsTab();

        gStorageEngine = std::make_shared<StorageEngine>();
        gStorageEngine->start();
    }

    if ( gSettings.config().m_currentChannelIndex >= 0 )
    {
        ui->m_channels->setCurrentIndex( gSettings.config().m_currentChannelIndex );
        onChannelChanged(gSettings.config().m_currentChannelIndex);
    }
}

void MainWin::setupDownloadsTab()
{
#ifdef STANDALONE_TEST
    gSettings.config().m_channels.clear();
    gSettings.config().m_channels.push_back( Settings::ChannelInfo{
                                                "0101010100000000000000000000000000000000000000000000000000000000",
                                                "0100000000050607080900010203040506070809000102030405060708090001",
                                                "my_channel" } );
    gSettings.config().m_channels.push_back( Settings::ChannelInfo{
                                                "0202020200000000000000000000000000000000000000000000000000000000",
                                                "0200000000050607080900010203040506070809000102030405060708090001",
                                                "my_channel2" } );
    gSettings.config().m_currentChannelIndex = 0;
#endif

    setupFsTreeTable();
    setupDownloadsTable();
    updateChannelsCBox();

    connect( ui->m_channels, &QComboBox::currentIndexChanged, this, [this] (int index)
    {
        qDebug() << index;
        onChannelChanged( index );
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
        ui->m_fsTreeTableView->selectRow( index.row() );
    });

    connect( ui->m_fsTreeTableView, &QTableView::clicked, this, [this] (const QModelIndex &index)
    {
        ui->m_fsTreeTableView->selectRow( index.row() );
    });

    m_fsTreeTableModel = new FsTreeTableModel();

    ui->m_fsTreeTableView->setModel( m_fsTreeTableModel );
    ui->m_fsTreeTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_fsTreeTableView->horizontalHeader()->hide();
    ui->m_fsTreeTableView->setGridStyle( Qt::NoPen );

//#ifdef STANDALONE_TEST
//    sirius::drive::FsTree fsTree;
//    fsTree.addFolder( "f1" );
//    fsTree.addFolder( "f2" );
//    fsTree.addFolder( "f2/f3" );
//    fsTree.addFile( "", "file1", sirius::drive::InfoHash(), 1024*1024 );
//    fsTree.addFile( "", "file2", sirius::drive::InfoHash(), 1024*1024 );
//    fsTree.addFile( "", "file3", sirius::drive::InfoHash(), 1024*1024 );
//    fsTree.addFile( "f2", "file21", sirius::drive::InfoHash(), 1024*1024 );
//    fsTree.addFile( "f2", "file22", sirius::drive::InfoHash(), 1024*1024 );
//    fsTree.addFile( "f2", "file23", sirius::drive::InfoHash(), 1024*1024 );
//    fsTree.addFile( "f2/f3", "file21", sirius::drive::InfoHash(), 1024*1024 );
//    fsTree.addFile( "f2/f3", "file22", sirius::drive::InfoHash(), 1024*1024 );
//    fsTree.addFile( "f2/f3", "file23", sirius::drive::InfoHash(), 1024*1024 );
//    m_fsTreeTableModel->setFsTree( fsTree, {} );
//#endif

    ui->m_fsTreeTableView->update();
    ui->m_path->setText( "Path: " + QString::fromStdString(m_fsTreeTableModel->currentPath()));
}

void MainWin::setupDownloadsTable()
{
#ifdef STANDALONE_TEST
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f1", "~/Downloads", 100});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f2", "~/Downloads", 50});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f3", "~/Downloads", 25});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f4", "~/Downloads", 0});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f11", "~/Downloads", 100});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f21", "~/Downloads", 50});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f31", "~/Downloads", 25});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f41", "~/Downloads", 0});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f12", "~/Downloads", 100});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f22", "~/Downloads", 50});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f32", "~/Downloads", 25});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f42", "~/Downloads", 0});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f13", "~/Downloads", 100});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f23", "~/Downloads", 50});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f33", "~/Downloads", 25});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f43", "~/Downloads", 0});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f1", "~/Downloads", 100});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f2", "~/Downloads", 50});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f3", "~/Downloads", 25});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f4", "~/Downloads", 0});

#endif

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

MainWin::~MainWin()
{
    delete ui;
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

    //#todo request FsTree hash
#ifdef STANDALONE_TEST
    std::thread( [this,index]
    {
        qDebug() << "onChannelChanged: " << index;

        std::array<uint8_t,32> fsTreeHash;

        if ( index == 0 )
        {
            sirius::utils::ParseHexStringIntoContainer( //session_log_alert: CONNECTION FAILED:!!!! "ae1661b44791616368085b35b5ab142f05b52e6bb15a96f3606ed5bfbf4293b2",
                                                        "92fe3c62d8e38fdd57d40a55a89961b5add872b5bf3314b124401631ecd73e0c",
                                                        64, fsTreeHash );
        }
        else if ( index == 1 )
        {
            sirius::utils::ParseHexStringIntoContainer( "3bf8ac3aca14dd8ae346a267345565fd0fcfedc67dc7a57ef5c6305ffed7e69c",
                                                        64, fsTreeHash );
        }

        sleep(1);
        onFsTreeHashReceived( gSettings.config().m_channels[index].m_hash, fsTreeHash );
    }).detach();
#endif
}

void MainWin::onFsTreeHashReceived( const std::string& channelHash, const std::array<uint8_t,32>& fsTreeHash )
{
    std::lock_guard<std::mutex> channelsLock( gChannelsMutex );

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

        std::unique_lock<std::mutex> channelsLock( gChannelsMutex );

        auto channelInfo = gSettings.currentChannelInfoPtr();
        if ( channelInfo != nullptr )
        {
            channelInfo->m_tmpRequestingFsTreeTorrent.reset();
            channelInfo->m_waitingFsTree = false;
            channelsLock.unlock();

            m_fsTreeTableModel->setFsTree( fsTree, {} );
        }
    });
}

