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

    if ( !fs::exists( getFsTreesFolder() ) )
    {
        try {
            fs::create_directories( getFsTreesFolder() );
        } catch( const std::runtime_error& err ) {
            QMessageBox msgBox(this);
            msgBox.setText( QString::fromStdString( "Cannot create directory to store fsTree!") );
            msgBox.setInformativeText( QString::fromStdString( "Error: " ) + err.what() );
            msgBox.setStandardButtons( QMessageBox::Ok );
            msgBox.exec();
        }
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

        if (ui->m_channels->count() == 0) {
            lockChannel("");
        } else {
            unlockChannel("");
        }

        int dnChannelIndex = m_model->currentDownloadChannelIndex();
        if ( dnChannelIndex >= 0 && dnChannelIndex < m_model->getDownloadChannels().size())
        {
            ui->m_channels->setCurrentIndex( dnChannelIndex );
        }
        else if ( dnChannelIndex < Model::dnChannels().size() > 0 )
        {
            ui->m_channels->setCurrentIndex( 0 );
        }
        lock.unlock();

        updateChannelsCBox();
        for( auto& downloadInfo: Model::downloads() )
        {
            if ( ! downloadInfo.isCompleted() )
            {
                if ( Model::findChannel( downloadInfo.m_channelHash ) == nullptr )
                {
                    downloadInfo.m_channelIsOutdated = true;
                }
            }
        }

        connect( ui->m_channels, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWin::onCurrentChannelChanged, Qt::QueuedConnection);
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::drivesLoaded, this, [this](const std::vector<xpx_chain_sdk::drives_page::DrivesPage>& drivesPages)
    {
        qDebug() << LOG_SOURCE << "drivesLoaded pages amount: " << drivesPages.size();

        connect(m_model, &Model::driveStateChanged, this, [this, drivesPages] (const std::string& driveKey, int state) {
            if (drivesPages.empty() || drivesPages[0].pagination.totalEntries == 0) {
                disconnect(m_model);
                emit drivesInitialized();
            } else if (state == no_modifications) {
                m_model->setLoadedDrivesCount(m_model->getLoadedDrivesCount() + 1);
                if (m_model->getLoadedDrivesCount() == drivesPages[0].pagination.totalEntries) {
                    std::vector<Drive*> outdatedDrives;
                    for (const auto& page : drivesPages) {
                        for (const auto& remoteDrive : page.data.drives) {
                            auto drive = m_model->findDrive(remoteDrive.data.multisig);
                            if (!drive) {
                                qWarning () << "Drive not found (invalid pointer to drive). Drive initialization failed!";
                                continue;
                            }

                            if (remoteDrive.data.replicatorCount == 0) {
                                qWarning () << "Replicators list for the following drive is empty: " << drive->getKey() << " Initialized failed!";
                                continue;
                            }

                            const auto rootHashUpperCase = QString::fromStdString(sirius::drive::toString(drive->getRootHash())).toUpper().toStdString();
                            auto fsTreeSaveFolder = getFsTreesFolder()/rootHashUpperCase;
                            bool isFsTreeExists = fs::exists( fsTreeSaveFolder / FS_TREE_FILE_NAME );

                            const auto remoteRootHash = rawHashFromHex(remoteDrive.data.rootHash.c_str());
                            if (!isFsTreeExists && drive->getRootHash() == remoteRootHash && Model::isZeroHash(remoteRootHash)) {
                                drive->setFsTree({});
                            }
                            else if (drive->getRootHash() != remoteRootHash || !isFsTreeExists) {
                                drive->setRootHash(remoteRootHash);
                                outdatedDrives.push_back(drive);
                            } else if (isFsTreeExists && drive->getRootHash() == remoteRootHash) {
                                sirius::drive::FsTree fsTree;
                                fsTree.deserialize(fsTreeSaveFolder / FS_TREE_FILE_NAME);
                                drive->setFsTree(fsTree);
                            }
                        }
                    }

                    if (outdatedDrives.empty()) {
                        disconnect(m_model);
                        emit drivesInitialized();
                    } else {
                        connect(m_onChainClient->getStorageEngine(), &StorageEngine::fsTreeReceived, this,
                        [this, counter = (int)outdatedDrives.size()] (auto driveKey, auto fsTreeHash, auto fsTree)
                        {
                            m_model->setOutdatedDriveNumber(m_model->getOutdatedDriveNumber() + 1);
                            onFsTreeReceived(driveKey, fsTreeHash, fsTree);
                            if (counter == m_model->getOutdatedDriveNumber()) {
                                disconnect(m_model);
                                disconnect(m_onChainClient->getStorageEngine());
                                m_model->saveSettings();
                                emit drivesInitialized();
                            }
                        }, Qt::QueuedConnection);
                    }

                    for (auto outdatedDrive : outdatedDrives) {
                        const auto fileStructureCDI = rawHashToHex(outdatedDrive->getRootHash()).toStdString();
                        onDownloadFsTreeDirect(outdatedDrive->getKey(), fileStructureCDI);
                    }
                }
            }
        }, Qt::QueuedConnection);

        // add drives created on another devices
        m_model->onDrivesLoaded(drivesPages);
        m_model->setDrivesLoaded(true);

        connect( m_onChainClient, &OnChainClient::downloadFsTreeDirect, this, &MainWin::onDownloadFsTreeDirect, Qt::QueuedConnection);

        // load my own channels
        xpx_chain_sdk::DownloadChannelsPageOptions options;
        options.consumerKey = m_model->getClientPublicKey();
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

    connect(this, &MainWin::drivesInitialized, this, [this]() {
        for (const auto& [key, drive] : m_model->getDrives()) {
            addDriveToUi(drive);
            if (boost::iequals(key, m_model->currentDriveKey()) ) {
                ui->m_drivePath->setText( "Path: " + QString::fromStdString(drive.getLocalFolder()));
                onDriveStateChanged(drive.getKey(), drive.getState());
            }
        }

        if (!m_model->getDrives().contains(m_model->currentDriveKey()) || m_model->currentDriveKey().empty()) {
            const auto driveKey = ui->m_driveCBox->currentData().toString().toStdString();
            m_model->setCurrentDriveKey(driveKey);
            setCurrentDriveOnUi(driveKey);
            const auto drive = m_model->getDrives()[driveKey];
            ui->m_drivePath->setText( "Path: " + QString::fromStdString(drive.getLocalFolder()));
            onDriveStateChanged(drive.getKey(), drive.getState());
        } else {
            setCurrentDriveOnUi(m_model->currentDriveKey());
        }

        addLocalModificationsWatcher();

        connect(m_model, &Model::driveStateChanged, this, &MainWin::onDriveStateChanged, Qt::QueuedConnection);
        connect(ui->m_driveCBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWin::onCurrentDriveChanged, Qt::QueuedConnection);
        connect(m_onChainClient->getStorageEngine(), &StorageEngine::fsTreeReceived, this, &MainWin::onFsTreeReceived, Qt::QueuedConnection);

        lockMainButtons(false);
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
        dialog.exec();
    }, Qt::QueuedConnection);

    connect(ui->m_closeDrive, &QPushButton::released, this, [this] () {
        auto drive = m_model->currentDrive();
        if (!drive) {
            qWarning() << LOG_SOURCE << "bad drive";
            return;
        }

        CloseDriveDialog dialog(m_onChainClient, drive, this);
        dialog.exec();
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::prepareDriveTransactionConfirmed, this, [this](auto driveKey) {
        onDriveCreationConfirmed( sirius::drive::toString( driveKey ) );
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::prepareDriveTransactionFailed, this, [this](auto driveKey, auto errorText) {
        onDriveCreationFailed( sirius::drive::toString( driveKey ), errorText.toStdString() );
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::closeDriveTransactionConfirmed, this, &MainWin::onDriveCloseConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::closeDriveTransactionFailed, this, &MainWin::onDriveCloseFailed, Qt::QueuedConnection);
    connect( ui->m_applyChangesBtn, &QPushButton::released, this, &MainWin::onApplyChanges, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::initializedSuccessfully, this, [this](auto networkName) {
        qDebug() << "network layer initialized successfully";

        loadBalance();
        ui->m_networkName->setText(networkName);

        xpx_chain_sdk::DrivesPageOptions options;
        options.owner = m_model->getClientPublicKey();
        m_onChainClient->loadDrives(options);

        if (ui->m_channels->count() == 0) {
            lockChannel("");
        } else {
            unlockChannel("");
        }

        if (ui->m_driveCBox->count() == 0) {
            lockDrive();
        } else {
            unlockDrive();
        }
    }, Qt::QueuedConnection);

    connect(ui->m_onBoardReplicator, &QPushButton::released, this, [this](){
        ReplicatorOnBoardingDialog dialog(this);
        dialog.exec();
    });

    connect(ui->m_offBoardReplicator, &QPushButton::released, this, [this](){
        ReplicatorOffBoardingDialog dialog(this);
        dialog.exec();
    });

    connect(m_onChainClient, &OnChainClient::internalError, this, &MainWin::onInternalError, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::dataModificationTransactionConfirmed, this, &MainWin::onDataModificationTransactionConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::dataModificationTransactionFailed, this, &MainWin::onDataModificationTransactionFailed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::dataModificationApprovalTransactionConfirmed, this, &MainWin::onDataModificationApprovalTransactionConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::dataModificationApprovalTransactionFailed, this, &MainWin::onDataModificationApprovalTransactionFailed, Qt::QueuedConnection);
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
    };

    m_modifyProgressPanel = new ModifyProgressPanel( m_model, 350, 350, this, modifyPanelCallback );
    m_modifyProgressPanel->setVisible(false);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index != 1) {
            m_modifyProgressPanel->setVisible(false);
        }

        // Downloads tab
        if (index == 0) {
            if (ui->m_channels->count() == 0) {
                lockChannel("");
            } else {
                unlockChannel("");
            }
        }

        // Drives tab
        if (index == 1) {
            if (ui->m_driveCBox->count() == 0) {
                lockDrive();
            } else {
                unlockDrive();
            }
        }

        auto drive = m_model->currentDrive();
        if (index == 1 && !m_model->getDrives().empty() && drive) {
            onDriveStateChanged(drive->getKey(), drive->getState());
        }
    }, Qt::QueuedConnection);
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
        qWarning() << "MainWin::cancelModification. Unknown drive";
        return;
    }

    CancelModificationDialog dialog(m_onChainClient, drive->getKey().c_str(), drive->getName().c_str(), this);
    dialog.exec();
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

    auto channelInfoAction = new QAction("Channel info", this);
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
        if (!index.isValid()) {
            return;
        }

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
        if (index.isValid()) {
            selectChannelFsItem( index.row() );
        }
    }, Qt::QueuedConnection);

    connect( ui->m_channelFsTableView, &QTableView::clicked, this, [this] (const QModelIndex &index)
    {
        if (index.isValid()) {
            selectChannelFsItem( index.row() );
        }
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
        if (!index.isValid()) {
            return;
        }

        int toBeSelectedRow = m_driveTableModel->onDoubleClick(index.row() );
        ui->m_driveFsTableView->selectRow( toBeSelectedRow );
        if ( auto* drivePtr = m_model->currentDrive(); drivePtr != nullptr )
        {
            drivePtr->setLastOpenedPath(m_driveTableModel->currentPath());
            qDebug() << LOG_SOURCE << "m_lastOpenedPath: " << drivePtr->getLastOpenedPath();
        }
    }, Qt::QueuedConnection);

    connect( ui->m_driveFsTableView, &QTableView::pressed, this, [this] (const QModelIndex &index)
    {
        if (index.isValid()) {
            selectDriveFsItem( index.row() );
        }
    }, Qt::QueuedConnection);

    connect( ui->m_driveFsTableView, &QTableView::clicked, this, [this] (const QModelIndex &index)
    {
        if (index.isValid()) {
            selectDriveFsItem( index.row() );
        }
    }, Qt::QueuedConnection);

    m_driveTableModel = new FsTreeTableModel(m_model, false );

    ui->m_driveFsTableView->setModel(m_driveTableModel );
    ui->m_driveFsTableView->horizontalHeader()->hide();
    ui->m_driveFsTableView->setColumnWidth(0,300);
    ui->m_driveFsTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_driveFsTableView->setGridStyle( Qt::NoPen );
    ui->m_driveFsTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    m_diffTreeModel = new DriveTreeModel(m_model, true, this);
    ui->m_diffTreeView->setModel( m_diffTreeModel );
    ui->m_diffTreeView->setTreePosition(1);
    ui->m_diffTreeView->setColumnHidden(0,true);
    ui->m_diffTreeView->header()->resizeSection(1, 300);
    ui->m_diffTreeView->header()->resizeSection(2, 30);
    ui->m_diffTreeView->header()->setDefaultAlignment(Qt::AlignLeft);
    ui->m_diffTreeView->setHeaderHidden(true);
    ui->m_diffTreeView->show();
    ui->m_diffTreeView->expandAll();

}

