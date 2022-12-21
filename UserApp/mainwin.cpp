//#include "moc_mainwin.cpp"

#include "mainwin.h"
#include "./ui_mainwin.h"

#include "Model.h"
#include "FsTreeTableModel.h"
#include "DownloadsTableModel.h"
#include "DriveTreeModel.h"
#include "DiffTableModel.h"

#include "QtGui/qclipboard.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "PrivKeyDialog.h"
#include "ChannelInfoDialog.h"
#include "DriveInfoDialog.h"
#include "AddDownloadChannelDialog.h"
#include "CloseChannelDialog.h"
#include "CancelModificationDialog.h"
#include "AddDriveDialog.h"
#include "CloseDriveDialog.h"
#include "DownloadPaymentDialog.h"
#include "StoragePaymentDialog.h"
#include "ReplicatorOnBoardingDialog.h"
#include "ReplicatorOffBoardingDialog.h"
#include "OnChainClient.h"
#include "ModifyProgressPanel.h"
#include "PopupMenu.h"
#include "EditDialog.h"

#include "crypto/Signer.h"
#include "utils/HexParser.h"

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

MainWin::MainWin(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWin)
{
    ui->setupUi(this);
}

void MainWin::init()
{
    if ( Model::homeFolder() != "/Users/alex" )
    {
        ALEX_LOCAL_TEST = false;
    }

    if ( Model::homeFolder() != "/home/cempl" )
    {
        VICTOR_LOCAL_TEST = false;
    }

    if ( ! Model::loadSettings() )
    {
        if ( Model::homeFolder() == "/Users/alex" )
        {
            gSettings.initForTests();
        }
        else if ( ! requestPrivateKey() )
        {
            m_mustExit = true;
        }
    }

    if ( m_mustExit )
    {
        return;
    }

    if ( Model::homeFolder() == "/Users/alex" )
    {
        if ( gSettings.config().m_accountName == "alex_local_test" )
        {
            gSettings.m_replicatorBootstrap = "192.168.2.101:5001";
            gSettings.setCurrentAccountIndex(2);
        }
        else
        {
            gSettings.m_replicatorBootstrap = "15.206.164.53:7904";
        }
    }

    // Clear old fsTrees
    //

    if ( fs::exists( fsTreesFolder() ) )
    {
        try {
             fs::remove_all( fsTreesFolder() );
        } catch( const std::runtime_error& err ) {
             QMessageBox msgBox(this);
             msgBox.setText( QString::fromStdString( "Cannot remove fsTreesFolder") );
             msgBox.setInformativeText( QString::fromStdString( "Error: " ) + err.what() );
             msgBox.setStandardButtons( QMessageBox::Ok );
             msgBox.exec();
        }
    }

    try {
        fs::create_directories( fsTreesFolder() );
    } catch( const std::runtime_error& err ) {
         QMessageBox msgBox(this);
         msgBox.setText( QString::fromStdString( "Cannot create fsTreesFolder") );
         msgBox.setInformativeText( QString::fromStdString( "Error: " ) + err.what() );
         msgBox.setStandardButtons( QMessageBox::Ok );
         msgBox.exec();
    }

    Model::startStorageEngine( [this]
    {
//         QMessageBox msgBox(this);
//         msgBox.setText( QString::fromStdString( "Address already in use") );
//         msgBox.setInformativeText( QString::fromStdString( "Port: " + gSettings.m_udpPort ) );
//         msgBox.setStandardButtons( QMessageBox::Ok );
//         msgBox.exec();

        qWarning() << LOG_SOURCE << "Address already in use: udoPort: " << gSettings.m_udpPort;
         exit(1);
    });

    setupIcons();
    setGeometry(gSettings.m_windowGeometry);

    connect( ui->m_settingsButton, &QPushButton::released, this, [this]
    {
        SettingsDialog settingsDialog( this );
        settingsDialog.exec();
    }, Qt::QueuedConnection);

    const std::string privateKey = gSettings.config().m_privateKeyStr;
    qDebug() << LOG_SOURCE << "privateKey: " << privateKey;
    const auto gatewayEndpoint = QString(gSettings.m_restBootstrap.c_str()).split(":");
    const std::string address = gatewayEndpoint[0].toStdString();
    const std::string port = gatewayEndpoint[1].toStdString();

    m_onChainClient = new OnChainClient(gStorageEngine.get(), privateKey, address, port, this);
    m_modificationsWatcher = new QFileSystemWatcher(this);

    connect(m_modificationsWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWin::onDriveLocalDirectoryChanged, Qt::QueuedConnection);
    connect(ui->m_addChannel, &QPushButton::released, this, [this] () {
        AddDownloadChannelDialog dialog(m_onChainClient, this);
        connect(&dialog, &AddDownloadChannelDialog::addDownloadChannel, this, &MainWin::addChannel);
        dialog.exec();
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::downloadChannelsLoaded, this,
            [this](OnChainClient::ChannelsType type, const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& channelsPages)
    {
        qDebug() << LOG_SOURCE << "downloadChannelsLoaded pages amount: " << channelsPages.size();
        if (type == OnChainClient::ChannelsType::MyOwn) {
            Model::onMyOwnChannelsLoaded(channelsPages);
        } else if (type == OnChainClient::ChannelsType::Sponsored) {
            Model::onSponsoredChannelsLoaded(channelsPages);
        }

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
        gSettings.config().m_channelsLoaded = true;
        m_channelFsTreeTableModel->update();

        int dnChannelIndex = Model::currentDnChannelIndex();
        if ( dnChannelIndex >= 0 && dnChannelIndex < Model::dnChannels().size() )
        {
            ui->m_channels->setCurrentIndex( dnChannelIndex );
        }
        else if ( dnChannelIndex < Model::dnChannels().size() > 0 )
        {
            ui->m_channels->setCurrentIndex( 0 );
        }
        lock.unlock();

        updateChannelsCBox();
        connect( ui->m_channels, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWin::onCurrentChannelChanged, Qt::QueuedConnection);
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::drivesLoaded, this, [this](const std::vector<xpx_chain_sdk::drives_page::DrivesPage>& drivesPages)
    {
        qDebug() << LOG_SOURCE << "drivesLoaded pages amount: " << drivesPages.size();

        // add drives created on another devices
        Model::onDrivesLoaded(drivesPages);

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
        gSettings.config().m_drivesLoaded = true;

        qDebug() << LOG_SOURCE << "m_currentDriveIndex: " << gSettings.config().m_currentDriveIndex;
        if ( gSettings.config().m_currentDriveIndex >= 0 )
        {
            Model::setCurrentDriveIndex( gSettings.config().m_currentDriveIndex );
            if ( auto* drivePtr = Model::currentDriveInfoPtr(); drivePtr != nullptr )
            {
                drivePtr->m_calclDiffIsWaitingFsTree = true;
                downloadLatestFsTree( drivePtr->m_driveKey );
            }
        }
        else
        {
            m_driveTreeModel->updateModel(false);
            ui->m_driveTreeView->expandAll();

            if ( auto* drive = Model::currentDriveInfoPtr(); drive != nullptr )
            {
                m_driveFsTreeTableModel->setFsTree( drive->m_fsTree, drive->m_lastOpenedPath );
            }
        }

        addLocalModificationsWatcher();
        updateDrivesCBox();
        updateModificationStatus();

        connect( ui->m_driveCBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWin::onCurrentDriveChanged, Qt::QueuedConnection);

        // load my own channels
        xpx_chain_sdk::DownloadChannelsPageOptions options;
        options.consumerKey = gSettings.config().m_publicKeyStr;
        lock.unlock();
        m_onChainClient->loadDownloadChannels(options);

        // load sponsored channels
        m_onChainClient->loadDownloadChannels({});
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::downloadChannelOpenTransactionConfirmed, this, [this](auto alias, auto channelKey, auto driveKey) {
        onChannelCreationConfirmed( alias, sirius::drive::toString( channelKey ), sirius::drive::toString(driveKey) );
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::downloadChannelOpenTransactionFailed, this, [this](auto& channelKey, auto& errorText) {
           onChannelCreationFailed( channelKey.toStdString(), errorText.toStdString() );
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::downloadChannelCloseTransactionConfirmed, this, &MainWin::onDownloadChannelCloseConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::downloadChannelCloseTransactionFailed, this, &MainWin::onDownloadChannelCloseFailed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::downloadPaymentTransactionConfirmed, this, &MainWin::onDownloadPaymentConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::downloadPaymentTransactionFailed, this, &MainWin::onDownloadPaymentFailed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::storagePaymentTransactionConfirmed, this, &MainWin::onStoragePaymentConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::storagePaymentTransactionFailed, this, &MainWin::onStoragePaymentFailed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::cancelModificationTransactionConfirmed, this, &MainWin::onCancelModificationTransactionConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::cancelModificationTransactionFailed, this, &MainWin::onCancelModificationTransactionFailed, Qt::QueuedConnection);

    connect(ui->m_closeChannel, &QPushButton::released, this, [this] () {
        auto channel = Model::currentChannelInfoPtr();
        if (!channel) {
            qWarning() << LOG_SOURCE << "bad channel";
            return;
        }

        CloseChannelDialog dialog(m_onChainClient, channel->m_hash.c_str(), channel->m_name.c_str(), this);
        auto rc = dialog.exec();
        if ( rc==QMessageBox::Ok )
        {
            channel->m_isDeleting = true;
            updateChannelsCBox();
            lockChannel(channel->m_hash);
        }
    }, Qt::QueuedConnection);

    connect(ui->m_addDrive, &QPushButton::released, this, [this] () {
        AddDriveDialog dialog(m_onChainClient, this);
        connect( &dialog, &AddDriveDialog::updateDrivesCBox, this, &MainWin::updateDrivesCBox );
        connect( &dialog, &AddDriveDialog::updateDrivesCBox, this, [this](){
            auto drive = Model::currentDriveInfoPtr();
            if (drive) {
                lockDrive(drive->m_driveKey);
            }
        } );
        dialog.exec();
        m_diffTableModel->updateModel();
    }, Qt::QueuedConnection);

    connect(ui->m_closeDrive, &QPushButton::released, this, [this] () {
        auto drive = Model::currentDriveInfoPtr();
        if (!drive) {
            qWarning() << LOG_SOURCE << "bad drive";
            return;
        }

        CloseDriveDialog dialog(m_onChainClient, drive->m_driveKey.c_str(), drive->m_name.c_str(), this);
        auto rc = dialog.exec();
        if ( rc==QMessageBox::Ok )
        {
            qDebug() << LOG_SOURCE << "drive->m_isDeleting = true";
            drive->m_isDeleting = true;
            updateDrivesCBox();
            lockDrive(drive->m_driveKey);
            markChannelsForDelete(drive->m_driveKey, true);

            auto channel = Model::currentChannelInfoPtr();
            if (channel) {
                lockChannel(channel->m_hash);
            }
        }
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::prepareDriveTransactionConfirmed, this, [this](auto alias, auto driveKey) {
        onDriveCreationConfirmed( alias, sirius::drive::toString( driveKey ) );
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::prepareDriveTransactionFailed, this, [this](auto alias, auto driveKey, auto errorText) {
        onDriveCreationFailed( alias, sirius::drive::toString( driveKey ), errorText.toStdString() );
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::closeDriveTransactionConfirmed, this, &MainWin::onDriveCloseConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::closeDriveTransactionFailed, this, &MainWin::onDriveCloseFailed, Qt::QueuedConnection);
    connect( ui->m_applyChangesBtn, &QPushButton::released, this, &MainWin::onApplyChanges, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::initializedSuccessfully, this, [this](auto networkName){
        qDebug() << "initializedSuccessfully";

        loadBalance();
        ui->m_networkName->setText(networkName);

        xpx_chain_sdk::DrivesPageOptions options;
        options.owner = gSettings.config().m_publicKeyStr;
        m_onChainClient->loadDrives(options);

        lockMainButtons(false);
    }, Qt::QueuedConnection);

    connect(ui->m_onBoardReplicator, &QPushButton::released, this, [this](){
        ReplicatorOnBoardingDialog dialog(this);
        dialog.exec();
    });

    connect(ui->m_offBoardReplicator, &QPushButton::released, this, [this](){
        ReplicatorOffBoardingDialog dialog(this);
        dialog.exec();
    });

    connect(m_onChainClient, &OnChainClient::dataModificationTransactionConfirmed, this, &MainWin::onDataModificationTransactionConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::dataModificationTransactionFailed, this, &MainWin::onDataModificationTransactionFailed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::dataModificationApprovalTransactionConfirmed, this, &MainWin::onDataModificationApprovalTransactionConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::dataModificationApprovalTransactionFailed, this, &MainWin::onDataModificationApprovalTransactionFailed, Qt::QueuedConnection);

//        connect( m_onChainClient->transactionsEngine(), &TransactionsEngine::modificationCreated, this, [this] (const QString& driveId, const std::array<uint8_t,32>& modificationId)
//        {
//            std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
//            if ( auto* driveInfo = Model::findDrive( driveId.toStdString() ); driveInfo != nullptr )
//            {
//                qWarning() << LOG_SOURCE << "m_currentModificationHash: " << sirius::drive::toString(modificationId).c_str();
//                driveInfo->m_currentModificationHash = modificationId;
//                driveInfo->m_modificationStatus = is_registring;
//                updateModificationStatus();
//            }
//            else
//            {
//                qWarning() << LOG_SOURCE << "unknown driveId: " << driveId;
//            }

//        }, Qt::QueuedConnection);

    connect(ui->m_refresh, &QPushButton::released, this, &MainWin::onRefresh, Qt::QueuedConnection);

    setupDownloadsTab();
    setupDrivesTab();

    // Start update-timer for downloads
    m_downloadUpdateTimer = new QTimer();
    connect( m_downloadUpdateTimer, &QTimer::timeout, this, [this]
    {
        auto selectedIndexes = ui->m_downloadsTableView->selectionModel()->selectedRows();

        m_downloadsTableModel->updateProgress();
        
        if ( ! selectedIndexes.empty() )
        {
            ui->m_downloadsTableView->selectRow( selectedIndexes.begin()->row() );
        }
        ui->m_removeDownloadBtn->setEnabled( ! selectedIndexes.empty() );

    }, Qt::QueuedConnection);
    m_downloadUpdateTimer->start(500); // 2 times per second

    lockMainButtons(true);

    auto modifyPanelCallback = [this](){
        cancelModification();
        updateModificationStatus();
    };

    m_modifyProgressPanel = new ModifyProgressPanel( 350, 350, this, modifyPanelCallback );

    connect( ui->tabWidget, &QTabWidget::currentChanged, this, &MainWin::updateModificationStatus, Qt::QueuedConnection);

    updateModificationStatus();
    setupNotifications();
}

MainWin::~MainWin()
{
    delete ui;
}

void MainWin::cancelModification()
{
    auto drive = Model::currentDriveInfoPtr();
    if (!drive) {
        qWarning() << LOG_SOURCE << "bad drive";
        return;
    }

    CancelModificationDialog dialog(m_onChainClient, drive->m_driveKey.c_str(), drive->m_name.c_str(), this);
    dialog.exec();
}

void MainWin::updateModificationStatus()
{
    if ( ui->tabWidget->currentIndex() != 1 )
    {
        m_modifyProgressPanel->setVisible(false);
    }
    else
    {
        if ( auto* driveInfo = Model::currentDriveInfoPtr(); driveInfo == nullptr )
        {
            m_modifyProgressPanel->setVisible(false);
        }
        else
        {
            switch( driveInfo->m_modificationStatus )
            {
            case no_modification:
                m_modifyProgressPanel->setVisible(false);
                break;
            case is_registering:
                m_modifyProgressPanel->setRegistering();
                m_modifyProgressPanel->setVisible(true);
                break;
            case is_registered:
                m_modifyProgressPanel->setRegistered();
                m_modifyProgressPanel->setVisible(true);
                break;
                //is_failed, is_canceling, is_canceled
            case is_approved:
                m_modifyProgressPanel->setApproved();
                m_modifyProgressPanel->setVisible(true);
                break;
            case is_approvedWithOldRootHash:
                m_modifyProgressPanel->setIsApprovedWithOldRootHash();
                m_modifyProgressPanel->setVisible(true);
                break;
            case is_failed:
                m_modifyProgressPanel->setFailed();
                m_modifyProgressPanel->setVisible(true);
                break;
            case is_canceling:
                m_modifyProgressPanel->setCanceling();
                m_modifyProgressPanel->setVisible(true);
                break;
            case is_canceled:
                m_modifyProgressPanel->setCanceled();
                m_modifyProgressPanel->setVisible(true);
                break;
            }
        }
    }
}

void MainWin::setupIcons() {
    qDebug() << getResource("./resources/icons/settings.png");
    QIcon settingsButtonIcon(getResource("./resources/icons/settings.png"));
    ui->m_settingsButton->setFixedSize(settingsButtonIcon.availableSizes().first());
    ui->m_settingsButton->setText("");
    ui->m_settingsButton->setIcon(settingsButtonIcon);
    ui->m_settingsButton->setStyleSheet("background: transparent; border: 0px;");
    ui->m_settingsButton->setIconSize(QSize(18, 18));

    QIcon notificationsButtonIcon(getResource("./resources/icons/notification.png"));
    ui->m_notificationsButton->setFixedSize(notificationsButtonIcon.availableSizes().first());
    ui->m_notificationsButton->setText("");
    ui->m_notificationsButton->setIcon(notificationsButtonIcon);
    ui->m_notificationsButton->setStyleSheet("background: transparent; border: 0px;");
    ui->m_notificationsButton->setIconSize(QSize(18, 18));

    QPixmap networkIcon(getResource("./resources/icons/network.png"));
    ui->m_networkLabel->setPixmap(networkIcon);
    ui->m_networkLabel->setStyleSheet("padding: 5px 5px 5px 5px;");
    ui->m_networkLabel->setAlignment(Qt::AlignLeft);
    ui->m_networkName->setAlignment(Qt::AlignLeft);
    ui->m_networkName->setStyleSheet("padding-top: 5px; padding-bottom: 5px;");
    ui->m_balance->setStyleSheet("padding-top: 5px; padding-bottom: 5px;");
    ui->m_balanceValue->setStyleSheet("padding-top: 5px; padding-bottom: 5px;");
    ui->label_5->setStyleSheet("padding-top: 5px; padding-bottom: 5px;");

    ui->m_balance->setText("Balance:");
}

void MainWin::setupDownloadsTab()
{
    if ( ALEX_LOCAL_TEST )
    {
        Model::stestInitChannels();
    }

    setupChannelFsTable();
    setupDownloadsTable();

    ui->m_channels->clear();
    ui->m_channels->addItem( "Loading..." );

    connect( ui->m_downloadBtn, &QPushButton::released, this, [this] ()
    {
        onDownloadBtn();
    }, Qt::QueuedConnection);

//    connect( ui->m_downloadBtn, &QPushButton::released, this, [this] ()
//    {
//        onDownloadBtn();
//    });

    PopupMenu* menu = new PopupMenu(ui->m_moreChannelsBtn, this);

    QAction* renameAction = new QAction("Rename", this);
    menu->addAction(renameAction);
    connect( renameAction, &QAction::triggered, this, [this](bool)
    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        if ( auto* channelInfo = Model::currentChannelInfoPtr(); channelInfo != nullptr )
        {
            std::string channelName = channelInfo->m_name;
            std::string channelKey = channelInfo->m_hash;
            lock.unlock();

            EditDialog dialog( "Rename channel", "Channel name:", channelName );
            if ( dialog.exec() == QDialog::Accepted )
            {
                qDebug() << "channelName: " << channelName.c_str();

                std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
                if ( auto* channelInfo = Model::findChannel(channelKey); channelInfo != nullptr )
                {
                    qDebug() << "channelName renamed: " << channelName.c_str();

                    channelInfo->m_name = channelName;
                    gSettings.save();
                    lock.unlock();
                    updateChannelsCBox();
                }
            }
            qDebug() << "channelName?: " << channelName;
        }
    });

    QAction* topUpAction = new QAction("Top-up", this);
    menu->addAction(topUpAction);
    connect( topUpAction, &QAction::triggered, this, [this](bool)
    {
        qDebug() << "TODO: topUpAction";
        DownloadPaymentDialog dialog(m_onChainClient, this);
        dialog.exec();
    });

    QAction* copyChannelKeyAction = new QAction("Copy channel key", this);
    menu->addAction(copyChannelKeyAction);
    connect( copyChannelKeyAction, &QAction::triggered, this, [this](bool)
    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        if ( auto* channelInfo = Model::currentChannelInfoPtr(); channelInfo != nullptr )
        {
            std::string channelKey = channelInfo->m_hash;
            lock.unlock();

            QClipboard* clipboard = QApplication::clipboard();
            if ( !clipboard ) {
                qWarning() << LOG_SOURCE << "bad clipboard";
                lock.unlock();
                return;
            }

            clipboard->setText( QString::fromStdString(channelKey), QClipboard::Clipboard );
            if ( clipboard->supportsSelection() ) {
                clipboard->setText( QString::fromStdString(channelKey), QClipboard::Selection );
            }
        }
    });

    QAction* channelInfoAction = new QAction("Channel info", this);
    menu->addAction(channelInfoAction);
    connect( channelInfoAction, &QAction::triggered, this, [this](bool)
    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        if ( auto* channelInfo = Model::currentChannelInfoPtr(); channelInfo != nullptr )
        {
            ChannelInfoDialog dialog( *channelInfo, this );
            dialog.exec();
        }
    });

    ui->m_moreChannelsBtn->setMenu(menu);
}

void MainWin::setupChannelFsTable()
{
    connect( ui->m_channelFsTableView, &QTableView::doubleClicked, this, [this] (const QModelIndex &index)
    {
        //qDebug() << LOG_SOURCE << index;
        int toBeSelectedRow = m_channelFsTreeTableModel->onDoubleClick( index.row() );
        ui->m_channelFsTableView->selectRow( toBeSelectedRow );
        ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));
        if ( auto* channelPtr = Model::currentChannelInfoPtr(); channelPtr != nullptr )
        {
            channelPtr->m_lastOpenedPath = m_channelFsTreeTableModel->currentPath();
            qDebug() << LOG_SOURCE << "m_lastOpenedPath: " << channelPtr->m_lastOpenedPath;
        }
    }, Qt::QueuedConnection);

    connect( ui->m_channelFsTableView, &QTableView::pressed, this, [this] (const QModelIndex &index)
    {
        selectChannelFsItem( index.row() );
    }, Qt::QueuedConnection);

    connect( ui->m_channelFsTableView, &QTableView::clicked, this, [this] (const QModelIndex &index)
    {
        selectChannelFsItem( index.row() );
    }, Qt::QueuedConnection);

    m_channelFsTreeTableModel = new FsTreeTableModel( true );

    ui->m_channelFsTableView->setModel( m_channelFsTreeTableModel );
    ui->m_channelFsTableView->setColumnWidth(0,300);
    ui->m_channelFsTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_channelFsTableView->horizontalHeader()->hide();
    ui->m_channelFsTableView->setGridStyle( Qt::NoPen );
    ui->m_channelFsTableView->update();
    ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));
}

void MainWin::selectChannelFsItem( int index )
{
    if ( index < 0 && index >= m_channelFsTreeTableModel->m_rows.size() )
    {
        return;
    }

    ui->m_channelFsTableView->selectRow( index );

    if (!m_channelFsTreeTableModel->m_rows.empty()) {
        ui->m_downloadBtn->setEnabled( ! m_channelFsTreeTableModel->m_rows[index].m_isFolder );
    }
}

void MainWin::setupDriveFsTable()
{
    connect( ui->m_driveFsTableView, &QTableView::doubleClicked, this, [this] (const QModelIndex &index)
    {
        //qDebug() << LOG_SOURCE << index;
        int toBeSelectedRow = m_driveFsTreeTableModel->onDoubleClick( index.row() );
        ui->m_driveFsTableView->selectRow( toBeSelectedRow );
        ui->m_drivePath->setText( "Path: " + QString::fromStdString(m_driveFsTreeTableModel->currentPathString()));
        if ( auto* drivePtr = Model::currentDriveInfoPtr(); drivePtr != nullptr )
        {
            drivePtr->m_lastOpenedPath = m_driveFsTreeTableModel->currentPath();
            qDebug() << LOG_SOURCE << "m_lastOpenedPath: " << drivePtr->m_lastOpenedPath;
        }
    }, Qt::QueuedConnection);

    connect( ui->m_driveFsTableView, &QTableView::pressed, this, [this] (const QModelIndex &index)
    {
        selectDriveFsItem( index.row() );
    }, Qt::QueuedConnection);

    connect( ui->m_driveFsTableView, &QTableView::clicked, this, [this] (const QModelIndex &index)
    {
        selectDriveFsItem( index.row() );
    }, Qt::QueuedConnection);

    m_driveFsTreeTableModel = new FsTreeTableModel( false );

    ui->m_driveFsTableView->setModel( m_driveFsTreeTableModel );
    ui->m_driveFsTableView->horizontalHeader()->hide();
    ui->m_driveFsTableView->setColumnWidth(0,300);
    ui->m_driveFsTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_driveFsTableView->setGridStyle( Qt::NoPen );
    ui->m_driveFsTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    ui->m_drivePath->setText( "Path: " + QString::fromStdString(m_driveFsTreeTableModel->currentPathString()));

    m_diffTreeModel = new DriveTreeModel(true);

    ui->m_diffTree->setModel( m_diffTreeModel );
    ui->m_diffTree->setTreePosition(1);
    ui->m_diffTree->setColumnHidden(0,true);
    ui->m_diffTree->header()->resizeSection(1, 300);
    ui->m_diffTree->header()->resizeSection(2, 30);
    ui->m_diffTree->header()->setDefaultAlignment(Qt::AlignLeft);
    ui->m_diffTree->setHeaderHidden(true);
    ui->m_diffTree->show();
    ui->m_diffTree->expandAll();

}

void MainWin::selectDriveFsItem( int index )
{
    if ( index < 0 && index >= m_driveFsTreeTableModel->m_rows.size() )
    {
        return;
    }

    ui->m_driveFsTableView->selectRow( index );

    if (!m_driveFsTreeTableModel->m_rows.empty()) {
        ui->m_downloadBtn->setEnabled( ! m_driveFsTreeTableModel->m_rows[index].m_isFolder );
    }
}


void MainWin::onDownloadBtn()
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    //
    // Download all selected files
    //
    auto selectedIndexes = ui->m_channelFsTableView->selectionModel()->selectedRows();
    for( auto index: selectedIndexes )
    {
        int row = index.row();
        if ( row < 1 || row >= m_channelFsTreeTableModel->m_rows.size() )
        {
            continue;
        }

        const auto& hash = m_channelFsTreeTableModel->m_rows[row].m_hash;
        const auto& name = m_channelFsTreeTableModel->m_rows[row].m_name;

        auto channelId = Model::currentChannelInfoPtr();
        if ( channelId != nullptr )
        {
            auto ltHandle = Model::downloadFile( channelId->m_hash,  hash );

            m_downloadsTableModel->beginResetModel();
            Model::downloads().emplace_back( DownloadInfo{ hash, channelId->m_hash, name,
                                                           Model::downloadFolder(),
                                                           false, 0, ltHandle } );
            m_downloadsTableModel->endResetModel();

            Model::saveSettings();
        }
    }
}

