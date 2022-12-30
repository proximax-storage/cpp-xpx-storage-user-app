//#include "moc_mainwin.cpp"

#include "mainwin.h"
#include "./ui_mainwin.h"

#include "Settings.h"
#include "Model.h"
#include "FsTreeTableModel.h"
#include "DownloadsTableModel.h"
#include "DriveTreeModel.h"
#include "DiffTableModel.h"
#include "QtGui/qclipboard.h"
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
#include "DriveModificationEvent.h"
#include "Drive.h"

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
    , m_settings(new Settings(this))
    , m_model(new Model(m_settings, this))
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

    if ( ! m_model->loadSettings() )
    {
        if ( Model::homeFolder() == "/Users/alex" )
        {
            m_model->initForTests();
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
        if ( m_model->getAccountName() == "alex_local_test" )
        {
            m_model->setBootstrapReplicator("192.168.2.101:5001");
            m_model->setCurrentAccountIndex(2);
        }
        else
        {
            m_model->setBootstrapReplicator("15.206.164.53:7904");
        }
    }

    // Clear old fsTrees
    //

    if ( fs::exists( getFsTreesFolder() ) )
    {
        try {
             fs::remove_all( getFsTreesFolder() );
        } catch( const std::runtime_error& err ) {
             QMessageBox msgBox(this);
             msgBox.setText( QString::fromStdString( "Cannot remove getFsTreesFolder") );
             msgBox.setInformativeText( QString::fromStdString( "Error: " ) + err.what() );
             msgBox.setStandardButtons( QMessageBox::Ok );
             msgBox.exec();
        }
    }

    try {
        fs::create_directories( getFsTreesFolder() );
    } catch( const std::runtime_error& err ) {
         QMessageBox msgBox(this);
         msgBox.setText( QString::fromStdString( "Cannot create getFsTreesFolder") );
         msgBox.setInformativeText( QString::fromStdString( "Error: " ) + err.what() );
         msgBox.setStandardButtons( QMessageBox::Ok );
         msgBox.exec();
    }

    m_model->startStorageEngine( [this]
    {
//         QMessageBox msgBox(this);
//         msgBox.setText( QString::fromStdString( "Address already in use") );
//         msgBox.setInformativeText( QString::fromStdString( "Port: " + gSettings.m_udpPort ) );
//         msgBox.setStandardButtons( QMessageBox::Ok );
//         msgBox.exec();

        qWarning() << LOG_SOURCE << "Address already in use: udoPort: " << m_model->getUdpPort();
        exit(1);
    });

    setupIcons();
    setupNotifications();
    setGeometry(m_model->getWindowGeometry());

    connect( ui->m_settingsButton, &QPushButton::released, this, [this]
    {
        SettingsDialog settingsDialog( m_settings, this );
        settingsDialog.exec();
    }, Qt::QueuedConnection);

    const std::string privateKey = m_model->getClientPrivateKey();
    qDebug() << LOG_SOURCE << "privateKey: " << privateKey;

    m_onChainClient = new OnChainClient(gStorageEngine.get(), privateKey, m_model->getGatewayIp(), m_model->getGatewayPort(), this);
    m_modificationsWatcher = new QFileSystemWatcher(this);

    connect(m_modificationsWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWin::onDriveLocalDirectoryChanged, Qt::QueuedConnection);
    connect(ui->m_addChannel, &QPushButton::released, this, [this] () {
        AddDownloadChannelDialog dialog(m_onChainClient, m_model, this);
        connect(&dialog, &AddDownloadChannelDialog::addDownloadChannel, this, &MainWin::addChannel);
        dialog.exec();
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::downloadChannelsLoaded, this,
            [this](OnChainClient::ChannelsType type, const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& channelsPages)
    {
        qDebug() << LOG_SOURCE << "downloadChannelsLoaded pages amount: " << channelsPages.size();
        if (type == OnChainClient::ChannelsType::MyOwn) {
            m_model->onMyOwnChannelsLoaded(channelsPages);
        } else if (type == OnChainClient::ChannelsType::Sponsored) {
            m_model->onSponsoredChannelsLoaded(channelsPages);
        }

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
        m_model->setDownloadChannelsLoaded(true);

        int dnChannelIndex = m_model->currentDownloadChannelIndex();
        if ( dnChannelIndex >= 0 && dnChannelIndex < Model::dnChannels().size())
        {
            ui->m_channels->setCurrentIndex( dnChannelIndex );
        }
        lock.unlock();

        updateChannelsCBox();
        connect(m_model, &Model::driveStateChanged, this, &MainWin::onDriveStateChanged, Qt::QueuedConnection);
        connect( ui->m_channels, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWin::onCurrentChannelChanged, Qt::QueuedConnection);
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::drivesLoaded, this, [this](const std::vector<xpx_chain_sdk::drives_page::DrivesPage>& drivesPages)
    {
        qDebug() << LOG_SOURCE << "drivesLoaded pages amount: " << drivesPages.size();

        // add drives created on another devices
        m_model->onDrivesLoaded(drivesPages);

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
        m_model->setDrivesLoaded(true);

        qDebug() << LOG_SOURCE << "m_currentDriveIndex: " << m_model->currentDriveIndex();
        if ( m_model->currentDriveIndex() >= 0 )
        {
            m_model->setCurrentDriveIndex( m_model->currentDriveIndex() );
            if ( auto* drivePtr = m_model->currentDrive(); drivePtr != nullptr )
            {
                drivePtr->setWaitingFsTree(true);
                downloadLatestFsTree( drivePtr->getKey() );
            }
        }
        else
        {
            m_driveTreeModel->updateModel(false);
            ui->m_driveTreeView->expandAll();

            if ( auto* drive = m_model->currentDrive(); drive != nullptr )
            {
                m_driveFsTreeTableModel->setFsTree( drive->getFsTree(), drive->getLastOpenedPath() );
            }
        }

        addLocalModificationsWatcher();
        updateDrivesCBox();
        updateModificationStatus();

        connect( ui->m_driveCBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWin::onCurrentDriveChanged, Qt::QueuedConnection);

        // load my own channels
        xpx_chain_sdk::DownloadChannelsPageOptions options;
        options.consumerKey = m_model->getClientPublicKey();
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
        auto channel = m_model->currentDownloadChannel();
        if (!channel) {
            qWarning() << LOG_SOURCE << "bad channel";
            return;
        }

        CloseChannelDialog dialog(m_onChainClient, channel->getKey().c_str(), channel->getName().c_str(), this);
        auto rc = dialog.exec();
        if ( rc==QMessageBox::Ok )
        {
            channel->setDeleting(true);
            updateChannelsCBox();
            lockChannel(channel->getKey());
        }
    }, Qt::QueuedConnection);

    connect(ui->m_addDrive, &QPushButton::released, this, [this] () {
        AddDriveDialog dialog(m_onChainClient, m_model, this);
        connect( &dialog, &AddDriveDialog::updateDrivesCBox, this, &MainWin::updateDrivesCBox );
        connect( &dialog, &AddDriveDialog::updateDrivesCBox, this, [this](){
            auto drive = m_model->currentDrive();
            if (drive) {
                lockDrive(drive->getKey());
            }
        } );
        dialog.exec();
        m_diffTableModel->updateModel();
    }, Qt::QueuedConnection);

    connect(ui->m_closeDrive, &QPushButton::released, this, [this] () {
        auto drive = m_model->currentDrive();
        if (!drive) {
            qWarning() << LOG_SOURCE << "bad drive";
            return;
        }

        CloseDriveDialog dialog(m_onChainClient, drive->getKey().c_str(), drive->getName().c_str(), this);
        auto rc = dialog.exec();
        if ( rc==QMessageBox::Ok )
        {
            qDebug() << LOG_SOURCE << "drive->m_isDeleting = true";
            drive->updateState(deleting);
            updateDrivesCBox();
            lockDrive(drive->getKey());
            m_model->markChannelsForDelete(drive->getKey(), true);

            auto channel = m_model->currentDownloadChannel();
            if (channel) {
                lockChannel(channel->getKey());
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
        options.owner = m_model->getClientPublicKey();
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
    connect(ui->m_refresh, &QPushButton::released, this, &MainWin::onRefresh, Qt::QueuedConnection);

    setupDownloadsTab();
    setupDrivesTab();

    // Start update-timer for downloads
    m_downloadUpdateTimer = new QTimer();
    connect( m_downloadUpdateTimer, &QTimer::timeout, this, [this] {m_downloadsTableModel->updateProgress();}, Qt::QueuedConnection);
    m_downloadUpdateTimer->start(500); // 2 times per second

    lockMainButtons(true);

    auto modifyPanelCallback = [this](){
        cancelModification();
        updateModificationStatus();
    };

    m_modifyProgressPanel = new ModifyProgressPanel( m_model, 350, 350, this, modifyPanelCallback );

    connect( ui->tabWidget, &QTabWidget::currentChanged, this, &MainWin::updateModificationStatus, Qt::QueuedConnection);

    updateModificationStatus();
}

MainWin::~MainWin()
{
    m_model->endStorageEngine();
    delete ui;
}

void MainWin::cancelModification()
{
    auto drive = m_model->currentDrive();
    if (!drive) {
        qWarning() << LOG_SOURCE << "bad drive";
        return;
    }

    CancelModificationDialog dialog(m_onChainClient, drive->getKey().c_str(), drive->getName().c_str(), this);
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
        if ( auto* driveInfo = m_model->currentDrive(); driveInfo == nullptr )
        {
            m_modifyProgressPanel->setVisible(false);
        }
        else
        {
            switch(driveInfo->getState())
            {
                case no_modifications:
                    m_modifyProgressPanel->setVisible(false);
                    break;
                case registering:
                    m_modifyProgressPanel->setRegistering();
                    m_modifyProgressPanel->setVisible(true);
                    break;
                case approved:
                    m_modifyProgressPanel->setApproved();
                    m_modifyProgressPanel->setVisible(true);
                    break;
                case failed:
                    m_modifyProgressPanel->setFailed();
                    m_modifyProgressPanel->setVisible(true);
                    break;
                case canceling:
                    m_modifyProgressPanel->setCanceling();
                    m_modifyProgressPanel->setVisible(true);
                    break;
                case canceled:
                    m_modifyProgressPanel->setCanceled();
                    m_modifyProgressPanel->setVisible(true);
                    break;
                case uploading:
                    m_modifyProgressPanel->setUploading();
                    m_modifyProgressPanel->setVisible(true);
                    break;
                case creating:
                    break;
                case unconfirmed:
                    break;
                case deleting:
                    break;
                case deleted:
                    break;
                case deleteIsFailed:
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
        m_model->stestInitChannels();
    }

    setupChannelFsTable();
    setupDownloadsTable();

    ui->m_channels->clear();
    ui->m_channels->addItem( "Loading..." );

    connect( ui->m_downloadBtn, &QPushButton::released, this, [this] ()
    {
        onDownloadBtn();
    }, Qt::QueuedConnection);

    auto menu = new PopupMenu(ui->m_moreChannelsBtn, this);
    auto renameAction = new QAction("Rename", this);
    menu->addAction(renameAction);
    connect( renameAction, &QAction::triggered, this, [this](bool)
    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        if ( auto* channelInfo = m_model->currentDownloadChannel(); channelInfo != nullptr )
        {
            std::string channelName = channelInfo->getName();
            std::string channelKey = channelInfo->getKey();
            lock.unlock();

            EditDialog dialog( "Rename channel", "Channel name:", channelName );
            if ( dialog.exec() == QDialog::Accepted )
            {
                qDebug() << "channelName: " << channelName.c_str();

                std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
                if ( auto* channelInfo = m_model->findChannel(channelKey); channelInfo != nullptr )
                {
                    qDebug() << "channelName renamed: " << channelName.c_str();

                    channelInfo->setName(channelName);
                    m_model->saveSettings();
                    lock.unlock();
                    updateChannelsCBox();
                }
            }
            qDebug() << "channelName?: " << channelName;
        }
    });

    auto topUpAction = new QAction("Top-up", this);
    menu->addAction(topUpAction);
    connect( topUpAction, &QAction::triggered, this, [this](bool)
    {
        qDebug() << "TODO: topUpAction";
        DownloadPaymentDialog dialog(m_onChainClient, m_model, this);
        dialog.exec();
    });

    auto copyChannelKeyAction = new QAction("Copy channel key", this);
    menu->addAction(copyChannelKeyAction);
    connect( copyChannelKeyAction, &QAction::triggered, this, [this](bool)
    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        if ( auto* channelInfo = m_model->currentDownloadChannel(); channelInfo != nullptr )
        {
            std::string channelKey = channelInfo->getKey();
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

        if ( auto* channelInfo = m_model->currentDownloadChannel(); channelInfo != nullptr )
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
        int toBeSelectedRow = m_channelFsTreeTableModel->onDoubleClick( index.row() );
        ui->m_channelFsTableView->selectRow( toBeSelectedRow );
        ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));
        if ( auto* channelPtr = m_model->currentDownloadChannel(); channelPtr != nullptr )
        {
            channelPtr->setLastOpenedPath(m_channelFsTreeTableModel->currentPath());
            qDebug() << LOG_SOURCE << "m_lastOpenedPath: " << channelPtr->getLastOpenedPath();
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

    m_channelFsTreeTableModel = new FsTreeTableModel( m_model, true );

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
        if ( auto* drivePtr = m_model->currentDrive(); drivePtr != nullptr )
        {
            drivePtr->setLastOpenedPath(m_driveFsTreeTableModel->currentPath());
            qDebug() << LOG_SOURCE << "m_lastOpenedPath: " << drivePtr->getLastOpenedPath();
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

    m_driveFsTreeTableModel = new FsTreeTableModel( m_model, false );

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

    auto& downloads = m_model->downloads();

    bool overrideFileForAll = false;

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

        auto channel = m_model->currentDownloadChannel();
        if ( channel )
        {
            auto ltHandle = m_model->downloadFile( channel->getKey(), hash );
            m_downloadsTableModel->beginResetModel();

            DownloadInfo downloadInfo;
            downloadInfo.setHash(hash);
            downloadInfo.setDownloadChannelKey(channel->getKey());
            downloadInfo.setFileName(name);
            downloadInfo.setSaveFolder(m_model->getDownloadFolder());
            downloadInfo.setCompleted(false);
            downloadInfo.setProgress(0);
            downloadInfo.setHandle(ltHandle);

            m_model->downloads().emplace_back(downloadInfo);
            m_downloadsTableModel->m_selectedRow = int(m_model->downloads().size()) - 1;
            m_downloadsTableModel->endResetModel();
        }
    }
}

void MainWin::setupDownloadsTable()
{
    m_downloadsTableModel = new DownloadsTableModel( m_model, this, [this](int row) { ui->m_downloadsTableView->selectRow(row); });

    connect( ui->m_downloadsTableView, &QTableView::doubleClicked, this, [this] (const QModelIndex &index)
    {
        m_downloadsTableModel->m_selectedRow = index.row();
        ui->m_downloadsTableView->selectRow( index.row() );
    });

    connect( ui->m_downloadsTableView, &QTableView::pressed, this, [this] (const QModelIndex &index)
    {
        ui->m_downloadsTableView->selectRow( index.row() );
        m_downloadsTableModel->m_selectedRow = index.row();
    });

    connect( ui->m_downloadsTableView, &QTableView::clicked, this, [this] (const QModelIndex &index)
    {
        m_downloadsTableModel->m_selectedRow = index.row();
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
        auto& downloads = m_model->downloads();

        if ( ! rows.empty() && rowIndex >= 0 && rowIndex < downloads.size() )
        {
            DownloadInfo dnInfo = m_model->downloads()[rowIndex];
            lock.unlock();

            QMessageBox msgBox;
            const QString message = QString::fromStdString("'" + dnInfo.getFileName() + "' will be removed.");
            msgBox.setText(message);
            msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
            auto reply = msgBox.exec();

            if ( reply == QMessageBox::Ok )
            {
                m_model->removeFromDownloads( rowIndex );
            }
            selectDownloadRow(-1);
            addNotification(message);
        }
    });

    ui->m_downloadsTableView->setModel( m_downloadsTableModel );
    ui->m_downloadsTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_downloadsTableView->horizontalHeader()->hide();
    ui->m_downloadsTableView->setGridStyle( Qt::NoPen );
    ui->m_downloadsTableView->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->m_downloadsTableView->setSelectionMode( QAbstractItemView::SingleSelection );
    ui->m_removeDownloadBtn->setEnabled( false );
}

void MainWin::selectDownloadRow( int row )
{
    //qDebug() << LOG_SOURCE << "selectDownloadRow: " << row;
    m_downloadsTableModel->m_selectedRow = row;
    ui->m_downloadsTableView->selectRow( row );
    ui->m_removeDownloadBtn->setEnabled( row >= 0 );
}

bool MainWin::requestPrivateKey()
{
    {
        PrivKeyDialog pKeyDialog( m_settings, this );
        pKeyDialog.exec();

        if ( pKeyDialog.result() != QDialog::Accepted )
        {
            qDebug() << LOG_SOURCE << "not accepted";
            return false;
        }
        qDebug() << LOG_SOURCE << "accepted";
    }

    SettingsDialog settingsDialog( m_settings, this, true );
    settingsDialog.exec();

    if ( settingsDialog.result() != QDialog::Accepted )
    {
        return false;
    }

    QRect basicGeometry;
    basicGeometry.setWidth(1000);
    basicGeometry.setHeight(700);
    m_model->setWindowGeometry(basicGeometry);

    return true;
}

void MainWin::onDriveStateChanged(const std::string& driveKey, int state) {
    qInfo() << "MainWin::onDriveStateChanged: " << driveKey << " state: " << state;
    auto drive = m_model->currentDrive();
    if (drive && drive->getKey() == driveKey) {
        updateModificationStatus();
        lockDrive(driveKey);
        unlockDrive(driveKey);
    }
}

void MainWin::updateChannelsCBox()
{
    qDebug() << LOG_SOURCE << "updateChannelsCBox: " << m_model->isDownloadChannelsLoaded() << " : " << m_model->getDownloadChannels().size();

    if ( !m_model->isDownloadChannelsLoaded() )
    {
        return;
    }

    ui->m_channels->clear();

    for( const auto& channelInfo: m_model->getDownloadChannels() )
    {
        if ( channelInfo.isCreating() )
        {
            ui->m_channels->addItem( QString::fromStdString( channelInfo.getName() + "(creating...)") );
        }
        else if ( channelInfo.isDeleting() )
        {
            ui->m_channels->addItem( QString::fromStdString( channelInfo.getName() + "(...deleting)" ) );
        }
        else
        {
            ui->m_channels->addItem( QString::fromStdString( channelInfo.getName()) );
        }
    }

    if ( m_model->currentDownloadChannel() == nullptr && !m_model->getDownloadChannels().empty() )
    {
        onCurrentChannelChanged(0);
    }

    qDebug() << LOG_SOURCE << "ui->m_channels: " << ui->m_channels->size();
    ui->m_channels->setCurrentIndex(m_model->currentDownloadChannelIndex() );

    if ( auto* channelPtr = m_model->currentDownloadChannel(); channelPtr != nullptr )
    {
        m_lastSelectedChannelKey = channelPtr->getKey();
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
    DownloadChannel newChannel;
    newChannel.setName(channelName);
    newChannel.setKey(channelKey);
    newChannel.setDriveKey(driveKey);
    newChannel.setAllowedPublicKeys(allowedPublicKeys);
    newChannel.setCreating(true);
    newChannel.setDeleting(false);
    newChannel.setCreatingTimePoint(std::chrono::steady_clock::now());

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    m_model->addDownloadChannel(newChannel);

    auto drive = m_model->findDrive(driveKey);
    if (drive && drive->getRootHash()) {
        m_model->applyFsTreeForChannels(driveKey, drive->getFsTree(), drive->getRootHash().value());
    }

    m_model->setCurrentDownloadChannelIndex((int) m_model->getDownloadChannels().size() - 1);

    updateChannelsCBox();
    lock.unlock();

    lockChannel(channelKey);
}

void MainWin::onChannelCreationConfirmed( const std::string& alias, const std::string& channelKey, const std::string& driveKey )
{
    qDebug() << "onChannelCreationConfirmed: alias:" << alias;
    qDebug() << "onChannelCreationConfirmed: channelKey:" << channelKey;

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto channel = m_model->findChannel( channelKey );
    if ( channel )
    {
        channel->setCreating(false);
        m_model->saveSettings();
    }
    else
    {
        qDebug() << "onChannelCreationConfirmed: channel not found!!! " << channelKey;

        DownloadChannel newChannel;
        newChannel.setName(alias);
        newChannel.setKey(channelKey);
        newChannel.setDriveKey(driveKey);
        newChannel.setCreating(false);

        m_model->addDownloadChannel(newChannel);
        m_model->saveSettings();
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
    auto* channelInfo = m_model->findChannel( channelKey );
    if (channelInfo) {
        const QString message = QString::fromStdString( "Channel creation failed (" + channelInfo->getName() + ")\n It will be removed.");
        showNotification(message, errorText.c_str());
        addNotification(message);

        m_model->removeChannel(channelKey);
        updateChannelsCBox();
        unlockChannel(channelInfo->getKey());
    }
}

void MainWin::onDriveCreationConfirmed( const std::string &alias, const std::string &driveKey )
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto drive = m_model->findDrive( driveKey );
    if ( drive )
    {
        m_modificationsWatcher->addPath(drive->getLocalFolder().c_str());
        drive->updateState(no_modifications);
    }

    m_model->saveSettings();
    lock.unlock();
    updateDrivesCBox();

    const QString message = QString::fromStdString( "Drive '" + alias + "' created successfully.");
    showNotification(message);
    addNotification(message);
	startCalcDiff();
    loadBalance();
}

void MainWin::onDriveCreationFailed(const std::string& alias, const std::string& driveKey, const std::string& errorText)
{
    const QString message = QString::fromStdString( "Drive creation failed (" + alias + ")\n It will be removed.");
    showNotification(message, errorText.c_str());
    addNotification(message);

    auto drive = m_model->findDrive(driveKey);
    if (drive) {
        drive->updateState(unconfirmed);
    }
}

void MainWin::onDriveCloseConfirmed(const std::array<uint8_t, 32>& driveKey) {
    const auto driveKeyHex = sirius::drive::toString(driveKey);
    auto drive = m_model->findDrive(driveKeyHex);
    if (drive) {
        m_modificationsWatcher->removePath(drive->getLocalFolder().c_str());
        drive->updateState(deleted);
    }

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    std::string driveName;
    auto* drivePtr = m_model->findDrive(driveKeyHex);
    if ( drivePtr != nullptr )
    {
        driveName = drivePtr->getName();
    }
    lock.unlock();

    m_model->removeDrive(driveKeyHex);
    updateDrivesCBox();
    m_diffTableModel->updateModel();

    m_model->removeChannelByDriveKey(sirius::drive::toString(driveKey));
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
    auto drive = m_model->findDrive(alias);
    if (drive) {
        alias = drive->getName();
        drive->updateState(deleteIsFailed);
        drive->updateState(no_modifications);
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
    m_model->markChannelsForDelete(driveId, false);
}

void MainWin::onApplyChanges()
{
    auto drive = m_model->currentDrive();
    if (!drive) {
        qWarning() << LOG_SOURCE << "bad drive";
        return;
    }

    auto actionList = drive->getActionsList();
    if ( actionList.empty() )
    {
        qWarning() << LOG_SOURCE << "no actions to apply";
        const QString message = "There is no difference.";
        showNotification(message);
        addNotification(message);
        return;
    }

    const std::string sandbox = getSettingsFolder().string() + "/" + drive->getKey() + "/modify_drive_data";
    auto driveKeyHex = rawHashFromHex(drive->getKey().c_str());
    m_onChainClient->applyDataModification(driveKeyHex, actionList, sandbox);

    drive->updateState(registering);
    updateModificationStatus();
    loadBalance();
}

void MainWin::onRefresh()
{
    auto channel = m_model->currentDownloadChannel();
    if (channel) {
        downloadLatestFsTree(channel->getDriveKey());
    }

    loadBalance();
}

void MainWin::onDownloadChannelCloseConfirmed(const std::array<uint8_t, 32> &channelId) {

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    std::string channelName;
    const auto channelKey = sirius::drive::toString(channelId);
    auto* channelPtr = m_model->findChannel( channelKey );
    if ( channelPtr != nullptr )
    {
        channelName = channelPtr->getName();
    }
    lock.unlock();

    m_model->removeChannel( sirius::drive::toString(channelId) );

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
    auto channel = m_model->findChannel(alias);
    if (channel) {
        alias = channel->getName();
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
    auto channel = m_model->findChannel(alias);
    if (channel) {
        alias = channel->getName();
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
    auto channel = m_model->findChannel(alias);
    if (channel) {
        alias = channel->getName();
    } else {
        qWarning () << LOG_SOURCE << "bad channel (alias not found): " << alias;
    }

    const QString message = QString::fromStdString( "Your payment for the following channel was UNSUCCESSFUL: '" + alias);
    showNotification(message);
    addNotification(message);
}

void MainWin::onStoragePaymentConfirmed(const std::array<uint8_t, 32> &driveKey) {
    std::string alias = rawHashToHex(driveKey).toStdString();
    auto drive = m_model->findDrive(alias);
    if (drive) {
        alias = drive->getName();
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
    auto drive = m_model->findDrive(alias);
    if (drive) {
        alias = drive->getName();
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

    if ( auto drive = m_model->findDrive( sirius::drive::toString(driveKey)); drive != nullptr )
    {
        drive->updateState(uploading);
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

    if ( auto drive = m_model->findDrive( sirius::drive::toString(driveKey)); drive != nullptr )
    {
        drive->updateState(failed);
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

    auto drive = m_model->findDrive(driveAlias);
    if (drive) {
        drive->updateState(approved);
        driveAlias = drive->getName();
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

void MainWin::onDataModificationApprovalTransactionFailed(const std::array<uint8_t, 32> &driveId, const std::string &fileStructureCdi, uint8_t code) {
    std::string driveAlias = rawHashToHex(driveId).toStdString();
    auto drive = m_model->findDrive(driveAlias);
    if (drive) {
        driveAlias = drive->getName();
        drive->updateState(failed);
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
    auto drive = m_model->findDrive(driveRawId);
    if (!drive) {
        qWarning () << LOG_SOURCE << "bad drive: " << driveRawId.c_str() << " modification id: " << modificationId;
    } else {
        drive->updateState(canceled);
    }

    updateModificationStatus();
    loadBalance();
}

void MainWin::onCancelModificationTransactionFailed(const std::array<uint8_t, 32> &driveId, const QString &modificationId) {
    std::string driveRawId = rawHashToHex(driveId).toStdString();
    auto drive = m_model->findDrive(driveRawId);
    if (!drive) {
        qWarning () << LOG_SOURCE << "bad drive: " << driveRawId.c_str() << " modification id: " << modificationId;
    } else {
        drive->updateState(failed);
    }

    updateModificationStatus();
	loadBalance();
}

void MainWin::loadBalance() {
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    auto publicKey = m_model->getClientPublicKey();
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
        m_model->setCurrentDownloadChannelIndex(index);

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        auto channel = m_model->currentDownloadChannel();
        if ( !channel )
        {
            qWarning() << LOG_SOURCE << "bad channel";
            m_channelFsTreeTableModel->setFsTree( {}, {} );
            return;
        }

        if ( !channel->getFsTreeHash() )
        {
            downloadLatestFsTree( channel->getDriveKey() );
        }
        else
        {
            m_channelFsTreeTableModel->setFsTree( channel->getFsTree(), channel->getLastOpenedPath() );
            ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));
        }

        lockChannel(channel->getKey());
        unlockChannel(channel->getKey());
    }
}

void MainWin::onDriveLocalDirectoryChanged(const QString&) {
    startCalcDiff();
}

void MainWin::onCurrentDriveChanged( int index )
{
    if (index >= 0) {
        m_model->setCurrentDriveIndex( index );
        updateModificationStatus();

        auto drive = m_model->currentDrive();
        if (drive) {
            lockDrive(drive->getKey());
            unlockDrive(drive->getKey());
        }

        // TODO: thread
        emit ui->m_calcDiffBtn->released();
    }

    if ( auto drivePtr = m_model->currentDrive(); drivePtr == nullptr )
    {
        qWarning() << LOG_SOURCE << "bad channel";
        m_driveFsTreeTableModel->setFsTree( {}, {} );
        return;
    }
    else
    {
        if ( ! drivePtr->getRootHash() )
        {
            // TODO improved, check drive state before
            downloadLatestFsTree( drivePtr->getKey() );
            m_driveTreeModel->updateModel(false);
            m_diffTreeModel->updateModel(true);
        }
        else
        {
            m_driveFsTreeTableModel->setFsTree( drivePtr->getFsTree(), drivePtr->getLastOpenedPath() );
            ui->m_path->setText( "Path: " + QString::fromStdString(m_driveFsTreeTableModel->currentPathString()));
        }
    }
}

void MainWin::setupDrivesTab()
{
    setupDriveFsTable();

    //ui->m_driveFsTableView->setVisible(false);

    connect( ui->m_openLocalFolderBtn, &QPushButton::released, this, [this]
    {
        qDebug() << LOG_SOURCE << "openLocalFolderBtn";

        auto* driveInfo = m_model->currentDrive();
        if ( driveInfo != nullptr )
        {
            qDebug() << LOG_SOURCE << "driveInfo->m_localDriveFolder: " << driveInfo->getLocalFolder().c_str();
            std::error_code ec;
            if ( ! fs::exists(driveInfo->getLocalFolder(), ec) )
            {
                fs::create_directories( driveInfo->getLocalFolder(), ec );
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
        m_model->stestInitDrives();
    }

    if ( m_model->currentDriveIndex() >= 0 )
    {
        m_model->setCurrentDriveIndex( m_model->currentDriveIndex() );
        if ( auto* drivePtr = m_model->currentDrive(); drivePtr != nullptr )
        {
            drivePtr->setWaitingFsTree(true);
            downloadLatestFsTree( drivePtr->getKey() );
        }
    }

    ui->m_driveCBox->clear();
    ui->m_driveCBox->addItem( "Loading..." );

//    updateDrivesCBox();

    // request FsTree info-hash
//    if ( ALEX_LOCAL_TEST )
//    {
//        auto* driveInfoPtr = m_model->currentDrive();
//        std::array<uint8_t,32> fsTreeHash;
//        sirius::utils::ParseHexStringIntoContainer( ROOT_HASH1,
//                                                        64, fsTreeHash );

//        qDebug() << LOG_SOURCE << "driveHash: " << driveInfoPtr->getKey().c_str();

//        m_model->downloadFsTree( driveInfoPtr->getKey(),
//                               "0000000000000000000000000000000000000000000000000000000000000000",
//                               fsTreeHash,
//                               [this] ( const std::string&           driveHash,
//                                        const std::array<uint8_t,32> fsTreeHash,
//                                        const sirius::drive::FsTree& fsTree )
//        {
//            m_model->onFsTreeForDriveReceived( driveHash, fsTreeHash, fsTree );

//            qDebug() << LOG_SOURCE << "driveHash: " << driveHash.c_str();

//            if ( driveHash == m_model->currentDrive()->getKey() )
//            {
//                qDebug() << LOG_SOURCE << "m_calcDiffBtn->setEnabled: " << driveHash.c_str();
//                ui->m_calcDiffBtn->setEnabled(true);
//            }
//        });
//    }

    m_driveTreeModel = new DriveTreeModel(m_model, false, this);
    ui->m_driveTreeView->setModel( m_driveTreeModel );
    ui->m_driveTreeView->setTreePosition(1);
    ui->m_driveTreeView->setColumnHidden(0,true);
    ui->m_driveTreeView->header()->resizeSection(1, 300);
    ui->m_driveTreeView->header()->resizeSection(2, 30);
    ui->m_driveTreeView->header()->setDefaultAlignment(Qt::AlignLeft);
    ui->m_driveTreeView->setHeaderHidden(true);
    ui->m_driveTreeView->show();
    ui->m_driveTreeView->expandAll();

    m_diffTableModel = new DiffTableModel(m_model);

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

        if ( auto* driveInfo = m_model->currentDrive(); driveInfo != nullptr )
        {
            std::string driveName = driveInfo->getName();
            std::string driveKey = driveInfo->getKey();
            lock.unlock();

            EditDialog dialog( "Rename drive", "Drive name:", driveName );
            if ( dialog.exec() == QDialog::Accepted )
            {
                std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
                if ( auto* driveInfo = m_model->findDrive(driveKey); driveInfo != nullptr )
                {
                    driveInfo->getName() = driveName;
                    m_model->saveSettings();
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

        if ( auto* driveInfo = m_model->currentDrive(); driveInfo != nullptr )
        {
            std::string driveName = driveInfo->getName();
            std::string driveKey = driveInfo->getKey();
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
                if ( auto* driveInfo = m_model->findDrive(driveKey); driveInfo != nullptr )
                {
                    if (path.toStdString() != driveInfo->getLocalFolder()) {
                        m_modificationsWatcher->removePath(driveInfo->getLocalFolder().c_str());
                        m_modificationsWatcher->addPath(path.toStdString().c_str());
                        driveInfo->setLocalFolder(path.toStdString());
                        driveInfo->setLocalFolderExists(true);
                        m_model->saveSettings();
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
        StoragePaymentDialog dialog(m_onChainClient, m_model, this);
        dialog.exec();
    });

    QAction* copyDriveKeyAction = new QAction("Copy drive key", this);
    menu->addAction(copyDriveKeyAction);
    connect( copyDriveKeyAction, &QAction::triggered, this, [this](bool)
    {
        qDebug() << "TODO: copyDriveKeyAction";

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        if ( auto* driveInfo = m_model->currentDrive(); driveInfo != nullptr )
        {
            std::string driveKey = driveInfo->getKey();
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

    for( const auto& drive: m_model->getDrives() )
    {
        if ( drive.getState() == creating )
        {
            ui->m_driveCBox->addItem( QString::fromStdString( drive.getName() + "(creating...)") );
        }
        else if ( drive.getState() == deleting )
        {
            ui->m_driveCBox->addItem( QString::fromStdString( drive.getName() + "(...deleting)" ) );
        }
        else
        {
            ui->m_driveCBox->addItem( QString::fromStdString( drive.getName()) );
        }
    }

    if ( m_model->currentDrive() == nullptr && !m_model->getDrives().empty() )
    {
        m_model->setCurrentDriveIndex(0);
    }

    if ( m_model->currentDriveIndex() >= 0 )
    {
        ui->m_driveCBox->setCurrentIndex( m_model->currentDriveIndex() );
    }

    m_driveTreeModel->updateModel(false);
    ui->m_driveTreeView->expandAll();

    if ( auto* drive = m_model->currentDrive(); drive != nullptr )
    {
        m_driveFsTreeTableModel->setFsTree( drive->getFsTree(), drive->getLastOpenedPath() );
    }

    if ( auto* drivePtr = m_model->currentDrive(); drivePtr != nullptr )
    {
        m_lastSelectedDriveKey = drivePtr->getKey();
    }
    else
    {
        m_lastSelectedDriveKey.clear();
    }
}

void MainWin::lockChannel(const std::string &channelId) {
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    auto channel = m_model->findChannel(channelId);
    if (channel && (channel->isCreating() || channel->isDeleting())) {
        ui->m_closeChannel->setDisabled(true);
        ui->m_moreChannelsBtn->setDisabled(true);
        ui->m_channelFsTableView->setDisabled(true);
        ui->m_refresh->setDisabled(true);
        ui->m_downloadBtn->setDisabled(true);
    }
}

void MainWin::unlockChannel(const std::string &channelId) {
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    auto channel = m_model->findChannel(channelId);
    if ((channel && !channel->isCreating() && !channel->isDeleting()) || (m_model->getDownloadChannels().empty())) {
        ui->m_closeChannel->setEnabled(true);
        ui->m_moreChannelsBtn->setEnabled(true);
        ui->m_channelFsTableView->setEnabled(true);
        ui->m_refresh->setEnabled(true);
        ui->m_downloadBtn->setEnabled(true);
    }
}

void MainWin::lockDrive(const std::string &driveId) {
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    auto drive = m_model->findDrive(driveId);
    if (drive && (drive->getState() == creating || drive->getState() == deleting)) {
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
    auto drive = m_model->findDrive(driveId);
    if ((drive && drive->getState() != creating && drive->getState() != deleting) || (m_model->getDrives().empty())) {
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

void MainWin::downloadLatestFsTree( const std::string& driveKey )
{
    qDebug() << LOG_SOURCE << "@ download FsTree: " << driveKey.c_str();

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    Drive* drivePtr = m_model->findDrive( driveKey );
    if (!drivePtr) {
        qWarning() << LOG_SOURCE << "bad drive key: " << driveKey;
        return;
    }

    //
    // Check that fsTree is not now downloading
    //
    bool isFsTreeNowDownloading = drivePtr->isDownloadingFsTree();
    drivePtr->setDownloadingFsTree(true);
    m_model->applyForChannels( drivePtr->getKey(), [&](DownloadChannel& channel)
    {
        isFsTreeNowDownloading = isFsTreeNowDownloading || channel.isWaitingFsTree();
        channel.setWaitingFsTree(true);
    });

    if ( isFsTreeNowDownloading )
    {
        qDebug() << LOG_SOURCE << "@ download FsTree: isFsTreeAlreadyDownloading";
        return;
    }

    //
    // Get drive root hash
    //
    m_onChainClient->getBlockchainEngine()->getDriveById( drivePtr->getKey(),
                                                          [driveKey=drivePtr->getKey(),this]
                                                          (auto drive, auto isSuccess, auto message, auto code )
    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        Drive* drivePtr = m_model->findDrive( driveKey );
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

            drivePtr->setDownloadingFsTree(false);
            drivePtr->setWaitingFsTree(false);
            m_model->applyForChannels( drivePtr->getKey(), [&](DownloadChannel& channel)
            {
                channel.setWaitingFsTree(false);
            });

            lock.unlock();

            if (drivePtr->getState() == no_modifications) {
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

        bool rootHashIsNotChanged = drivePtr->getRootHash() && *drivePtr->getRootHash() == fsTreeHash;

        m_model->applyForChannels( driveKey, [&] (const DownloadChannel& channel )
        {
            rootHashIsNotChanged = rootHashIsNotChanged && channel.getFsTreeHash() && channel.getFsTreeHash() == fsTreeHash;
        });

        if ( rootHashIsNotChanged )
        {
            qDebug() << LOG_SOURCE << "rootHash Is Not Changed";

            drivePtr->setDownloadingFsTree(false);
            m_model->applyForChannels( driveKey, [&] (DownloadChannel& channel)
            {
                channel.setWaitingFsTree(false);
            });

            if ( auto* channel = m_model->currentDownloadChannel(); channel != nullptr && channel->getDriveKey() == driveKey )
            {
                m_channelFsTreeTableModel->updateRows();
            }

            if ( drivePtr->isWaitingFsTree() )
            {
                continueCalcDiff( *drivePtr );
            }
            else
            {
                m_driveFsTreeTableModel->setFsTree( drivePtr->getFsTree(), drivePtr->getLastOpenedPath() );
            }

            return;
        }

        // Check previously saved FsTree-s
        {
            auto fsTreeSaveFolder = getFsTreesFolder()/sirius::drive::toString(fsTreeHash);
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
                    fsTree = {};
                    fsTree.addFile( {}, std::string("!!! bad FsTree: ") + ex.what(),{},0);
                }
                onFsTreeReceived( driveKey, fsTreeHash, fsTree );
                return;
            }
        }

        m_model->downloadFsTree( driveKey,
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

void MainWin::onFsTreeReceived( const std::string& driveKey, const std::array<uint8_t,32>& fsTreeHash, const sirius::drive::FsTree& fsTree )
{
    qDebug() << LOG_SOURCE << "@ on FsTree received. DriveKey: " << driveKey.c_str();
    fsTree.dbgPrint();

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto drivePtr = m_model->findDrive( driveKey );
    if (!drivePtr) {
        qWarning() << LOG_SOURCE << "bad drive";
        return;
    }

    drivePtr->setRootHash(fsTreeHash);
    drivePtr->setFsTree(fsTree);
    drivePtr->setDownloadingFsTree(false);

    m_model->applyFsTreeForChannels(driveKey, fsTree, fsTreeHash);

    if ( auto* channel = m_model->currentDownloadChannel(); channel != nullptr && channel->getDriveKey() == driveKey )
    {
        qDebug() << LOG_SOURCE << "@ on FsTree received: channelName:" << channel->getName().c_str();
        channel->setFsTreeHash(fsTreeHash);
        channel->setFsTree(fsTree);
        channel->setWaitingFsTree(false);
        m_channelFsTreeTableModel->setFsTree( fsTree, channel->getLastOpenedPath() );
    }

    if (  drivePtr->isWaitingFsTree() )
    {
        continueCalcDiff( *drivePtr );
    }
}

// TODO: rework!
void MainWin::continueCalcDiff( Drive& drive )
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    drive.setWaitingFsTree(false);


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

    if ( auto drivePtr = m_model->currentDrive(); drivePtr != nullptr )
    {
        drivePtr->setWaitingFsTree(true);
        downloadLatestFsTree( drivePtr->getKey() );
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
        m_model->setWindowGeometry(frameGeometry());
        m_model->saveSettings();
        lock.unlock();

        event->accept();
    }
}

void MainWin::addLocalModificationsWatcher() {
    auto drives = m_model->getDrives();
    for (const auto& drive : drives) {
        m_modificationsWatcher->addPath(drive.getLocalFolder().c_str());
    }
}