void MainWin::selectDriveFsItem( int index )
{
    if ( index < 0 && index >= m_driveTableModel->m_rows.size() )
    {
        return;
    }

    ui->m_driveFsTableView->selectRow( index );

    if (!m_driveTableModel->m_rows.empty()) {
        ui->m_downloadBtn->setEnabled( ! m_driveTableModel->m_rows[index].m_isFolder );
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

        auto channel = m_model->currentDownloadChannel();
        if ( channel )
        {
            auto ltHandle = Model::downloadFile( channelId->m_hash,  hash );

            m_downloadsTableModel->beginResetModel();
            Model::downloads().insert( Model::downloads().begin(), DownloadInfo{ hash, channelId->m_hash, name,
                                                                                 Model::downloadFolder(),
                                                                                 false, 0, false, ltHandle } );
//            Model::downloads().emplace_back( DownloadInfo{ hash, channelId->m_hash, name,
//                                                           Model::downloadFolder(),
//                                                           false, 0, ltHandle } );
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
    ui->m_downloadsTableView->horizontalHeader()->setStretchLastSection(false);
    ui->m_downloadsTableView->setColumnWidth(1,60);
    ui->m_downloadsTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);


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
        auto& downloads = m_model->downloads();

        if ( ! rows.empty() && rowIndex >= 0 && rowIndex < downloads.size() )
        {
            DownloadInfo dnInfo = downloads[rowIndex];
            lock.unlock();

            QMessageBox msgBox;
            const QString message = QString::fromStdString("'" + dnInfo.getFileName() + "' will be removed.");
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

void MainWin::checkDriveForUpdates(Drive* drive, const std::function<void(bool)>& callback)
{
    if (!drive) {
        qWarning() << "MainWin::checkDriveForUpdates. Invalid pointer to drive";
        return;
    }

    qDebug () << "MainWin::checkDriveForUpdates. Drive key: " << drive->getKey();

    m_onChainClient->getBlockchainEngine()->getDriveById( drive->getKey(),
    [drive, callback] (auto remoteDrive, auto isSuccess, auto message, auto code ) {
        bool result = false;
        if (!isSuccess) {
            qWarning() << "MainWin::checkDriveForUpdates:: callback(false): " << message.c_str() << " : " << code.c_str();
            callback(result);
            return;
        }

        drive->setReplicators(remoteDrive.data.replicators);

        const auto remoteRootHash = rawHashFromHex(remoteDrive.data.rootHash.c_str());
        if (remoteRootHash != drive->getRootHash()) {
            drive->setRootHash(remoteRootHash);
            result = true;
        }

        callback(result);
    });
}

void MainWin::checkDriveForUpdates(DownloadChannel* channel, const std::function<void(bool)>& callback)
{
    if (!channel) {
        qWarning() << "MainWin::checkDriveForUpdates. Invalid pointer to channel";
        callback(false);
        return;
    }

    qDebug () << "MainWin::checkDriveForUpdates. Channel key: " << channel->getKey();

    m_onChainClient->getBlockchainEngine()->getDriveById( channel->getDriveKey(),
    [channel, callback] (auto remoteDrive, auto isSuccess, auto message, auto code ) {
        bool result = false;
        if (!isSuccess) {
            qWarning() << "MainWin::checkDriveForUpdates:: callback(false): " << message.c_str() << " : " << code.c_str();
            callback(result);
            return;
        }

        const auto remoteRootHash = rawHashFromHex(remoteDrive.data.rootHash.c_str());
        if (remoteRootHash != channel->getFsTreeHash()) {
            channel->setFsTreeHash(remoteRootHash);
            result = true;
        }

        callback(result);
    });
}

void MainWin::updateReplicatorsForChannel(const std::string& channelId, const std::function<void()>& callback)
{
    auto channel = m_model->findChannel(channelId);
    if (!channel) {
        qWarning() << "MainWin::updateReplicatorsForChannel. Invalid pointer to channel: " << channelId;
        callback();
        return;
    }

    qDebug () << "MainWin::updateReplicatorsForChannel. Channel key: " << channelId;

    m_onChainClient->getBlockchainEngine()->getDownloadChannelById( channelId,
    [channel, callback] (auto remoteChannel, auto isSuccess, auto message, auto code ) {
        bool result = false;
        if (!isSuccess) {
            qWarning() << "MainWin::updateReplicatorsForChannel:: callback(): " << message.c_str() << " : " << code.c_str();
            callback();
            return;
        }

        channel->setReplicators(remoteChannel.data.shardReplicators);
        callback();
    });
}

void MainWin::onInternalError(const QString& errorText)
{
    showNotification(errorText);
    addNotification(errorText);
}

void MainWin::setCurrentDriveOnUi(const std::string& driveKey)
{
    auto drive = m_model->findDrive(driveKey);
    if (!drive) {
        qWarning() << "MainWin::setCurrentDriveOnUi. Unknown drive: " << driveKey;
        if (ui->m_driveCBox->count() > 0) {
            ui->m_driveCBox->setCurrentIndex(0);
            m_model->setCurrentDriveKey( ui->m_driveCBox->currentData().toString().toStdString() );
        }

        return;
    }

    const auto currentDriveKey = ui->m_driveCBox->currentData().toString();
    if ( ! boost::iequals(driveKey, currentDriveKey.toStdString())) {
        const int index = ui->m_driveCBox->findData(QVariant::fromValue(QString::fromStdString(driveKey)), Qt::UserRole, Qt::MatchFixedString);
        if (index != -1) {
            ui->m_driveCBox->setCurrentIndex(index);
        } else {
            qWarning() << "MainWin::setCurrentDriveOnUi. Drive not found in UI: " << driveKey;
        }
    }
}

void MainWin::onDriveStateChanged(const std::string& driveKey, int state)
{
    auto drive = m_model->findDrive(driveKey);
    if (!drive) {
        qWarning() << "MainWin::onDriveStateChanged. Invalid drive";
        return;
    }

    qInfo() << "MainWin::onDriveStateChanged: " << drive->getKey() << " state: " << getPrettyDriveState(state);

    switch(state)
    {
        case no_modifications:
        {
            m_modifyProgressPanel->setVisible(false);
            updateDriveStatusOnUi(*drive);

            m_model->calcDiff();

            m_driveTableModel->setFsTree(drive->getFsTree(), { drive->getLocalFolder() } );
            m_diffTableModel->updateModel();

            m_driveTreeModel->updateModel(false);
            ui->m_driveTreeView->expand(m_driveTreeModel->index(0, 0));

            m_diffTreeModel->updateModel(false);
            ui->m_diffTreeView->expand(m_diffTreeModel->index(0, 0));

            if (isCurrentDrive(drive)) {
                unlockDrive();
            }

            if (isCurrentDrive(drive) && ui->tabWidget->currentIndex() == 1)  {
                unlockDrive();
                loadBalance();
            }

            break;
        }
        case registering:
        {
            if (isCurrentDrive(drive) && ui->tabWidget->currentIndex() == 1) {
                m_modifyProgressPanel->setRegistering();
                m_modifyProgressPanel->setVisible(true);
                lockDrive();
            }
            break;
        }
        case approved:
        {
            if (isCurrentDrive(drive) && ui->tabWidget->currentIndex() == 1) {
                m_modifyProgressPanel->setApproved();
                m_modifyProgressPanel->setVisible(true);

                //startCalcDiff();
                //onRefresh();

                QString message;
                message.append("Your modification was applied. Drive: ");
                message.append(drive->getName().c_str());
                addNotification(message);
                loadBalance();
                unlockDrive();
            }

            break;
        }
        case failed:
        {
            if (isCurrentDrive(drive) && ui->tabWidget->currentIndex() == 1) {
                m_modifyProgressPanel->setFailed();
                m_modifyProgressPanel->setVisible(true);

                const QString message = "Your modification was declined.";
                addNotification(message);

                loadBalance();
                unlockDrive();
            }

            break;
        }
        case canceling:
        {
            if (isCurrentDrive(drive) && ui->tabWidget->currentIndex() == 1) {
                m_modifyProgressPanel->setCanceling();
                m_modifyProgressPanel->setVisible(true);
                lockDrive();
            }

            break;
        }
        case canceled:
        {
            if (isCurrentDrive(drive) && ui->tabWidget->currentIndex() == 1) {
                m_modifyProgressPanel->setCanceled();
                m_modifyProgressPanel->setVisible(true);

                loadBalance();
                unlockDrive();
            }

            break;
        }
        case uploading:
        {
            if (isCurrentDrive(drive) && ui->tabWidget->currentIndex() == 1) {
                m_modifyProgressPanel->setUploading();
                m_modifyProgressPanel->setVisible(true);
                lockDrive();
            }

            break;
        }
        case creating:
        {
            addDriveToUi(*drive);
            setCurrentDriveOnUi(drive->getKey());
            updateDriveStatusOnUi(*drive);

            ui->m_drivePath->setText( "Path: " + QString::fromStdString(drive->getLocalFolder()));
            lockDrive();

            break;
        }
        case unconfirmed:
        {
            removeDriveFromUi(*drive);
            m_model->removeDrive(drive->getKey());
            m_model->removeChannelByDriveKey(drive->getKey());

            // TODO optimize the functions
            updateChannelsCBox();
            break;
        }
        case deleting:
        {
            lockDrive();
            updateDriveStatusOnUi(*drive);

            m_model->markChannelsForDelete(drive->getKey(), true);
            auto channel = m_model->currentDownloadChannel();
            if (channel) {
                lockChannel(channel->getKey());
            }

            // TODO optimize the functions
            updateChannelsCBox();

            break;
        }
        case deleted:
        {
            std::string driveName = drive->getName();

            removeDriveFromUi(*drive);
            m_model->removeDrive(drive->getKey());
            m_model->removeChannelByDriveKey(drive->getKey());
            m_diffTableModel->updateModel();

            // TODO optimize the functions
            updateChannelsCBox();

            const QString message = QString::fromStdString( "Drive '" + driveName + "' closed (removed).");
            showNotification(message);
            addNotification(message);

            loadBalance();

            if (ui->m_driveCBox->count() == 0) {
                lockDrive();
            }

            break;
        }
        default:
        {
            qWarning() << "Unknown drive state: " << getPrettyDriveState(state);
            break;
        }
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
    m_model->addDownloadChannel(newChannel);
    m_model->setCurrentDownloadChannelIndex((int) m_model->getDownloadChannels().size() - 1);

    auto drive = m_model->findDrive(driveKey);
    if (drive) {
        m_model->applyFsTreeForChannels(driveKey, drive->getFsTree(), drive->getRootHash());
    }

    updateChannelsCBox();
    lockChannel(channelKey);
}

void MainWin::onChannelCreationConfirmed( const std::string& alias, const std::string& channelKey, const std::string& driveKey )
{
    qDebug() << "MainWin::onChannelCreationConfirmed: alias:" << alias << " key: " << channelKey;
    auto channel = m_model->findChannel( channelKey );
    if ( channel )
    {
        channel->setCreating(false);
        m_model->saveSettings();

        updateChannelsCBox();
        onRefresh();

        const QString message = QString::fromStdString( "Channel '" + alias + "' created successfully.");
        showNotification(message);
        addNotification(message);

        unlockChannel(channelKey);
    } else
    {
        qWarning() << "MainWin::onChannelCreationConfirmed. Unknown channel " << channelKey;
    }
}

void MainWin::onChannelCreationFailed( const std::string& channelKey, const std::string& errorText )
{
    qDebug() << "MainWin::onChannelCreationFailed: key:" << channelKey << " error: " << errorText;

    auto channelInfo = m_model->findChannel( channelKey );
    if (channelInfo) {
        const QString message = QString::fromStdString( "Channel creation failed (" + channelInfo->getName() + ")\n It will be removed.");
        showNotification(message, errorText.c_str());
        addNotification(message);

        m_model->removeChannel(channelKey);
        updateChannelsCBox();
        unlockChannel(channelInfo->getKey());
    } else {
        qWarning() << "MainWin::onChannelCreationFailed. Unknown channel: " << channelKey;
    }
}

void MainWin::onDriveCreationConfirmed( const std::string &driveKey )
{
    qDebug() << "MainWin::onDriveCreationConfirmed: key:" << driveKey;

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    auto drive = m_model->findDrive( driveKey );
    if ( drive )
    {
        m_modificationsWatcher->addPath(drive->getLocalFolder().c_str());
        drive->updateState(no_modifications);
        m_model->saveSettings();
        lock.unlock();

        const QString message = QString::fromStdString("Drive '" + drive->getName() + "' created successfully.");
        showNotification(message);
        addNotification(message);
    } else {
        qWarning() << "MainWin::onDriveCreationConfirmed. Unknown drive: " << driveKey;
    }
}

void MainWin::onDriveCreationFailed(const std::string& driveKey, const std::string& errorText)
{
    qDebug() << "MainWin::onDriveCreationFailed: key:" << driveKey << " error: " << errorText;

    auto drive = m_model->findDrive(driveKey);
    if (drive) {
        const QString message = QString::fromStdString( "Drive creation failed (" + drive->getName() + ")\n It will be removed.");
        showNotification(message, errorText.c_str());
        addNotification(message);
        drive->updateState(unconfirmed);
    } else {
        qWarning() << "MainWin::onDriveCreationFailed. Unknown drive: " << driveKey;
    }
}

void MainWin::onDriveCloseConfirmed(const std::array<uint8_t, 32>& driveKey) {
    const auto driveKeyHex = sirius::drive::toString(driveKey);

    qDebug() << "MainWin::onDriveCloseConfirmed: key:" << driveKeyHex;

    auto drive = m_model->findDrive(driveKeyHex);
    if (drive) {
        m_modificationsWatcher->removePath(drive->getLocalFolder().c_str());
        drive->updateState(deleted);
    } else {
        qWarning() << "MainWin::onDriveCloseConfirmed. Unknown drive: " << driveKeyHex;
    }
}

void MainWin::onDriveCloseFailed(const std::array<uint8_t, 32>& driveKey, const QString& errorText) {
    const auto driveId = rawHashToHex(driveKey).toStdString();

    qDebug() << "MainWin::onDriveCloseFailed: key:" << driveId;

    std::string alias = driveId;
    auto drive = m_model->findDrive(alias);
    if (drive) {
        alias = drive->getName();
        drive->updateState(no_modifications);
    } else {
        qWarning () << "MainWin::onDriveCloseFailed. Unknown drive: " << driveId << " error: " << errorText;
    }

    QString message = "The drive '";
    message.append(alias.c_str());
    message.append("' was not close by reason: ");
    message.append(errorText);
    showNotification(message);
    addNotification(message);
    m_model->markChannelsForDelete(driveId, false);
}

void MainWin::onApplyChanges()
{
    auto drive = m_model->currentDrive();
    if (!drive) {
        qWarning() << "MainWin::onApplyChanges. Unknown drive (Invalid pointer to current drive)";
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
}

void MainWin::onRefresh()
{
    auto channel = m_model->currentDownloadChannel();
    if (!channel) {
        qWarning () << "MainWin::onRefresh. Invalid pointer to channel!";
        return;
    }

    if (channel->isDownloadingFsTree()) {
        qWarning () << "MainWin::onRefresh. FsTree downloading still in progress. Channel key: " << channel->getKey();
        return;
    }

    auto updateReplicatorsCallback = [this, channel]() {
        auto driveUpdatesCallback = [this, channel](bool isOutdated) {
            if (isOutdated) {
                const auto rootHash = rawHashToHex(channel->getFsTreeHash()).toStdString();
                downloadFsTreeByChannel(channel->getKey(), rootHash);
            } else {
                m_channelFsTreeTableModel->setFsTree( channel->getFsTree(), channel->getLastOpenedPath() );
                ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));
            }

            loadBalance();
        };

        checkDriveForUpdates(channel, driveUpdatesCallback);
        loadBalance();
    };

    updateReplicatorsForChannel(channel->getKey(), updateReplicatorsCallback);
}

void MainWin::onDownloadChannelCloseConfirmed(const std::array<uint8_t, 32> &channelId) {

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    std::string channelName;
    const auto channelKey = sirius::drive::toString(channelId);
    auto channelPtr = m_model->findChannel( channelKey );
    if (channelPtr)
    {
        channelName = channelPtr->getName();
    }
    lock.unlock();

    m_model->removeChannel( sirius::drive::toString(channelId) );

    if (channelPtr)
    {
        const QString message = QString::fromStdString( "Channel '" + channelName + "' closed (removed).");
        showNotification(message);
        addNotification(message);
    }

    updateChannelsCBox();
    loadBalance();
    unlockChannel(channelKey);

    if (ui->m_channels->count() == 0) {
        lockChannel("");
    }
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
    }
    else
    {
        qDebug () << LOG_SOURCE << "Your last modification was NOT registered: !!! drive not found";
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
    }
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

    qDebug () << LOG_SOURCE << "Your modification was APPLIED. Drive: " +
                 QString::fromStdString(driveAlias) + " CDI: " +
                 QString::fromStdString(fileStructureCdi);
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

    QString message;
    message.append("Your modifications was DECLINED. Drive: ");
    message.append(driveAlias.c_str());
    showNotification(message);
	addNotification(message);
}

void MainWin::onCancelModificationTransactionConfirmed(const std::array<uint8_t, 32> &driveId, const QString &modificationId) {
    std::string driveRawId = rawHashToHex(driveId).toStdString();
    auto drive = m_model->findDrive(driveRawId);
    if (drive) {
        drive->updateState(canceled);
    } else {
        qWarning () << LOG_SOURCE << "bad drive: " << driveRawId.c_str() << " modification id: " << modificationId;
    }
}

void MainWin::onCancelModificationTransactionFailed(const std::array<uint8_t, 32> &driveId, const QString &modificationId) {
    std::string driveRawId = rawHashToHex(driveId).toStdString();
    auto drive = m_model->findDrive(driveRawId);
    if (drive) {
        drive->updateState(failed);
    } else {
        qWarning () << LOG_SOURCE << "bad drive: " << driveRawId.c_str() << " modification id: " << modificationId;
    }
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

        auto channel = m_model->currentDownloadChannel();
        if ( !channel )
        {
            qWarning() << LOG_SOURCE << "bad channel";
            m_channelFsTreeTableModel->setFsTree( {}, {} );
            return;
        }

        auto updateReplicatorsCallback = [this, channel]() {
            auto driveUpdatesCallback = [this, channel](bool isOutdated) {
                if (isOutdated) {
                    const auto rootHash = rawHashToHex(channel->getFsTreeHash()).toStdString();
                    downloadFsTreeByChannel(channel->getKey(), rootHash);
                } else {
                    m_channelFsTreeTableModel->setFsTree( channel->getFsTree(), channel->getLastOpenedPath() );
                    ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));
                }

                loadBalance();
            };

            checkDriveForUpdates(channel, driveUpdatesCallback);
            loadBalance();
        };

        updateReplicatorsForChannel(channel->getKey(), updateReplicatorsCallback);
        lockChannel(channel->getKey());
        unlockChannel(channel->getKey());
    }
}