void MainWin::setupDownloadsTable()
{
    m_downloadsTableModel = new DownloadsTableModel(this);

    ui->m_downloadsTableView->setModel( m_downloadsTableModel );
    ui->m_downloadsTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_downloadsTableView->horizontalHeader()->hide();
    ui->m_downloadsTableView->setGridStyle( Qt::NoPen );
    ui->m_downloadsTableView->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->m_downloadsTableView->setSelectionMode( QAbstractItemView::SingleSelection );

    //ui->m_downloadsTableView->update();
    ui->m_removeDownloadBtn->setEnabled( false );

    connect( ui->m_downloadsTableView, &QTableView::doubleClicked, this, [this] (const QModelIndex &index)
    {
        ui->m_downloadsTableView->selectRow( index.row() );
    });

    connect( ui->m_downloadsTableView, &QTableView::pressed, this, [this] (const QModelIndex &index)
    {
        ui->m_downloadsTableView->selectRow( index.row() );
    });

    connect( ui->m_downloadsTableView, &QTableView::clicked, this, [this] (const QModelIndex &index)
    {
        ui->m_downloadsTableView->selectRow( index.row() );
    });

    connect( ui->m_removeDownloadBtn, &QPushButton::released, this, [this] ()
    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        qDebug() << LOG_SOURCE << "removeDownloadBtn pressed: " << ui->m_downloadsTableView->selectionModel()->selectedRows();

        auto rows = ui->m_downloadsTableView->selectionModel()->selectedRows();
        if ( rows.empty() )
        {
            return;
        }

        int rowIndex = rows.begin()->row();
        auto& downloads = Model::downloads();

        if ( ! rows.empty() && rowIndex >= 0 && rowIndex < downloads.size() )
        {
            DownloadInfo dnInfo = Model::downloads()[rowIndex];
            lock.unlock();

            QMessageBox msgBox;
            const QString message = QString::fromStdString("'" + dnInfo.m_fileName + "' will be removed.");
            msgBox.setText(message);
            msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
            auto reply = msgBox.exec();

            if ( reply == QMessageBox::Ok )
            {
                Model::removeFromDownloads( rowIndex );
                addNotification(message);
                Model::saveSettings();
            }
        }
    });
}

bool MainWin::requestPrivateKey()
{
    {
        PrivKeyDialog pKeyDialog( this );
        pKeyDialog.exec();

        if ( pKeyDialog.result() != QDialog::Accepted )
        {
            qDebug() << LOG_SOURCE << "not accepted";
            return false;
        }
        qDebug() << LOG_SOURCE << "accepted";
    }

    SettingsDialog settingsDialog( this, true );
    settingsDialog.exec();

    if ( settingsDialog.result() != QDialog::Accepted )
    {
        return false;
    }

    gSettings.m_windowGeometry.setWidth(1000);
    gSettings.m_windowGeometry.setHeight(700);

    return true;
}

void MainWin::updateChannelsCBox()
{
    qDebug() << LOG_SOURCE << "updateChannelsCBox: " << gSettings.config().m_channelsLoaded << " : " << Model::dnChannels().size();

    if ( ! gSettings.config().m_channelsLoaded )
    {
        return;
    }

    ui->m_channels->clear();

    for( const auto& channelInfo: Model::dnChannels() )
    {
        if ( channelInfo.m_isCreating )
        {
            ui->m_channels->addItem( QString::fromStdString( channelInfo.m_name + "(creating...)") );
        }
        else if ( channelInfo.m_isDeleting )
        {
            ui->m_channels->addItem( QString::fromStdString( channelInfo.m_name + "(...deleting)" ) );
        }
        else
        {
            ui->m_channels->addItem( QString::fromStdString( channelInfo.m_name) );
        }
    }

    if ( Model::currentChannelInfoPtr() == nullptr && !Model::dnChannels().empty() )
    {
        onCurrentChannelChanged(0);
    }

    qDebug() << LOG_SOURCE << "ui->m_channels: " << ui->m_channels->size();
    ui->m_channels->setCurrentIndex( Model::currentDnChannelIndex() );

    if ( auto* channelPtr = Model::currentChannelInfoPtr(); channelPtr != nullptr )
    {
        m_lastSelectedChannelKey = channelPtr->m_hash;
    }
    else
    {
        m_lastSelectedChannelKey.clear();
    }
}