void MainWin::onDriveLocalDirectoryChanged(const QString& path) {
    for (auto& drive : m_model->getDrives()) {
        if (drive.second.getLocalFolder() == path.toStdString() &&
            m_model->currentDrive() &&
            drive.second.getKey() == m_model->currentDrive()->getKey()) {

			// TODO: calc diff and update view only
            onDriveStateChanged(drive.second.getKey(), no_modifications);
            break;
        }
    }
}

void MainWin::onCurrentDriveChanged( int index )
{
    if (index >= 0) {
        const auto driveKey = ui->m_driveCBox->currentData().toString();
        m_model->setCurrentDriveKey( driveKey.toStdString() );

        auto drive = m_model->currentDrive();
        if (drive) {
            onDriveStateChanged(drive->getKey(), drive->getState());
            ui->m_drivePath->setText( "Path: " + QString::fromStdString(drive->getLocalFolder()));
        } else {
            qWarning() << "MainWin::onCurrentDriveChanged. Drive not found (Invalid pointer to drive)";
            m_driveTableModel->setFsTree({}, {} );
        }
    }
}

void MainWin::setupDrivesTab()
{
    setupDriveFsTable();
    connect( ui->m_openLocalFolderBtn, &QPushButton::released, this, [this]
    {
        qDebug() << LOG_SOURCE << "openLocalFolderBtn";

        auto driveInfo = m_model->currentDrive();
        if ( driveInfo )
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
        auto drive = m_model->currentDrive();
        if (drive) {
            auto callback = [this, drive](bool isOutdated) {
                if (isOutdated) {
                    const auto rootHash = rawHashToHex(drive->getRootHash()).toStdString();
                    onDownloadFsTreeDirect(drive->getKey(), rootHash);
                }

                m_model->calcDiff();
            };

            checkDriveForUpdates(drive, callback);
        }
    }, Qt::QueuedConnection);

    if ( ALEX_LOCAL_TEST )
    {
        m_model->stestInitDrives();
    }

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
    ui->m_diffTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_diffTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);


    auto menu = new PopupMenu(ui->m_moreDrivesBtn, this);
    auto renameAction = new QAction("Rename", this);
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
                    driveInfo->setName(driveName);
                    m_model->saveSettings();
                    lock.unlock();
                    updateDriveNameOnUi(*driveInfo);
                }
            }
        }
    });

    auto changeFolderAction = new QAction("Change local folder", this);
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
                if ( auto driveInfo = m_model->findDrive(driveKey); driveInfo )
                {
                    if (path.toStdString() != driveInfo->getLocalFolder()) {
                        m_modificationsWatcher->removePath(driveInfo->getLocalFolder().c_str());
                        m_modificationsWatcher->addPath(path.toStdString().c_str());
                        driveInfo->setLocalFolder(path.toStdString());
                        driveInfo->setLocalFolderExists(true);
                        onDriveStateChanged(driveInfo->getKey(), driveInfo->getState());
                        m_model->saveSettings();
                        ui->m_drivePath->setText( "Path: " + QString::fromStdString(driveInfo->getLocalFolder()));
                    }

                    lock.unlock();
                }
            }
        }
    });

    auto topUpAction = new QAction("Top-up", this);
    menu->addAction(topUpAction);
    connect( topUpAction, &QAction::triggered, this, [this](bool)
    {
        StoragePaymentDialog dialog(m_onChainClient, m_model, this);
        dialog.exec();
    });

    auto copyDriveKeyAction = new QAction("Copy drive key", this);
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

    auto driveInfoAction = new QAction("Drive info", this);
    menu->addAction(driveInfoAction);
    connect( driveInfoAction, &QAction::triggered, this, [this](bool)
    {
        qDebug() << "TODO: driveInfoAction";

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        if ( auto* driveInfo = m_model->currentDrive(); driveInfo != nullptr )
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

void MainWin::addNotification(const QString& message)
{
    // No memory leaks, m_notificationsWidget will take ownerships after insert
    auto item = new QListWidgetItem(tr(message.toStdString().c_str()));
    m_notificationsWidget->insertItem(0, item);
}

void MainWin::updateDriveNameOnUi(const Drive& drive)
{
    const int index = ui->m_driveCBox->findData(QVariant::fromValue(QString::fromStdString(drive.getKey())), Qt::UserRole, Qt::MatchFixedString);
    if (index != -1) {
        ui->m_driveCBox->setItemText(index, QString::fromStdString(drive.getName()));
    } else {
        qWarning () << "MainWin::updateDriveNameOnUi. Unknown drive";
    }
}

void MainWin::updateDriveStatusOnUi(const Drive& drive)
{
    const int index = ui->m_driveCBox->findData(QVariant::fromValue(QString::fromStdString(drive.getKey())), Qt::UserRole, Qt::MatchFixedString);
    if (index != -1) {
        if ( drive.getState() == creating ) {
            ui->m_driveCBox->setItemText(index, QString::fromStdString( drive.getName() + "(creating...)"));
        } else if (drive.getState() == deleting) {
            ui->m_driveCBox->setItemText(index, QString::fromStdString(drive.getName() + "(...deleting)"));
        } else if (drive.getState() == no_modifications) {
            ui->m_driveCBox->setItemText(index, QString::fromStdString(drive.getName()));
        }
    }
}

void MainWin::addDriveToUi(const Drive& drive)
{
    const int index = ui->m_driveCBox->findData(QVariant::fromValue(QString::fromStdString(drive.getKey())), Qt::UserRole, Qt::MatchFixedString);
    if (index == -1) {
        ui->m_driveCBox->addItem(QString::fromStdString(drive.getName()), QString::fromStdString(drive.getKey()));
        ui->m_driveCBox->model()->sort(0);
    }
};

void MainWin::removeDriveFromUi(const Drive& drive)
{
    const int index = ui->m_driveCBox->findData(QVariant::fromValue(QString::fromStdString(drive.getKey())), Qt::UserRole, Qt::MatchFixedString);
    if (index != -1) {
        ui->m_driveCBox->removeItem(index);
    }
}

void MainWin::lockChannel(const std::string &channelId) {
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    auto channel = m_model->findChannel(channelId);
    if ((channel && (channel->isCreating() || channel->isDeleting())) || channelId.empty())
    {
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
    if (((channel && !channel->isCreating() && !channel->isDeleting()) || (m_model->getDownloadChannels().empty())) || channelId.empty()) {
        ui->m_closeChannel->setEnabled(true);
        ui->m_moreChannelsBtn->setEnabled(true);
        ui->m_channelFsTableView->setEnabled(true);
        ui->m_refresh->setEnabled(true);
        ui->m_downloadBtn->setEnabled(true);
    }
}

void MainWin::lockDrive() {
    ui->m_closeDrive->setDisabled(true);
    ui->m_moreDrivesBtn->setDisabled(true);
    ui->m_driveTreeView->setDisabled(true);
    ui->m_driveFsTableView->setDisabled(true);
    ui->m_openLocalFolderBtn->setDisabled(true);
    ui->m_applyChangesBtn->setDisabled(true);
    ui->m_diffTableView->setDisabled(true);
    ui->m_calcDiffBtn->setDisabled(true);
}

void MainWin::unlockDrive() {
    ui->m_closeDrive->setEnabled(true);
    ui->m_moreDrivesBtn->setEnabled(true);
    ui->m_driveTreeView->setEnabled(true);
    ui->m_driveFsTableView->setEnabled(true);
    ui->m_openLocalFolderBtn->setEnabled(true);
    ui->m_applyChangesBtn->setEnabled(true);
    ui->m_diffTableView->setEnabled(true);
    ui->m_calcDiffBtn->setEnabled(true);
}

void MainWin::onDownloadFsTreeDirect(const std::string& driveId, const std::string& fileStructureCdi)
{
    qDebug()  << "MainWin::onDownloadFsTreeDirect. Drive key: " << driveId << " fileStructureCdi: " << fileStructureCdi;
    auto drive = m_model->findDrive(driveId);
    if (!drive) {
        qWarning() << "MainWin::onDownloadFsTreeDirect. Unknown drive (Invalid pointer to drive): " << driveId;
        return;
    }

    if (drive->isDownloadingFsTree()) {
        qWarning() << "MainWin::onDownloadFsTreeDirect. FsTree downloading still in progress: " << driveId;
        return;
    }

    drive->setDownloadingFsTree(true);
    const auto channelId = "0000000000000000000000000000000000000000000000000000000000000000";
    m_onChainClient->getStorageEngine()->downloadFsTree(driveId, channelId, rawHashFromHex(fileStructureCdi.c_str()), drive->getReplicators());
}

void MainWin::downloadFsTreeByChannel(const std::string& channelId, const std::string& fileStructureCdi)
{
    qDebug()  << "MainWin::downloadFsTreeByChannel. Channel key: " << channelId << " fileStructureCdi: " << fileStructureCdi;
    auto channel = m_model->findChannel(channelId);
    if (!channel) {
        qWarning() << "MainWin::downloadFsTreeByChannel. Unknown channel (Invalid pointer to channel): " << channelId;
        return;
    }

    if (channel->isDownloadingFsTree()) {
        qWarning() << "MainWin::downloadFsTreeByChannel. FsTree downloading still in progress(for channel). Channel key: " << channel->getKey();
        return;
    }

    auto drive = m_model->findDrive(channel->getDriveKey());
    if (drive) {
        qInfo() << "MainWin::downloadFsTreeByChannel. Drive found, download fsTree direct. Drive key: " << drive->getKey();
        onDownloadFsTreeDirect(drive->getKey(), fileStructureCdi);
    } else {
        channel->setDownloadingFsTree(true);
        m_onChainClient->getStorageEngine()->downloadFsTree(channel->getDriveKey(), channelId, rawHashFromHex(fileStructureCdi.c_str()), channel->getReplicators());
    }
}

void MainWin::onFsTreeReceived( const std::string& driveKey, const std::array<uint8_t,32>& fsTreeHash, const sirius::drive::FsTree& fsTree )
{
    qDebug()  << "MainWin::onFsTreeReceived. Drive key: " << driveKey.c_str();
    fsTree.dbgPrint();

    auto drive = m_model->findDrive( driveKey );
    if (drive) {
        qDebug() << "MainWin::onFsTreeReceived. FsTree downloaded and apply for drive: " << driveKey;
        drive->setDownloadingFsTree(false);
        drive->setRootHash(fsTreeHash);
        drive->setFsTree(fsTree);

        // TODO: fix, update view only. Do not close frame
        onDriveStateChanged(drive->getKey(), no_modifications);
    }

    m_model->applyFsTreeForChannels(driveKey, fsTree, fsTreeHash);
    if ( auto channel = m_model->currentDownloadChannel(); channel && boost::iequals( channel->getDriveKey(), driveKey ) )
    {
        m_channelFsTreeTableModel->setFsTree( fsTree, channel->getLastOpenedPath() );
    }

    m_model->saveSettings();
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
    for (const auto& [key, drive] : drives) {
        m_modificationsWatcher->addPath(drive.getLocalFolder().c_str());
    }
}

bool MainWin::isCurrentDrive(Drive* drive)
{
    auto currentDrive = m_model->currentDrive();
    if (drive && currentDrive) {
        return drive->getKey() == currentDrive->getKey();
    }

    return false;
}