void MainWin::addChannel( const std::string&              channelName,
                          const std::string&              channelKey,
                          const std::string&              driveKey,
                          const std::vector<std::string>& allowedPublicKeys )
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto& channels = Model::dnChannels();
    auto creationTime = std::chrono::steady_clock::now();
    channels.emplace_back( ChannelInfo{ channelName,
                                        channelKey,
                                        driveKey,
                                        allowedPublicKeys,
                                        true, false, // isCreating, isDeleting
                                        creationTime
                           } );

    // Copy fsTree (if it exists)
    if ( auto drivePtr = Model::findDrive(driveKey); drivePtr != nullptr && drivePtr->m_rootHash )
    {
        channels.back().m_fsTreeHash = drivePtr->m_rootHash;
        channels.back().m_fsTree     = drivePtr->m_fsTree;
    }
    else
    {
        Model::applyForChannels( driveKey, [&channels] (const ChannelInfo& channelInfo )
        {
            if ( ! channels.back().m_fsTreeHash && channelInfo.m_fsTreeHash )
            {
                channels.back().m_fsTreeHash = channelInfo.m_fsTreeHash;
                channels.back().m_fsTree     = channelInfo.m_fsTree;
            }
        });
    }

    Model::setCurrentDnChannelIndex( (int)channels.size()-1 );

    updateChannelsCBox();
    lock.unlock();

    lockChannel(channelKey);
}

void MainWin::onChannelCreationConfirmed( const std::string& alias, const std::string& channelKey, const std::string& driveKey )
{
    qDebug() << "onChannelCreationConfirmed: alias:" << alias;
    qDebug() << "onChannelCreationConfirmed: channelKey:" << channelKey;

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto* channel = Model::findChannel( channelKey );
    if ( channel )
    {
        channel->m_isCreating = false;
        Model::saveSettings();
    }
    else
    {
        qDebug() << "onChannelCreationConfirmed: channel not found!!! " << channelKey;

        ChannelInfo channelInfo;
        channelInfo.m_name = alias;
        channelInfo.m_hash = channelKey;
        channelInfo.m_driveHash = driveKey;

        channelInfo.m_isCreating = false;

        Model::dnChannels().push_back(channelInfo);
        Model::saveSettings();
    }

    lock.unlock();

    updateChannelsCBox();
    onRefresh();

    const QString message = QString::fromStdString( "Channel '" + alias + "' created successfully.");
    showNotification(message);
    addNotification(message);

    unlockChannel(channelKey);
}

void MainWin::onChannelCreationFailed( const std::string& channelKey, const std::string& errorText )
{
    auto* channelInfo = Model::findChannel( channelKey );
    if (channelInfo) {
        const QString message = QString::fromStdString( "Channel creation failed (" + channelInfo->m_name + ")\n It will be removed.");
        showNotification(message, errorText.c_str());
        addNotification(message);

        Model::removeChannel(channelKey);
        updateChannelsCBox();
        unlockChannel(channelInfo->m_hash);
    }
}

void MainWin::onDriveCreationConfirmed( const std::string &alias, const std::string &driveKey )
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto* drive = Model::findDrive( driveKey );
    if ( drive )
    {
        m_modificationsWatcher->addPath(drive->m_localDriveFolder.c_str());
        drive->m_isCreating = false;
        drive->m_isConfirmed = true;
        gSettings.save();
    }
    else
    {
        DriveInfo driveInfo;
        driveInfo.m_name = alias;
        driveInfo.m_driveKey = driveKey;
        driveInfo.m_isCreating = false;
        drive->m_isConfirmed = true;

        Model::drives().push_back(driveInfo);
        gSettings.save();
    }

    lock.unlock();

    updateDrivesCBox();

    const QString message = QString::fromStdString( "Drive '" + alias + "' created successfully.");
    showNotification(message);
    addNotification(message);
	startCalcDiff();
    loadBalance();
    unlockDrive(driveKey);
}

void MainWin::onDriveCreationFailed(const std::string& alias, const std::string& driveKey, const std::string& errorText)
{
    const QString message = QString::fromStdString( "Drive creation failed (" + alias + ")\n It will be removed.");
    showNotification(message, errorText.c_str());
    addNotification(message);

    Model::removeDrive(driveKey);
    Model::removeChannelByDriveKey(driveKey);
    updateDrivesCBox();
    updateChannelsCBox();
    unlockDrive(driveKey);
}

void MainWin::onDriveCloseConfirmed(const std::array<uint8_t, 32>& driveKey) {
    const auto driveKeyHex = sirius::drive::toString(driveKey);
    auto* drive = Model::findDrive(driveKeyHex);
    if (drive) {
        m_modificationsWatcher->removePath(drive->m_localDriveFolder.c_str());
    }

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    std::string driveName;
    auto* drivePtr = Model::findDrive(driveKeyHex);
    if ( drivePtr != nullptr )
    {
        driveName = drivePtr->m_name;
    }
    lock.unlock();

    Model::removeDrive(driveKeyHex);
    updateDrivesCBox();
    m_diffTableModel->updateModel();

    Model::removeChannelByDriveKey(sirius::drive::toString(driveKey));
    updateChannelsCBox();

    if ( drivePtr != nullptr )
    {
        const QString message = QString::fromStdString( "Drive '" + driveName + "' closed (removed).");
        showNotification(message);
        addNotification(message);
    }

    loadBalance();
    unlockDrive(driveKeyHex);
    unlockChannel("");
}

void MainWin::onDriveCloseFailed(const std::array<uint8_t, 32>& driveKey, const QString& errorText) {
    const auto driveId = rawHashToHex(driveKey).toStdString();
    std::string alias = driveId;
    auto drive = Model::findDrive(alias);
    if (drive) {
        alias = drive->m_name;
    } else {
        qWarning () << LOG_SOURCE << "bad drive (alias not found): " << alias;
    }

    QString message = "The drive '";
    message.append(alias.c_str());
    message.append("' was not close by reason: ");
    message.append(errorText);
    showNotification(message);
    addNotification(message);
    unlockDrive(driveId);
    markChannelsForDelete(driveId, false);
}

void MainWin::onApplyChanges()
{
    auto drive = Model::currentDriveInfoPtr();
    if (!drive) {
        qWarning() << LOG_SOURCE << "bad drive";
        return;
    }

    auto& actionList = drive->m_actionList;
    if ( actionList.empty() )
    {
        const QString message = "There is no difference.";
        showNotification(message);
        addNotification(message);
        return;
    }

    auto actions = drive->m_actionList;
    if (actions.empty()) {
        qWarning() << LOG_SOURCE << "no actions to apply";
        return;
    }

    const std::string sandbox = settingsFolder().string() + "/" + drive->m_driveKey + "/modify_drive_data";
    auto driveKeyHex = rawHashFromHex(drive->m_driveKey.c_str());
    m_onChainClient->applyDataModification(driveKeyHex, actions, sandbox);

    drive->m_modificationStatus = is_registering;
    updateModificationStatus();
    loadBalance();
}

void MainWin::onRefresh()
{
    auto channel = Model::currentChannelInfoPtr();
    if (channel) {
        downloadLatestFsTree(channel->m_driveHash);
    }

    loadBalance();
}

void MainWin::onDownloadChannelCloseConfirmed(const std::array<uint8_t, 32> &channelId) {

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    std::string channelName;
    const auto channelKey = sirius::drive::toString(channelId);
    auto* channelPtr = Model::findChannel( channelKey );
    if ( channelPtr != nullptr )
    {
        channelName = channelPtr->m_name;
    }
    lock.unlock();

    Model::removeChannel( sirius::drive::toString(channelId) );

    if ( channelPtr != nullptr )
    {
        const QString message = QString::fromStdString( "Channel '" + channelName + "' closed (removed).");
        showNotification(message);
        addNotification(message);
    }

    updateChannelsCBox();
    loadBalance();
    unlockChannel(channelKey);
}

void MainWin::onDownloadChannelCloseFailed(const std::array<uint8_t, 32> &channelId, const QString &errorText) {
    const auto channelKey = rawHashToHex(channelId).toStdString();
    std::string alias = channelKey;
    auto channel = Model::findChannel(alias);
    if (channel) {
        alias = channel->m_name;
    } else {
        qWarning () << LOG_SOURCE << "bad channel (alias not found): " << alias;
    }

    QString message = "The channel '";
    message.append(alias.c_str());
    message.append("' was not close by reason: ");
    message.append(errorText);
    showNotification(message);
    addNotification(message);
    unlockChannel(channelKey);
}

void MainWin::onDownloadPaymentConfirmed(const std::array<uint8_t, 32> &channelId) {
    std::string alias = rawHashToHex(channelId).toStdString();
    auto channel = Model::findChannel(alias);
    if (channel) {
        alias = channel->m_name;
    } else {
        qWarning () << LOG_SOURCE << "bad channel (alias not found): " << alias;
    }

    const QString message = QString::fromStdString( "Your payment for the following channel was successful: '" + alias);
    showNotification(message);
    addNotification(message);
    loadBalance();
}

void MainWin::onDownloadPaymentFailed(const std::array<uint8_t, 32> &channelId, const QString &errorText) {
    std::string alias = rawHashToHex(channelId).toStdString();
    auto channel = Model::findChannel(alias);
    if (channel) {
        alias = channel->m_name;
    } else {
        qWarning () << LOG_SOURCE << "bad channel (alias not found): " << alias;
    }

    const QString message = QString::fromStdString( "Your payment for the following channel was UNSUCCESSFUL: '" + alias);
    showNotification(message);
    addNotification(message);
}

void MainWin::onStoragePaymentConfirmed(const std::array<uint8_t, 32> &driveKey) {
    std::string alias = rawHashToHex(driveKey).toStdString();
    auto drive = Model::findDrive(alias);
    if (drive) {
        alias = drive->m_name;
    } else {
        qWarning () << LOG_SOURCE << "bad drive (alias not found): " << alias;
    }

    const QString message = QString::fromStdString( "Your payment for the following drive was successful: '" + alias);
    showNotification(message);
    addNotification(message);
    loadBalance();
}

void MainWin::onStoragePaymentFailed(const std::array<uint8_t, 32> &driveKey, const QString &errorText) {
    std::string alias = rawHashToHex(driveKey).toStdString();
    auto drive = Model::findDrive(alias);
    if (drive) {
        alias = drive->m_name;
    } else {
        qWarning () << LOG_SOURCE << "bad drive (alias not found): " << alias;
    }

    const QString message = QString::fromStdString( "Your payment for the following drive was UNSUCCESSFUL: '" + alias);
    showNotification(message);
    addNotification(message);
}

void MainWin::onDataModificationTransactionConfirmed(const std::array<uint8_t, 32>& driveKey, const std::array<uint8_t, 32> &modificationId) {
    qDebug () << LOG_SOURCE << "Your last modification was registered: '" + rawHashToHex(modificationId);

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    if ( auto drive = Model::findDrive( sirius::drive::toString(driveKey)); drive != nullptr )
    {
        drive->m_modificationStatus = is_registered;
        lock.unlock();

        updateModificationStatus();
    }
    else
    {
        qDebug () << LOG_SOURCE << "Your last modification was registered: !!! drive not found";
    }

    loadBalance();
}

void MainWin::onDataModificationTransactionFailed(const std::array<uint8_t, 32>& driveKey, const std::array<uint8_t, 32> &modificationId) {
    qDebug () << LOG_SOURCE << "Your last modification was declined: '" + rawHashToHex(modificationId);

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    if ( auto drive = Model::findDrive( sirius::drive::toString(driveKey)); drive != nullptr )
    {
        drive->m_modificationStatus = is_failed;
        lock.unlock();

        updateModificationStatus();
    }

    const QString message = "Your modification was declined.";
    showNotification(message);
    addNotification(message);
}

void MainWin::onDataModificationApprovalTransactionConfirmed(const std::array<uint8_t, 32> &driveId,
                                                             const std::string &fileStructureCdi) {
    std::string driveAlias = rawHashToHex(driveId).toStdString();

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto drive = Model::findDrive(driveAlias);
    if (drive) {
        drive->m_modificationStatus = is_approved;
        driveAlias = drive->m_name;
    } else {
        qWarning () << LOG_SOURCE << "bad drive (alias not found): " << driveAlias;
    }

    lock.unlock();

    updateModificationStatus();

    qDebug () << LOG_SOURCE << "Your modification was APPLIED. Drive: " +
                 QString::fromStdString(driveAlias) + " CDI: " +
                 QString::fromStdString(fileStructureCdi);

    startCalcDiff();
    onRefresh();

    QString message;
    message.append("Your modification was applied. Drive: ");
    message.append(driveAlias.c_str());
    addNotification(message);
    loadBalance();
}

void MainWin::onDataModificationApprovalTransactionFailed(const std::array<uint8_t, 32> &driveId) {
    std::string driveAlias = rawHashToHex(driveId).toStdString();
    auto drive = Model::findDrive(driveAlias);
    if (drive) {
        driveAlias = drive->m_name;
    } else {
        qWarning () << LOG_SOURCE << "bad drive (alias not found): " << driveAlias;
    }

    startCalcDiff();

    QString message;
    message.append("Your modifications was DECLINED. Drive: ");
    message.append(driveAlias.c_str());
    showNotification(message);
	addNotification(message);
}

void MainWin::onCancelModificationTransactionConfirmed(const std::array<uint8_t, 32> &driveId, const QString &modificationId) {
    std::string driveRawId = rawHashToHex(driveId).toStdString();
    auto drive = Model::findDrive(driveRawId);
    if (!drive) {
        qWarning () << LOG_SOURCE << "bad drive: " << driveRawId.c_str() << " modification id: " << modificationId;
    } else {
        drive->m_modificationStatus = ModificationStatus::is_canceled;
    }

    updateModificationStatus();
    loadBalance();
}

void MainWin::onCancelModificationTransactionFailed(const std::array<uint8_t, 32> &driveId, const QString &modificationId) {
    std::string driveRawId = rawHashToHex(driveId).toStdString();
    auto drive = Model::findDrive(driveRawId);
    if (!drive) {
        qWarning () << LOG_SOURCE << "bad drive: " << driveRawId.c_str() << " modification id: " << modificationId;
    } else {
        drive->m_modificationStatus = ModificationStatus::is_failed;
    }

    updateModificationStatus();
	loadBalance();
}

void MainWin::loadBalance() {
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    auto publicKey = gSettings.config().m_publicKeyStr;
    lock.unlock();
    m_onChainClient->getBlockchainEngine()->getAccountInfo(publicKey, [this](xpx_chain_sdk::AccountInfo info, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            qWarning() << LOG_SOURCE << message.c_str() << " : " << code.c_str();
            return;
        }

        // 1423072717967804887 - publicTest
        // 992621222383397347 - local
        const xpx_chain_sdk::MosaicName PRX_XPX { 1423072717967804887, { "prx.xpx" } };
        auto mosaicIterator = std::find_if( info.mosaics.begin(), info.mosaics.end(), [PRX_XPX]( auto mosaic )
        {
            return mosaic.id == PRX_XPX.mosaicId;
        });

        const uint64_t balance = mosaicIterator == info.mosaics.end() ? 0 : mosaicIterator->amount;
        ui->m_balanceValue->setText(QString::number(balance));
    });
}

void MainWin::onCurrentChannelChanged( int index )
{
    if (index >= 0) {
        Model::setCurrentDnChannelIndex( index );

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        auto channel = Model::currentChannelInfoPtr();
        if ( !channel )
        {
            qWarning() << LOG_SOURCE << "bad channel";
            m_channelFsTreeTableModel->setFsTree( {}, {} );
            return;
        }

        m_downloadsTableModel->beginResetModel();
        m_downloadsTableModel->endResetModel();

        if ( ! channel->m_fsTreeHash )
        {
            downloadLatestFsTree( channel->m_driveHash );
        }
        else
        {
            m_channelFsTreeTableModel->setFsTree( channel->m_fsTree, channel->m_lastOpenedPath );
            ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));
        }

        if (channel) {
            lockChannel(channel->m_hash);
            unlockChannel(channel->m_hash);
        }
    }
}

void MainWin::onDriveLocalDirectoryChanged(const QString&) {
    startCalcDiff();
}

void MainWin::onCurrentDriveChanged( int index )
{
    if (index >= 0) {
        Model::setCurrentDriveIndex( index );
        updateModificationStatus();

        auto drive = Model::currentDriveInfoPtr();
        if (drive) {
            lockDrive(drive->m_driveKey);
            unlockDrive(drive->m_driveKey);
        }

        // TODO: thread
        emit ui->m_calcDiffBtn->released();
    }

    if ( auto drivePtr = Model::currentDriveInfoPtr(); drivePtr == nullptr )
    {
        qWarning() << LOG_SOURCE << "bad channel";
        m_driveFsTreeTableModel->setFsTree( {}, {} );
        return;
    }
    else
    {
        if ( ! drivePtr->m_rootHash )
        {
            downloadLatestFsTree( drivePtr->m_driveKey );
            m_driveTreeModel->updateModel(false);
            m_diffTreeModel->updateModel(true);
        }
        else
        {
            m_driveFsTreeTableModel->setFsTree( drivePtr->m_fsTree, drivePtr->m_lastOpenedPath );
            ui->m_path->setText( "Path: " + QString::fromStdString(m_driveFsTreeTableModel->currentPathString()));
        }
    }
}

void MainWin::setupDrivesTab()
{
    setupDriveFsTable();

    //ui->m_driveFsTableView->setVisible(false);

    connect( ui->m_openLocalFolderBtn, &QPushButton::released, this, []
    {
        qDebug() << LOG_SOURCE << "openLocalFolderBtn";

        auto* driveInfo = Model::currentDriveInfoPtr();
        if ( driveInfo != nullptr )
        {
            qDebug() << LOG_SOURCE << "driveInfo->m_localDriveFolder: " << driveInfo->m_localDriveFolder.c_str();
            std::error_code ec;
            if ( ! fs::exists(driveInfo->m_localDriveFolder, ec) )
            {
                fs::create_directories( driveInfo->m_localDriveFolder, ec );
            }
//            QDesktopServices::openUrl( QString::fromStdString( "file://" + driveInfo->m_localDriveFolder));

#ifdef __APPLE__
            QDesktopServices::openUrl( QUrl::fromLocalFile( QString::fromStdString( driveInfo->m_localDriveFolder )));
#else
            //QProcess::startDetached("xdg-open " + QString::fromStdString( driveInfo->m_localDriveFolder ));
            //system("/usr/bin/xdg-open file:///home/cempl");
            qDebug() << "TRY OPEN";
            auto result = system("/home/cempl/repositories/cpp-xpx-storage-user-app/resources/scripts/open_local_folder.sh");
            qDebug() << "TRY OPEN " << result;
            qDebug() << "TRY OPEN " << std::strerror(errno);
#endif
        }
    });

    connect( ui->m_calcDiffBtn, &QPushButton::released, this, [this]
    {
        startCalcDiff();
    });

    if ( ALEX_LOCAL_TEST )
    {
        Model::stestInitDrives();
    }

    if ( gSettings.config().m_currentDriveIndex >= 0 )
    {
        Model::setCurrentDriveIndex( gSettings.config().m_currentDriveIndex );
        if ( auto* drivePtr = Model::currentDriveInfoPtr(); drivePtr != nullptr )
        {
            drivePtr->m_calclDiffIsWaitingFsTree = true;
            downloadLatestFsTree( drivePtr->m_driveKey );
        }
    }

    ui->m_driveCBox->clear();
    ui->m_driveCBox->addItem( "Loading..." );

//    updateDrivesCBox();

    // request FsTree info-hash
//    if ( ALEX_LOCAL_TEST )
//    {
//        auto* driveInfoPtr = Model::currentDriveInfoPtr();
//        std::array<uint8_t,32> fsTreeHash;
//        sirius::utils::ParseHexStringIntoContainer( ROOT_HASH1,
//                                                        64, fsTreeHash );

//        qDebug() << LOG_SOURCE << "driveHash: " << driveInfoPtr->m_driveKey.c_str();

//        Model::downloadFsTree( driveInfoPtr->m_driveKey,
//                               "0000000000000000000000000000000000000000000000000000000000000000",
//                               fsTreeHash,
//                               [this] ( const std::string&           driveHash,
//                                        const std::array<uint8_t,32> fsTreeHash,
//                                        const sirius::drive::FsTree& fsTree )
//        {
//            Model::onFsTreeForDriveReceived( driveHash, fsTreeHash, fsTree );

//            qDebug() << LOG_SOURCE << "driveHash: " << driveHash.c_str();

//            if ( driveHash == Model::currentDriveInfoPtr()->m_driveKey )
//            {
//                qDebug() << LOG_SOURCE << "m_calcDiffBtn->setEnabled: " << driveHash.c_str();
//                ui->m_calcDiffBtn->setEnabled(true);
//            }
//        });
//    }

    m_driveTreeModel = new DriveTreeModel(false);
    ui->m_driveTreeView->setModel( m_driveTreeModel );
    ui->m_driveTreeView->setTreePosition(1);
    ui->m_driveTreeView->setColumnHidden(0,true);
    ui->m_driveTreeView->header()->resizeSection(1, 300);
    ui->m_driveTreeView->header()->resizeSection(2, 30);
    ui->m_driveTreeView->header()->setDefaultAlignment(Qt::AlignLeft);
    ui->m_driveTreeView->setHeaderHidden(true);
    ui->m_driveTreeView->show();
    ui->m_driveTreeView->expandAll();

    m_diffTableModel = new DiffTableModel();

    ui->m_diffTableView->setModel( m_diffTableModel );
    ui->m_diffTableView->horizontalHeader()->hide();
//    ui->m_diffTableView->horizontalHeader()->resizeSection(0, 250);
//    ui->m_diffTableView->horizontalHeader()->resizeSection(1, 300);
    ui->m_diffTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_diffTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    //ui->m_diffTableView->setGridStyle( Qt::NoPen );


    PopupMenu* menu = new PopupMenu(ui->m_moreDrivesBtn, this);

    QAction* renameAction = new QAction("Rename", this);
    menu->addAction(renameAction);
    connect( renameAction, &QAction::triggered, this, [this](bool)
    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        if ( auto* driveInfo = Model::currentDriveInfoPtr(); driveInfo != nullptr )
        {
            std::string driveName = driveInfo->m_name;
            std::string driveKey = driveInfo->m_driveKey;
            lock.unlock();

            EditDialog dialog( "Rename drive", "Drive name:", driveName );
            if ( dialog.exec() == QDialog::Accepted )
            {
                std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
                if ( auto* driveInfo = Model::findDrive(driveKey); driveInfo != nullptr )
                {
                    driveInfo->m_name = driveName;
                    gSettings.save();
                    lock.unlock();
                    updateDrivesCBox();
                }
            }
        }
    });

    QAction* changeFolderAction = new QAction("Change local folder", this);
    menu->addAction(changeFolderAction);
    connect( changeFolderAction, &QAction::triggered, this, [this](bool)
    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        if ( auto* driveInfo = Model::currentDriveInfoPtr(); driveInfo != nullptr )
        {
            std::string driveName = driveInfo->m_name;
            std::string driveKey = driveInfo->m_driveKey;
            lock.unlock();

            QFlags<QFileDialog::Option> options = QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks;
    #ifdef Q_OS_LINUX
            options |= QFileDialog::DontUseNativeDialog;
    #endif
            const QString path = QFileDialog::getExistingDirectory(this, tr("Choose directory"), "/", options);
            if ( path.trimmed().isEmpty() )
            {
                qWarning () << LOG_SOURCE  << "bad path";
                return;
            }
            else
            {
                std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
                if ( auto* driveInfo = Model::findDrive(driveKey); driveInfo != nullptr )
                {
                    if (path.toStdString() != driveInfo->m_localDriveFolder) {
                        m_modificationsWatcher->removePath(driveInfo->m_localDriveFolder.c_str());
                        m_modificationsWatcher->addPath(path.toStdString().c_str());
                        driveInfo->m_localDriveFolder = path.toStdString();
                        driveInfo->m_localDriveFolderExists = true;
                        gSettings.save();
                        startCalcDiff();
                    }

                    lock.unlock();
                    updateDrivesCBox();
                }
            }
        }
    });

    QAction* topUpAction = new QAction("Top-up", this);
    menu->addAction(topUpAction);
    connect( topUpAction, &QAction::triggered, this, [this](bool)
    {
        StoragePaymentDialog dialog(m_onChainClient, this);
        dialog.exec();
    });

    QAction* copyDriveKeyAction = new QAction("Copy drive key", this);
    menu->addAction(copyDriveKeyAction);
    connect( copyDriveKeyAction, &QAction::triggered, this, [this](bool)
    {
        qDebug() << "TODO: copyDriveKeyAction";

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        if ( auto* driveInfo = Model::currentDriveInfoPtr(); driveInfo != nullptr )
        {
            std::string driveKey = driveInfo->m_driveKey;
            lock.unlock();

            QClipboard* clipboard = QApplication::clipboard();
            if ( !clipboard ) {
                qWarning() << LOG_SOURCE << "bad clipboard";
                lock.unlock();
                return;
            }

            clipboard->setText( QString::fromStdString(driveKey), QClipboard::Clipboard );
            if ( clipboard->supportsSelection() ) {
                clipboard->setText( QString::fromStdString(driveKey), QClipboard::Selection );
            }
        }
    });

    QAction* driveInfoAction = new QAction("Drive info", this);
    menu->addAction(driveInfoAction);
    connect( driveInfoAction, &QAction::triggered, this, [this](bool)
    {
        qDebug() << "TODO: driveInfoAction";

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        if ( auto* driveInfo = Model::currentDriveInfoPtr(); driveInfo != nullptr )
        {
            DriveInfoDialog dialog( *driveInfo, this);
            dialog.exec();
        }
    });

    ui->m_moreDrivesBtn->setMenu(menu);
}

void MainWin::setupNotifications() {
    m_notificationsWidget = new QListWidget(this);
    m_notificationsWidget->setAttribute(Qt::WA_QuitOnClose);
    m_notificationsWidget->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    m_notificationsWidget->setWindowModality(Qt::NonModal);
    m_notificationsWidget->setMaximumHeight(500);
    m_notificationsWidget->setStyleSheet("padding: 5px 5px 5px 5px; font: 13px; border-radius: 3px; background-color: #FFF; border: 1px solid gray;");

    connect(ui->m_notificationsButton, &QPushButton::released, this, [this](){
        if (m_notificationsWidget->count() < 1) {
            QToolTip::showText(QCursor::pos(), tr("Empty!"), nullptr, {}, 3000);
            return;
        }

        if (m_notificationsWidget->isVisible()) {
            m_notificationsWidget->hide();
        } else {
            auto width = m_notificationsWidget->sizeHintForColumn(0) + m_notificationsWidget->frameWidth() * 2;
            m_notificationsWidget->setFixedWidth(width);
            m_notificationsWidget->show();
            QPoint point = ui->m_notificationsButton->mapToGlobal(QPoint());
            m_notificationsWidget->move(QPoint(point.x() - m_notificationsWidget->width() + ui->m_notificationsButton->width(), point.y() + ui->m_notificationsButton->height()));
        }
    });
}

void MainWin::showNotification(const QString &message, const QString& error) {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Notification");
    msgBox.setText(message);

    if (!error.isEmpty()) {
        msgBox.setInformativeText(error);
    }

    msgBox.setStandardButtons( QMessageBox::Ok );
    msgBox.exec();
}

void MainWin::addNotification(const QString& message) {
    // No memory leaks, m_notificationsWidget will take ownerships after insert
    auto item = new QListWidgetItem(tr(message.toStdString().c_str()));
    m_notificationsWidget->insertItem(0, item);
}

void MainWin::updateDrivesCBox()
{
    ui->m_driveCBox->clear();

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    for( const auto& drive: Model::drives() )
    {
        if ( drive.m_isCreating )
        {
            ui->m_driveCBox->addItem( QString::fromStdString( drive.m_name + "(creating...)") );
        }
        else if ( drive.m_isDeleting )
        {
            ui->m_driveCBox->addItem( QString::fromStdString( drive.m_name + "(...deleting)" ) );
        }
        else
        {
            ui->m_driveCBox->addItem( QString::fromStdString( drive.m_name) );
        }
    }

    if ( Model::currentDriveInfoPtr() == nullptr && !Model::drives().empty() )
    {
        Model::setCurrentDriveIndex(0);
    }

    if ( Model::currentDriveIndex() >= 0 )
    {
        ui->m_driveCBox->setCurrentIndex( Model::currentDriveIndex() );
    }

    m_driveTreeModel->updateModel(false);
    ui->m_driveTreeView->expandAll();

    if ( auto* drive = Model::currentDriveInfoPtr(); drive != nullptr )
    {
        m_driveFsTreeTableModel->setFsTree( drive->m_fsTree, drive->m_lastOpenedPath );
    }

    if ( auto* drivePtr = Model::currentDriveInfoPtr(); drivePtr != nullptr )
    {
        m_lastSelectedDriveKey = drivePtr->m_driveKey;
    }
    else
    {
        m_lastSelectedDriveKey.clear();
    }
}

void MainWin::lockChannel(const std::string &channelId) {
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    auto channel = Model::findChannel(channelId);
    if (channel && (channel->m_isCreating || channel->m_isDeleting)) {
        ui->m_closeChannel->setDisabled(true);
        ui->m_moreChannelsBtn->setDisabled(true);
        ui->m_channelFsTableView->setDisabled(true);
        ui->m_refresh->setDisabled(true);
        ui->m_downloadBtn->setDisabled(true);
    }
}

void MainWin::unlockChannel(const std::string &channelId) {
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    auto channel = Model::findChannel(channelId);
    if ((channel && !channel->m_isCreating && !channel->m_isDeleting) || (Model::dnChannels().empty())) {
        ui->m_closeChannel->setEnabled(true);
        ui->m_moreChannelsBtn->setEnabled(true);
        ui->m_channelFsTableView->setEnabled(true);
        ui->m_refresh->setEnabled(true);
        ui->m_downloadBtn->setEnabled(true);
    }
}

void MainWin::lockDrive(const std::string &driveId) {
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    auto drive = Model::findDrive(driveId);
    if (drive && (drive->m_isCreating || drive->m_isDeleting)) {
        ui->m_closeDrive->setDisabled(true);
        ui->m_moreDrivesBtn->setDisabled(true);
        ui->m_driveTreeView->setDisabled(true);
        ui->m_driveFsTableView->setDisabled(true);
        ui->m_openLocalFolderBtn->setDisabled(true);
        ui->m_applyChangesBtn->setDisabled(true);
        ui->m_diffTableView->setDisabled(true);
        ui->m_calcDiffBtn->setDisabled(true);
    }
}

void MainWin::unlockDrive(const std::string &driveId) {
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    auto drive = Model::findDrive(driveId);
    if ((drive && !drive->m_isCreating && !drive->m_isDeleting) || (Model::drives().empty())) {
        ui->m_closeDrive->setEnabled(true);
        ui->m_moreDrivesBtn->setEnabled(true);
        ui->m_driveTreeView->setEnabled(true);
        ui->m_driveFsTableView->setEnabled(true);
        ui->m_openLocalFolderBtn->setEnabled(true);
        ui->m_applyChangesBtn->setEnabled(true);
        ui->m_diffTableView->setEnabled(true);
        ui->m_calcDiffBtn->setEnabled(true);
    }
}

void MainWin::markChannelsForDelete(const std::string& driveId, bool state) {
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    auto drive = Model::findDrive(driveId);
    if (drive) {
        auto& channels = gSettings.config().m_dnChannels;
        for (auto& channel : channels) {
            if (boost::iequals( channel.m_driveHash, drive->m_driveKey )) {
                channel.m_isDeleting = state;
            }
        }
    }
}

void MainWin::downloadLatestFsTree( const std::string& driveKey )
{
    qDebug() << LOG_SOURCE << "@ download FsTree: " << driveKey.c_str();

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    DriveInfo* drivePtr = Model::findDrive( driveKey );
    if (!drivePtr) {
        qWarning() << LOG_SOURCE << "bad drive key: " << driveKey;
        return;
    }

    //
    // Check that fsTree is not now downloading
    //
    bool isFsTreeNowDownloading = drivePtr->m_downloadingFsTree;
    drivePtr->m_downloadingFsTree = true;
    Model::applyForChannels( drivePtr->m_driveKey, [&](ChannelInfo& channelInfo)
    {
        isFsTreeNowDownloading = isFsTreeNowDownloading || channelInfo.m_waitingFsTree;
        channelInfo.m_waitingFsTree = true;
    });

    if ( isFsTreeNowDownloading )
    {
        qDebug() << LOG_SOURCE << "@ download FsTree: isFsTreeAlreadyDownloading";
        return;
    }

    //
    // Get drive root hash
    //
    m_onChainClient->getBlockchainEngine()->getDriveById( drivePtr->m_driveKey,
                                                          [driveKey=drivePtr->m_driveKey,this]
                                                          (auto drive, auto isSuccess, auto message, auto code )
    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        DriveInfo* drivePtr = Model::findDrive( driveKey );
        if (!drivePtr) {
            qWarning() << LOG_SOURCE << "drive removed";
            return;
        }

        if ( !isSuccess )
        {
            //
            // Handle drive error
            //
            qWarning() << LOG_SOURCE << "message: " << message.c_str() << " code: " << code.c_str();

            drivePtr->m_downloadingFsTree = false;
            drivePtr->m_calclDiffIsWaitingFsTree = false;
            Model::applyForChannels( drivePtr->m_driveKey, [&](ChannelInfo& channelInfo)
            {
                channelInfo.m_waitingFsTree = false;
            });

            lock.unlock();

            if ( drivePtr->m_isConfirmed )
            {
                const QString messageText = "Cannot get drive root hash.";
                const QString error = QString::fromStdString( "Drive: " + driveKey + "\nError: " + message );
                showNotification(messageText, error);
                addNotification(messageText);
            }

            return;
        }

        qDebug() << LOG_SOURCE << "@ on RootHash received: " << drive.data.rootHash.c_str();

        std::array<uint8_t,32> fsTreeHash{};
        sirius::utils::ParseHexStringIntoContainer( drive.data.rootHash.c_str(), 64, fsTreeHash );

        bool rootHashIsNotChanged = drivePtr->m_rootHash && *drivePtr->m_rootHash == fsTreeHash;

        Model::applyForChannels( driveKey, [&] (const ChannelInfo& channelInfo )
        {
            rootHashIsNotChanged = rootHashIsNotChanged
                                && channelInfo.m_fsTreeHash && channelInfo.m_fsTreeHash == fsTreeHash;
        });

        if ( rootHashIsNotChanged )
        {
            qDebug() << LOG_SOURCE << "rootHash Is Not Changed";

            drivePtr->m_downloadingFsTree = false;

            Model::applyForChannels( driveKey, [&] (ChannelInfo& channelInfo)
            {
                channelInfo.m_waitingFsTree = false;
            });

            if ( auto* channelPtr = Model::currentChannelInfoPtr(); channelPtr != nullptr && channelPtr->m_driveHash == driveKey )
            {
                m_channelFsTreeTableModel->setFsTree( channelPtr->m_fsTree, channelPtr->m_lastOpenedPath );
            }

            if ( drivePtr->m_calclDiffIsWaitingFsTree )
            {
                continueCalcDiff( *drivePtr );
            }
            else
            {
                m_driveFsTreeTableModel->setFsTree( drivePtr->m_fsTree, drivePtr->m_lastOpenedPath );
            }

            return;
        }

        // Check previously saved FsTree-s
        {
            auto fsTreeSaveFolder = fsTreesFolder()/sirius::drive::toString(fsTreeHash);
            if ( fs::exists( fsTreeSaveFolder / FS_TREE_FILE_NAME ) )
            {
                qDebug() << LOG_SOURCE << "Use previously saved FsTree";

                sirius::drive::FsTree fsTree;
                try
                {
                    qDebug() << LOG_SOURCE << "Using already saved fsTree";
                    fsTree.deserialize( fsTreeSaveFolder / FS_TREE_FILE_NAME );
                } catch (const std::runtime_error& ex )
                {
                    qDebug() << LOG_SOURCE << "Invalid fsTree: " << ex.what();
//                    qDebug() << LOG_SOURCE << "Invalid fsTree: fsTreeSaveFolder:" << fsTreeSaveFolder;
                    fsTree = {};
                    fsTree.addFile( {}, std::string("!!! bad FsTree: ") + ex.what(),{},0);
                }
                onFsTreeReceived( driveKey, fsTreeHash, fsTree );
                return;
            }
        }

        Model::downloadFsTree( driveKey,
                               driveKey,
                               fsTreeHash,
                               drivePtr->m_replicatorList,
                               [this] ( const std::string&           driveHash,
                                        const std::array<uint8_t,32> fsTreeHash,
                                        const sirius::drive::FsTree& fsTree )
        {
            onFsTreeReceived( driveHash, fsTreeHash, fsTree );
        });
    });
}

void MainWin::onFsTreeReceived( const std::string& driveHash, const std::array<uint8_t,32>& fsTreeHash, const sirius::drive::FsTree& fsTree )
{
    qDebug() << LOG_SOURCE << "@ on FsTree received: " << driveHash.c_str();
    fsTree.dbgPrint();

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto drivePtr = Model::findDrive( driveHash );
    if (!drivePtr) {
        qWarning() << LOG_SOURCE << "bad drive";
        return;
    }

    drivePtr->m_rootHash = fsTreeHash;
    drivePtr->m_fsTree   = fsTree;
    drivePtr->m_downloadingFsTree = false;

    Model::applyForChannels( driveHash, [&] (ChannelInfo& channelInfo)
    {
        channelInfo.m_fsTreeHash = fsTreeHash;
        channelInfo.m_fsTree     = fsTree;
        channelInfo.m_waitingFsTree = false;
    });

    if ( auto* channelPtr = Model::currentChannelInfoPtr(); channelPtr != nullptr && channelPtr->m_driveHash == driveHash )
    {
        qDebug() << LOG_SOURCE << "@ on FsTree received: channelName:" << channelPtr->m_name.c_str();
        channelPtr->m_fsTreeHash = fsTreeHash;
        channelPtr->m_fsTree     = fsTree;
        channelPtr->m_waitingFsTree = false;
        m_channelFsTreeTableModel->setFsTree( fsTree, channelPtr->m_lastOpenedPath );
    }

    if (  drivePtr->m_calclDiffIsWaitingFsTree )
    {
        continueCalcDiff( *drivePtr );
    }
}

void MainWin::continueCalcDiff( DriveInfo& drive )
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    drive.m_calclDiffIsWaitingFsTree = false;

    qDebug() << LOG_SOURCE << "drive.m_isConfirmed: " << drive.m_isConfirmed;

    if ( drive.m_isConfirmed )
    {
        Model::calcDiff();

        QMetaObject::invokeMethod( this, [this]()
        {
            std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

            if ( auto* drive = Model::currentDriveInfoPtr(); drive != nullptr )
            {
                m_diffTableModel->updateModel();
                //ui->m_diffTableView->resizeColumnsToContents();
                m_diffTreeModel->updateModel(true);
                m_driveTreeModel->updateModel(false);
                m_driveFsTreeTableModel->setFsTree( drive->m_fsTree, drive->m_lastOpenedPath );

                ui->m_diffTree->expandAll();
                ui->m_driveTreeView->expandAll();
            }
        }, Qt::QueuedConnection);

        static bool atFirst = true;
        if ( atFirst )
        {
            atFirst = false;
            std::thread([this] {
                usleep(500000);
                std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

                if ( auto* drive = Model::currentDriveInfoPtr(); drive != nullptr )
                {
                    m_driveFsTreeTableModel->setFsTree( drive->m_fsTree, drive->m_lastOpenedPath );
                }
            }).detach();
        }
    }
}

void MainWin::startCalcDiff()
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    if ( auto drivePtr = Model::currentDriveInfoPtr(); drivePtr != nullptr )
    {
        drivePtr->m_calclDiffIsWaitingFsTree = true;
        downloadLatestFsTree( drivePtr->m_driveKey );
    }
}


void MainWin::lockMainButtons(bool state) {
    ui->m_refresh->setDisabled(state);
    ui->m_addChannel->setDisabled(state);
    ui->m_closeChannel->setDisabled(state);
    ui->m_addDrive->setDisabled(state);
    ui->m_closeDrive->setDisabled(state);
}

void MainWin::closeEvent(QCloseEvent *event) {
    if (event) {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
        gSettings.m_windowGeometry = frameGeometry();
        gSettings.save();
        lock.unlock();

        event->accept();
    }
}

void MainWin::addLocalModificationsWatcher() {
    auto drives = gSettings.config().m_drives;
    for (const auto& drive : drives) {
        m_modificationsWatcher->addPath(drive.m_localDriveFolder.c_str());
    }
}
