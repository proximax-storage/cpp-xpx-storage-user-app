//#include "moc_mainwin.cpp"

#include "mainwin.h"
#include "./ui_mainwin.h"

#include "Dialogs/WizardAddStreamAnnounceDialog.h"
#include "Entities/Settings.h"
#include "Models/Model.h"
#include "Models/FsTreeTableModel.h"
#include "Models/DownloadsTableModel.h"
#include "Models/EasyDownloadTableModel.h"
#include "Models/DriveTreeModel.h"
#include "Models/DiffTableModel.h"
#include "QtGui/qclipboard.h"
#include "Dialogs/SettingsDialog.h"
#include "Dialogs/PrivKeyDialog.h"
#include "Dialogs/ChannelInfoDialog.h"
#include "Dialogs/DriveInfoDialog.h"
#include "Dialogs/AddDownloadChannelDialog.h"
#include "Dialogs/CloseChannelDialog.h"
#include "Dialogs/CancelModificationDialog.h"
#include "Dialogs/AddDriveDialog.h"
#include "Dialogs/CloseDriveDialog.h"
#include "Dialogs/DownloadPaymentDialog.h"
#include "Dialogs/StoragePaymentDialog.h"
#include "Dialogs/PasteLinkDialog.h"
#include "Dialogs/ConfirmLinkDialog.h"
#include "ReplicatorTreeItem.h"
#include "Models/ReplicatorTreeModel.h"
#include "Dialogs/ReplicatorOnBoardingDialog.h"
#include "Dialogs/ReplicatorOffBoardingDialog.h"
#include "Dialogs/ReplicatorInfoDialog.h"
#include "Dialogs/ModifyProgressPanel.h"
#include "Dialogs/StreamingPanel.h"
#include "PopupMenu.h"
#include "Dialogs/EditDialog.h"

#include "crypto/Signer.h"
#include "Entities/Drive.h"
#include "ErrorCodeTranslator/ErrorCodeTranslator.h"

#include <qdebug.h>
#include <QFileIconProvider>
#include <QScreen>
#include <QComboBox>
#include <QMessageBox>
#include <QDesktopServices>
#include <thread>
#include <QListWidget>
#include <QAction>
#include <QToolTip>
#include <QTcpServer>
#include <QFileDialog>
#include <QStyledItemDelegate>
#include <filesystem>

#include <boost/algorithm/string.hpp>

MainWin* MainWin::m_instance = nullptr;

MainWin::MainWin(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWin)
    , m_settings(new Settings(this))
    , m_model(new Model(m_settings, this))
    , m_XPX_MOSAIC_ID(0)
    , mpWorker(new Worker)
    , mpThread(new QThread)
    , mpCustomCoutStream(std::make_shared<CustomLogsRedirector>(std::cout))
    , mpCustomCerrorStream(std::make_shared<CustomLogsRedirector>(std::cerr))
    , mpCustomClogStream(std::make_shared<CustomLogsRedirector>(std::clog))
{
    ui->setupUi(this);
    m_instance = this;
}

void MainWin::displayError( const std::string& text, const std::string& informativeText )
{
    QMessageBox msgBox;
    msgBox.setText( QString::fromStdString( text ) );
    if ( !informativeText.empty() )
    {
        msgBox.setInformativeText( QString::fromStdString(informativeText) );
    }
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void MainWin::init()
{
    if ( ! m_model->loadSettings() )
    {
        initGeometry();
        std::error_code ec;
        if ( fs::exists( "/Users/alex/Proj/cpp-xpx-storage-user-app", ec ) )
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

    initWorker();

    m_model->initStorageEngine();
    connect(gStorageEngine.get(), &StorageEngine::newError, this, &MainWin::onErrorsHandler, Qt::QueuedConnection);

    setupIcons();
    setupNotifications();
    setGeometry(m_model->getWindowGeometry());

    const std::string privateKey = m_model->getClientPrivateKey();
    qDebug() << "MainWin::init. Private key: " << privateKey;

    m_onChainClient = new OnChainClient(gStorageEngine.get(), this);

    // Should be called before m_onChainClient->start(...)
    connect(m_onChainClient, &OnChainClient::newError, this, &MainWin::onErrorsHandler, Qt::QueuedConnection);

    m_onChainClient->start(privateKey, m_model->getGatewayIp(), m_model->getGatewayPort(), m_model->getFeeMultiplier());
    m_model->startStorageEngine();

    m_modificationsWatcher = new QFileSystemWatcher(this);

    connect( ui->m_settingsButton, &QPushButton::released, this, &MainWin::showSettingsDialog, Qt::QueuedConnection);
    connect(m_modificationsWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWin::onDriveLocalDirectoryChanged, Qt::QueuedConnection);
    connect(ui->m_addChannel, &QPushButton::released, this, [this] () {
        AddDownloadChannelDialog dialog(m_onChainClient, m_model, this);
        connect(m_onChainClient->getDialogSignalsEmitter(), &DialogSignals::addDownloadChannel, this, &MainWin::addChannel);
        dialog.exec();
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::downloadChannelsLoaded, this,
            [this](OnChainClient::ChannelsType type, const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& channelsPages)
    {
        qDebug() << LOG_SOURCE << "downloadChannelsLoaded pages amount: " << channelsPages.size();

        ui->m_channels->clear();
        if (type == OnChainClient::ChannelsType::MyOwn) {
            // load sponsored channels
            m_onChainClient->loadDownloadChannels({});
            m_model->onMyOwnChannelsLoaded(channelsPages);
        } else if (type == OnChainClient::ChannelsType::Sponsored) {
            m_model->onSponsoredChannelsLoaded(channelsPages);

            for (const auto& [key, drive] : m_model->getDrives()) {
                m_model->applyFsTreeForChannels(key, drive.getFsTree(), drive.getRootHash());
            }

            for (const auto& [key, channel] : m_model->getDownloadChannels())
            {
                addEntityToUi(ui->m_channels, channel.getName(), key);
            }

            m_model->setDownloadChannelsLoaded(true);

            if (m_model->getDownloadChannels().empty()) {
                lockChannel("");
            } else {
                unlockChannel("");
            }

            for( auto& downloadInfo: m_model->downloads() )
            {
                if ( ! downloadInfo.isCompleted() )
                {
                    if ( m_model->findChannel( downloadInfo.getDownloadChannelKey() ) == nullptr )
                    {
                        downloadInfo.setChannelOutdated(true);
                    }
                }
            }

            auto channel = m_model->currentDownloadChannel();
            if (channel) {
                m_channelFsTreeTableModel->setFsTree( channel->getFsTree(), channel->getLastOpenedPath() );
                ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));
            } else {
                if (m_model->getDownloadChannels().empty()) {
                    m_channelFsTreeTableModel->setFsTree( {}, {} );
                } else {
                    m_model->setCurrentDownloadChannelKey(m_model->getDownloadChannels().begin()->first);
                    m_channelFsTreeTableModel->setFsTree(m_model->getDownloadChannels().begin()->second.getFsTree(), m_model->getDownloadChannels().begin()->second.getLastOpenedPath() );
                    ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));
                }
            }

            connect( ui->m_channels, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWin::onCurrentChannelChanged, Qt::QueuedConnection);
        }
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::drivesLoaded, this, [this](const std::vector<xpx_chain_sdk::drives_page::DrivesPage>& drivesPages)
    {
        qDebug() << LOG_SOURCE << "drivesLoaded pages amount: " << drivesPages.size();

        connect(this, &MainWin::driveStateChangedSignal, this, [this, drivesPages] (const std::string& driveKey, int state) {
            if (drivesPages.empty() || drivesPages[0].pagination.totalEntries == 0)
            {
                // no drives in Rest response
                drivesInitialized();
            }
            else if (state == no_modifications)
            {
                m_model->setLoadedDrivesCount(m_model->getLoadedDrivesCount() + 1);
                if (m_model->getLoadedDrivesCount() == drivesPages[0].pagination.totalEntries)
                {
                    // Gather drives with old fsTree
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
                            auto fsTreeSaveFolder = getFsTreesFolder() / rootHashUpperCase;
                            bool isFsTreeExists = fs::exists( (fsTreeSaveFolder / FS_TREE_FILE_NAME).make_preferred() );

                            const auto remoteRootHash = rawHashFromHex(remoteDrive.data.rootHash.c_str());
                            if (!isFsTreeExists && drive->getRootHash() == remoteRootHash && Model::isZeroHash(remoteRootHash)) {
                                drive->setFsTree({});
                            }
                            else if (drive->getRootHash() != remoteRootHash || !isFsTreeExists) {
                                drive->setRootHash(remoteRootHash);
                                outdatedDrives.push_back(drive);
                            } else if (isFsTreeExists && drive->getRootHash() == remoteRootHash) {
                                sirius::drive::FsTree fsTree;
                                fsTree.deserialize((fsTreeSaveFolder / FS_TREE_FILE_NAME).make_preferred());
                                drive->setFsTree(fsTree);
                            }
                        }
                    }

                    if (outdatedDrives.empty())
                    {
                        drivesInitialized();
                    }
                    else
                    {
                        connect(m_onChainClient->getStorageEngine(), &StorageEngine::fsTreeReceived, this,
                        [this, counter = (int)outdatedDrives.size()] (auto driveKey, auto fsTreeHash, auto fsTree)
                        {
                            m_model->setOutdatedDriveNumber(m_model->getOutdatedDriveNumber() + 1);
                            onFsTreeReceived(driveKey, fsTreeHash, fsTree);
                            if (counter == m_model->getOutdatedDriveNumber()) {
                                disconnect(m_onChainClient->getStorageEngine(),&StorageEngine::fsTreeReceived,0,0);
                                m_model->saveSettings();
                                drivesInitialized();
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

        if (drivesPages[0].pagination.totalEntries == 0) {
            m_model->getDrives().clear();
            m_model->saveSettings();
            drivesInitialized();
        }

        connect( m_onChainClient, &OnChainClient::downloadFsTreeDirect, this, &MainWin::onDownloadFsTreeDirect, Qt::QueuedConnection);

        // load my own channels
        xpx_chain_sdk::DownloadChannelsPageOptions options;
        options.consumerKey = m_model->getClientPublicKey();
        m_onChainClient->loadDownloadChannels(options);
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::downloadChannelOpenTransactionConfirmed, this, &MainWin::onChannelCreationConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::downloadChannelOpenTransactionFailed, this, &MainWin::onChannelCreationFailed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::downloadChannelCloseTransactionConfirmed, this, &MainWin::onDownloadChannelCloseConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::downloadChannelCloseTransactionFailed, this, &MainWin::onDownloadChannelCloseFailed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::downloadPaymentTransactionConfirmed, this, &MainWin::onDownloadPaymentConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::downloadPaymentTransactionFailed, this, &MainWin::onDownloadPaymentFailed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::storagePaymentTransactionConfirmed, this, &MainWin::onStoragePaymentConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::storagePaymentTransactionFailed, this, &MainWin::onStoragePaymentFailed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::cancelModificationTransactionConfirmed, this, &MainWin::onCancelModificationTransactionConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::cancelModificationTransactionFailed, this, &MainWin::onCancelModificationTransactionFailed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::replicatorOnBoardingTransactionConfirmed, this, &MainWin::onReplicatorOnBoardingTransactionConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::replicatorOnBoardingTransactionFailed, this, &MainWin::onReplicatorOnBoardingTransactionFailed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::replicatorOffBoardingTransactionConfirmed, this, &MainWin::onReplicatorOffBoardingTransactionConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::replicatorOffBoardingTransactionFailed, this, &MainWin::onReplicatorOffBoardingTransactionFailed, Qt::QueuedConnection);
    connect(ui->m_closeChannel, &QPushButton::released, this, &MainWin::onCloseChannel, Qt::QueuedConnection);
    connect(ui->m_closeDrive, &QPushButton::released, this, &MainWin::onCloseDrive, Qt::QueuedConnection);

    connect(ui->m_addDrive, &QPushButton::released, this, [this] () {
        AddDriveDialog dialog(m_onChainClient, m_model, this);
        dialog.exec();
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::prepareDriveTransactionConfirmed, this, [this](auto driveKey) {onDriveCreationConfirmed( sirius::drive::toString( driveKey ) );
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::prepareDriveTransactionFailed, this, [this](auto driveKey, auto errorText) {
        onDriveCreationFailed( sirius::drive::toString( driveKey ), errorText.toStdString() );
    }, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::closeDriveTransactionConfirmed, this, &MainWin::onDriveCloseConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::closeDriveTransactionFailed, this, &MainWin::onDriveCloseFailed, Qt::QueuedConnection);
    connect( ui->m_applyChangesBtn, &QPushButton::released, this, &MainWin::onApplyChanges, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::networkInitialized, this, &MainWin::networkDataHandler, Qt::QueuedConnection);

    connect(ui->m_onBoardReplicator, &QPushButton::released, this, [this](){
        ReplicatorOnBoardingDialog dialog(m_onChainClient, m_model, this);
        dialog.exec();
    });

    connect(ui->m_offBoardReplicatorBtn, &QPushButton::released, this, [this](){
        ReplicatorOffBoardingDialog dialog(m_onChainClient, m_model, this);
        dialog.exec();
    });

    connect(m_onChainClient, &OnChainClient::dataModificationTransactionConfirmed, this, &MainWin::onDataModificationTransactionConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::dataModificationTransactionFailed, this, &MainWin::onDataModificationTransactionFailed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::dataModificationApprovalTransactionConfirmed, this, &MainWin::onDataModificationApprovalTransactionConfirmed, Qt::QueuedConnection);
    connect(m_onChainClient, &OnChainClient::dataModificationApprovalTransactionFailed, this, &MainWin::onDataModificationApprovalTransactionFailed, Qt::QueuedConnection);
    connect( m_onChainClient, &OnChainClient::deployContractTransactionConfirmed, this,&MainWin::onDeployContractTransactionConfirmed, Qt::QueuedConnection );
    connect( m_onChainClient, &OnChainClient::deployContractTransactionFailed, this,&MainWin::onDeployContractTransactionFailed, Qt::QueuedConnection );
    connect( m_onChainClient, &OnChainClient::deployContractTransactionConfirmed, this,&MainWin::onDeployContractApprovalTransactionConfirmed, Qt::QueuedConnection );
    connect( m_onChainClient, &OnChainClient::deployContractTransactionConfirmed, this,&MainWin::onDeployContractApprovalTransactionConfirmed, Qt::QueuedConnection );
    connect(ui->m_refresh, &QPushButton::released, this, &MainWin::onRefresh, Qt::QueuedConnection);

    connect(m_onChainClient, &OnChainClient::newNotification, this, [this](auto notification) {
        showNotification(notification);
        addNotification(notification);
    }, Qt::QueuedConnection);


    setupDownloadsTab();
    setupDrivesTab();
    setupMyReplicatorTab();

    // Start update-timer for downloads
    m_downloadUpdateTimer = new QTimer(this);
    connect( m_downloadUpdateTimer, &QTimer::timeout, this, [this]
    {
        auto selectedIndexes = ui->m_downloadsTableView->selectionModel()->selectedRows();

        //m_downloadsTableModel->updateProgress();

        if ( ! selectedIndexes.empty() )
        {
            ui->m_downloadsTableView->selectRow( selectedIndexes.begin()->row() );
        }
        ui->m_removeDownloadBtn->setEnabled( ! selectedIndexes.empty() );

        this->m_easyDownloadTableModel->updateProgress( ui->m_easyDownloadTable->selectionModel() );
        
    }, Qt::QueuedConnection);

    m_downloadUpdateTimer->start(500); // 2 times per second

    m_modificationStatusTimer = new QTimer(this);
    connect( m_modificationStatusTimer, &QTimer::timeout, this, [this] {
        auto drive = m_model->currentDrive();
        if (drive && isCurrentDrive(drive)
            && drive->getState() == DriveState::uploading
            && m_onChainClient->transactionsEngine()->isModificationsPresent(rawHashFromHex(drive->getKey().c_str())))
        {
            auto drivePubKey = rawHashFromHex(drive->getKey().c_str());
            auto modificationId = m_onChainClient->transactionsEngine()->getLatestModificationId(drivePubKey);
            for (const auto& replicator : drive->getReplicators()) {
                std::optional<boost::asio::ip::udp::endpoint> replicatorEndpoint = {};
                //
                m_model->requestModificationStatus(rawHashToHex(replicator.array()).toStdString(), drive->getKey(), rawHashToHex(modificationId).toStdString());
            }
        } else {
            if (m_modificationStatusTimer->isActive()) {
                m_modificationStatusTimer->stop();
            }
        }
    }, Qt::QueuedConnection);

    lockMainButtons(true);
    lockDrive();
    lockChannel("");

    auto modifyPanelCallback = [this](){
        cancelModification();
    };

    m_modifyProgressPanel = new ModifyProgressPanel( m_model, 350, 350, this, modifyPanelCallback );
    m_modifyProgressPanel->setVisible(false);

    connect(this, &MainWin::updateUploadedDataAmount, this, [this](uint64_t amount, uint64_t replicatorNumber) {
        m_modifyProgressPanel->updateUploadedDataAmount(amount,replicatorNumber);
    }, Qt::QueuedConnection);

    connect(this, &MainWin::modificationFinishedByReplicators, this, [this]() {
        if (m_modificationStatusTimer->isActive()) {
            m_modificationStatusTimer->stop();
        }

        m_modifyProgressPanel->setApproving();
    }, Qt::QueuedConnection);

    m_startViewingProgressPanel = new ModifyProgressPanel( m_model, 350, 350, this, [this]{ cancelViewingStream(); });
    m_startViewingProgressPanel->setVisible(false);

    m_streamingProgressPanel = new ModifyProgressPanel( m_model, 350, 350, this, [this]{ cancelStreaming(); }, ModifyProgressPanel::create_announcement );
    m_streamingProgressPanel->setVisible(false);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {

        updateViewerProgressPanel( index );
        updateStreamerProgressPanel( index );

        // Downloads tab
        if (index == 0) {
            if (m_model->getDownloadChannels().empty()) {
                lockChannel("");
            } else {
                auto channel = m_model->currentDownloadChannel();
                if (channel) {
                    lockChannel(channel->getKey());
                } else {
                    unlockChannel("");
                }
            }
        }

        // Drives tab
        if (index == 2) {
            if (m_model->getDrives().empty() || !m_model->isDrivesLoaded()) {
                lockDrive();
            } else {
                unlockDrive();
            }
        }

        //auto drive = m_model->currentDrive();
        auto* drive = m_model->findDriveByNameOrPublicKey( ui->m_driveCBox->currentText().toStdString() );
        if (index == 2 && !m_model->getDrives().empty() && drive) {
            updateDriveWidgets(drive->getKey(), drive->getState(),false);
        }
        else if (auto drive = m_model->findDriveByNameOrPublicKey(ui->m_streamDriveCBox->currentText().toStdString());
                 drive && index == 5 && (ui->m_streamingTabView->currentIndex() == 1 || ui->m_streamingTabView->currentIndex() == 2))
        {
            updateDriveWidgets( drive->getKey(), drive->getState(), false );
        }
        else
        {
            m_modifyProgressPanel->setVisible(false);
            m_streamingProgressPanel->setVisible(false);
        }
    }, Qt::QueuedConnection );

    connect(m_settings, &Settings::downloadError, this, [this](auto message){
        addNotification(message);
        showNotification(message);
    }, Qt::QueuedConnection);

    if (m_settings->m_isDriveStructureAsTree) {
        ui->m_driveFsTableView->hide();
        ui->m_diffTableView->hide();
    } else {
        ui->m_driveTreeView->hide();
        ui->m_diffTreeView->hide();
    }

    m_model->setModificationStatusResponseHandler(
            [this](auto replicatorKey, auto modificationHash,auto msg, auto currentTask, bool isQueued, bool isFinished, auto error){
        dataModificationsStatusHandler(replicatorKey, modificationHash, msg, currentTask, isQueued, isFinished, error);
            } );

    ui->m_contractMosaicTable->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Stretch );
    ui->m_contractCallMosaicTable->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Stretch );

    connect( &m_model->driveContractModel(), &DriveContractModel::driveContractAdded, this,
             [this]( const std::string& driveKey ) {
                 const auto& drives = m_model->getDrives();
                 auto driveIt = drives.find( driveKey );
                 if ( driveIt == drives.end()) {
                     qWarning() << LOG_SOURCE << "added unknown contract drive " << driveKey;
                     return;
                 }
                 ui->m_contractDriveCBox->addItem(
                         QString::fromStdString( driveIt->second.getName()), QString::fromStdString( driveKey ));
             }, Qt::QueuedConnection );

    connect( ui->m_contractDriveCBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, [this]( int index ) {
        auto* pContractDrive = contractDeploymentData();
        if ( !pContractDrive ) {
            ui->m_deploymentParameters->hide();
            return;
        }
        ui->m_contractAssignee->setText( QString::fromStdString( pContractDrive->m_assignee ));
        ui->m_contractFile->setText( QString::fromStdString( pContractDrive->m_file ));
        ui->m_contractFunction->setText( QString::fromStdString( pContractDrive->m_function ));
        ui->m_contractParameters->setText( QString::fromStdString( pContractDrive->m_parameters ));
        ui->m_contractExecutionCallPayment->setValue( pContractDrive->m_executionCallPayment );
        ui->m_contractDownloadCallPayment->setValue( pContractDrive->m_downloadCallPayment );
        ui->m_contractAutomaticExecutionsNumber->setValue( pContractDrive->m_automaticExecutionsNumber );
        ui->m_contractAutomaticExecutionFileName->setText(
                QString::fromStdString( pContractDrive->m_automaticExecutionFileName ));
        ui->m_contractAutomaticExecutionFunctionName->setText(
                QString::fromStdString( pContractDrive->m_automaticExecutionFunctionName ));
        ui->m_contractAutomaticExecutionCallPayment->setValue( pContractDrive->m_automaticExecutionCallPayment );
        ui->m_contractAutomaticDownloadCallPayment->setValue( pContractDrive->m_automaticDownloadCallPayment );

        ui->m_contractMosaicTable->setRowCount( 0 );

        for ( const auto&[mosaicId, amount]: pContractDrive->m_servicePayments ) {
            ui->m_contractMosaicTable->blockSignals( true );

            int row = ui->m_contractMosaicTable->rowCount();
            ui->m_contractMosaicTable->insertRow( row );

            auto* mosaicItem = new QTableWidgetItem;
            mosaicItem->setText( QString::fromStdString( mosaicId ));
            ui->m_contractMosaicTable->setItem( row, 0, mosaicItem );

            auto* amountItem = new QTableWidgetItem;
            amountItem->setText( QString::fromStdString( amount ));
            ui->m_contractMosaicTable->setItem( row, 1, amountItem );

            ui->m_contractMosaicTable->blockSignals( false );
        }

        ui->m_deploymentParameters->show();

        ui->m_contractDeployBtn->setDisabled( !pContractDrive->isValid());

    }, Qt::QueuedConnection );

    connect( ui->m_contractAssignee, &QLineEdit::textChanged, this, [this]( const auto& text ) {
        bool valid = true;
        if ( auto* pContractDrive = contractDeploymentData(); pContractDrive ) {
            pContractDrive->m_assignee = text.toStdString();
            valid = pContractDrive->isValid();
        } else {
            valid = false;
        }
        ui->m_contractDeployBtn->setDisabled( !valid );
    } );

    connect( ui->m_contractFile, &QLineEdit::textChanged, this, [this]( const auto& text ) {
        bool valid = true;
        if ( auto* pContractDrive = contractDeploymentData(); pContractDrive ) {
            pContractDrive->m_file = text.toStdString();
            valid = pContractDrive->isValid();
        } else {
            valid = false;
        }
        ui->m_contractDeployBtn->setDisabled( !valid );
    } );

    connect( ui->m_contractFunction, &QLineEdit::textChanged, this, [this]( const auto& text ) {
        bool valid = true;
        if ( auto* pContractDrive = contractDeploymentData(); pContractDrive ) {
            pContractDrive->m_function = text.toStdString();
            valid = pContractDrive->isValid();
        } else {
            valid = false;
        }
        ui->m_contractDeployBtn->setDisabled( !valid );
    } );

    connect( ui->m_contractParameters, &QLineEdit::textChanged, this, [this]( const auto& text ) {
        bool valid = true;
        if ( auto* pContractDrive = contractDeploymentData(); pContractDrive ) {
            pContractDrive->m_parameters = text.toStdString();
            valid = pContractDrive->isValid();
        } else {
            valid = false;
        }
        ui->m_contractDeployBtn->setDisabled( !valid );
    } );

    connect( ui->m_contractExecutionCallPayment, &QSpinBox::valueChanged, this, [this]( int value ) {
        auto* pContractDrive = contractDeploymentData();
        if ( pContractDrive ) {
            pContractDrive->m_executionCallPayment = value;
        }
    } );

    connect( ui->m_contractDownloadCallPayment, &QSpinBox::valueChanged, this, [this]( int value ) {
        auto* pContractDrive = contractDeploymentData();
        if ( pContractDrive ) {
            pContractDrive->m_downloadCallPayment = value;
        }
    } );

    connect( ui->m_contractExecutionCallPayment, &QSpinBox::valueChanged, this, [this]( int value ) {
        auto* pContractDrive = contractDeploymentData();
        if ( pContractDrive ) {
            pContractDrive->m_executionCallPayment = value;
        }
    } );

    connect( ui->m_contractAutomaticExecutionFileName, &QLineEdit::textChanged, this, [this]( const auto& text ) {
        bool valid = true;
        if ( auto* pContractDrive = contractDeploymentData(); pContractDrive ) {
            pContractDrive->m_automaticExecutionFileName = text.toStdString();
            valid = pContractDrive->isValid();
        } else {
            valid = false;
        }
        ui->m_contractDeployBtn->setDisabled( !valid );
    } );

    connect( ui->m_contractAutomaticExecutionFunctionName, &QLineEdit::textChanged, this, [this]( const auto& text ) {
        bool valid = true;
        if ( auto* pContractDrive = contractDeploymentData(); pContractDrive ) {
            pContractDrive->m_automaticExecutionFunctionName = text.toStdString();
            valid = pContractDrive->isValid();
        } else {
            valid = false;
        }
        ui->m_contractDeployBtn->setDisabled( !valid );
    } );

    connect( ui->m_contractAutomaticExecutionCallPayment, &QSpinBox::valueChanged, this, [this]( int value ) {
        auto* pContractDrive = contractDeploymentData();
        if ( pContractDrive ) {
            pContractDrive->m_automaticExecutionCallPayment = value;
        }
    } );

    connect( ui->m_contractAutomaticDownloadCallPayment, &QSpinBox::valueChanged, this, [this]( int value ) {
        auto* pContractDrive = contractDeploymentData();
        if ( pContractDrive ) {
            pContractDrive->m_automaticDownloadCallPayment = value;
        }
    } );

    connect( ui->m_contractAutomaticExecutionsNumber, &QSpinBox::valueChanged, this, [this]( int value ) {
        auto* pContractDrive = contractDeploymentData();
        if ( pContractDrive ) {
            pContractDrive->m_automaticExecutionsNumber = value;
        }
    } );

    connect( ui->m_addMosaic, &QPushButton::released, this, [this] {
        ui->m_contractMosaicTable->insertRow( ui->m_contractMosaicTable->rowCount());
        auto* pContractDrive = contractDeploymentData();
        if ( pContractDrive ) {
            pContractDrive->m_servicePayments.emplace_back();
        }
        ui->m_contractDeployBtn->setDisabled( !pContractDrive || !pContractDrive->isValid());
    } );

    connect( ui->m_removeMosaic, &QPushButton::released, this, [this] {
        int selectedRow = ui->m_contractMosaicTable->currentRow();
        ui->m_contractMosaicTable->setCurrentIndex( QModelIndex());
        ui->m_contractMosaicTable->removeRow( selectedRow );
        auto* pContractDrive = contractDeploymentData();
        if ( pContractDrive ) {
            pContractDrive->m_servicePayments.erase( pContractDrive->m_servicePayments.begin() + selectedRow );
        }

        ui->m_contractDeployBtn->setDisabled( !pContractDrive || !pContractDrive->isValid());
    } );

    connect( ui->m_contractMosaicTable->selectionModel(),
             &QItemSelectionModel::selectionChanged,
             this,
             [this]( const QItemSelection& selected, const QItemSelection& deselected ) {
                 ui->m_removeMosaic->setDisabled( selected.indexes().empty());
    });

    QObject::connect( ui->m_contractMosaicTable, &QTableWidget::itemChanged, this,
                      [this]( QTableWidgetItem* item ) {
                          auto* pContractDrive = contractDeploymentData();
                          if ( pContractDrive ) {
                              if ( item->column() == 0 ) {
                                  pContractDrive->m_servicePayments.at(
                                          item->row()).m_mosaicId = item->text().toStdString();
                              } else {
                                  pContractDrive->m_servicePayments.at(
                                          item->row()).m_amount = item->text().toStdString();
                              }
                          }
                          ui->m_contractDeployBtn->setDisabled( !pContractDrive || !pContractDrive->isValid());
                      } );

    QObject::connect( ui->m_contractCallKey, &QLineEdit::textChanged, this, [this]( const auto& text ) {
        m_model->driveContractModel().getContractManualCallData().m_contractKey = text.toStdString();
        ui->m_contractCallRun->setDisabled(!m_model->driveContractModel().getContractManualCallData().isValid());
    } );

    QObject::connect( ui->m_contractCallFile, &QLineEdit::textChanged, this, [this]( const auto& text ) {
        m_model->driveContractModel().getContractManualCallData().m_file = text.toStdString();
        ui->m_contractCallRun->setDisabled(!m_model->driveContractModel().getContractManualCallData().isValid());
    } );

    QObject::connect( ui->m_contractCallFunction, &QLineEdit::textChanged, this, [this]( const auto& text ) {
        m_model->driveContractModel().getContractManualCallData().m_function = text.toStdString();
        ui->m_contractCallRun->setDisabled(!m_model->driveContractModel().getContractManualCallData().isValid());
    } );

    QObject::connect( ui->m_contractCallParameters, &QLineEdit::textChanged, this, [this]( const auto& text ) {
        m_model->driveContractModel().getContractManualCallData().m_parameters = text.toStdString();
        ui->m_contractCallRun->setDisabled(!m_model->driveContractModel().getContractManualCallData().isValid());
    } );

    connect( ui->m_contractCallExecutionPayment, &QSpinBox::valueChanged, this, [this]( int value ) {
        m_model->driveContractModel().getContractManualCallData().m_executionCallPayment = value;
        ui->m_contractCallRun->setDisabled(!m_model->driveContractModel().getContractManualCallData().isValid());
    } );

    connect( ui->m_contractCallDownloadPayment, &QSpinBox::valueChanged, this, [this]( int value ) {
        m_model->driveContractModel().getContractManualCallData().m_downloadCallPayment = value;
        ui->m_contractCallRun->setDisabled(!m_model->driveContractModel().getContractManualCallData().isValid());
    } );

    connect( ui->m_contractCallAddMosaic, &QPushButton::released, this, [this] {
        ui->m_contractCallMosaicTable->insertRow( ui->m_contractMosaicTable->rowCount());
        m_model->driveContractModel().getContractManualCallData().m_servicePayments.emplace_back();
        ui->m_contractCallRun->setDisabled(!m_model->driveContractModel().getContractManualCallData().isValid());
    } );

    connect( ui->m_contractCallRemoveMosaic, &QPushButton::released, this, [this] {
        int selectedRow = ui->m_contractCallMosaicTable->currentRow();
        if (selectedRow < 0) {
            return;
        }

        ui->m_contractCallMosaicTable->setCurrentIndex( QModelIndex() );
        ui->m_contractCallMosaicTable->removeRow( selectedRow );
        auto& payments = m_model->driveContractModel().getContractManualCallData().m_servicePayments;
        payments.erase(payments.begin() + selectedRow);
        ui->m_contractCallRun->setDisabled(!m_model->driveContractModel().getContractManualCallData().isValid());
    } );

    connect( ui->m_contractCallMosaicTable->selectionModel(),
             &QItemSelectionModel::selectionChanged,
             this,
             [this]( const QItemSelection& selected, const QItemSelection& deselected ) {
                 ui->m_contractCallRemoveMosaic->setDisabled( selected.indexes().empty() );
             } );

    QObject::connect( ui->m_contractCallMosaicTable, &QTableWidget::itemChanged, this,
                      [this]( QTableWidgetItem* item ) {
                          auto& payments = m_model->driveContractModel().getContractManualCallData().m_servicePayments;
                          if ( item->column() == 0 ) {
                              payments.at(
                                      item->row()).m_mosaicId = item->text().toStdString();
                          } else {
                              payments.at(
                                      item->row()).m_amount = item->text().toStdString();
                          }
                          ui->m_contractCallRun->setDisabled(!m_model->driveContractModel().getContractManualCallData().isValid());
                      } );

    connect( ui->m_contractDeployBtn, &QPushButton::released, this, &MainWin::onDeployContract, Qt::QueuedConnection );

    connect( ui->m_contractCallRun, &QPushButton::released, this, &MainWin::onRunContract, Qt::QueuedConnection );

    QSizePolicy sp_retain = ui->m_deploymentParameters->sizePolicy();
    sp_retain.setRetainSizeWhenHidden( true );
    ui->m_deploymentParameters->setSizePolicy( sp_retain );


    emit ui->m_contractDriveCBox->currentIndexChanged(-1);
    ui->m_contractCallRun->setDisabled(true);
//    connect( &m_model->driveContractModel(), &DriveContractModel::driveContractRemoved, this,
//             [this]( const std::string& driveKey )
//             {
//        ui->m_contractDriveCBox->addItem( QString::fromStdString( driveKey ));
//             } );

    // TODO temporary hide
    // Replicators tab: contributed, used
    ui->label_2->hide();
    ui->label_7->hide();

    initStreaming();
    initWizardStreaming();
    initWizardArchiveStreaming();

    std::error_code ec;
    // if ( ! fs::exists( "/Users/alex/Proj/cpp-xpx-storage-user-app", ec ) )
    // {
    //     ui->tabWidget->setTabVisible( 4, false );
    // }

    // Hide contracts
    ui->tabWidget->setTabVisible( 4, false );

    // Hide streaming
    ui->tabWidget->setTabVisible( 5, true );

    doUpdateBalancePeriodically();
}

void MainWin::initGeometry()
{
    auto basicGeometry = QGuiApplication::primaryScreen()->geometry();
    QRect rect = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, { 1000, 800 }, basicGeometry);
    m_model->setWindowGeometry(rect);
}

void MainWin::drivesInitialized()
{
    qInfo() << "MainWin::drivesInitialized: " << m_model->getDrives().size();
    disconnect(this,&MainWin::driveStateChangedSignal, nullptr, nullptr);

    ui->m_driveCBox->clear();
    ui->m_streamDriveCBox->clear();
    for (const auto& [key, drive] : m_model->getDrives()) {
        addEntityToUi(ui->m_driveCBox, drive.getName(), drive.getKey());
        addEntityToUi(ui->m_streamDriveCBox, drive.getName(), drive.getKey());
        if (boost::iequals(key, m_model->currentDriveKey()) ) {
            ui->m_drivePath->setText( "Path: /" );
            updateDriveWidgets(drive.getKey(), drive.getState(), false);
        }
    }

    if (!m_model->getDrives().empty() && (!m_model->getDrives().contains(m_model->currentDriveKey()) || m_model->currentDriveKey().empty())) {
        const auto driveKey = ui->m_driveCBox->currentData().toString().toStdString();
        m_model->setCurrentDriveKey(driveKey);
        setCurrentDriveOnUi(driveKey);
        const auto drive = m_model->getDrives()[driveKey];
        ui->m_drivePath->setText( "Path: /" );
        updateDriveWidgets(drive.getKey(), drive.getState(), false);
    } else if (!m_model->getDrives().empty()) {
        setCurrentDriveOnUi(m_model->currentDriveKey());
    }

    addLocalModificationsWatcher();

    connect(this, &MainWin::driveStateChangedSignal, this, [this](const std::string& driveKey, int state, bool itIsNewState )
    {
        m_settings->accountConfig().m_driveContractModel.onDriveStateChanged( driveKey, state );
        MainWin::updateDriveWidgets( driveKey, state, itIsNewState);
    }, Qt::QueuedConnection);

    connect(ui->m_driveCBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWin::onCurrentDriveChanged, Qt::QueuedConnection);
    connect(m_onChainClient->getStorageEngine(), &StorageEngine::fsTreeReceived, this, &MainWin::onFsTreeReceived, Qt::QueuedConnection);

    lockMainButtons(false);
    unlockDrive();
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
    if ( dialog.exec() == QMessageBox::Ok )
    {
        drive->updateDriveState(canceling);
    }
}

void MainWin::doUpdateBalancePeriodically()
{
    connect(m_onChainClient, &OnChainClient::updateBalance, this, [this](){
        loadBalance();
    });
}

void MainWin::setupIcons() {
    QIcon settingsButtonIcon(getResource("./resources/icons/settings.png"));
    if (settingsButtonIcon.isNull())
    {
        qWarning () << "MainWin::setupIcons: settings icon is null: settings.png";
    } else {
        ui->m_settingsButton->setFixedSize(settingsButtonIcon.availableSizes().first());
        ui->m_settingsButton->setIcon(settingsButtonIcon);
    }

    ui->m_settingsButton->setText("");
    ui->m_settingsButton->setStyleSheet("background: transparent; border: 0px;");
    ui->m_settingsButton->setIconSize(QSize(18, 18));

    QIcon notificationsButtonIcon(getResource("./resources/icons/notification.png"));
    if (notificationsButtonIcon.isNull())
    {
        qWarning () << "MainWin::setupIcons: notifications icon is null: notification.png";
    } else {
        ui->m_notificationsButton->setFixedSize(notificationsButtonIcon.availableSizes().first());
        ui->m_notificationsButton->setIcon(notificationsButtonIcon);
    }

    ui->m_notificationsButton->setText("");
    ui->m_notificationsButton->setStyleSheet("background: transparent; border: 0px;");
    ui->m_notificationsButton->setIconSize(QSize(18, 18));

    QPixmap networkIcon(getResource("./resources/icons/network.png"));
    if (settingsButtonIcon.isNull())
    {
        qWarning () << "MainWin::setupIcons: network icon is null: network.png";
    } else {
        ui->m_networkLabel->setPixmap(networkIcon);
    }

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
    setupEasyDownloads();
    setupChannelFsTable();
    setupDownloadsTable();

    ui->m_downloadsPath->setText(static_cast<QLatin1StringView>("Path: " + m_settings->accountConfig().m_downloadFolder));
    ui->m_channels->addItem( "Loading..." );

    connect( ui->m_downloadBtn, &QPushButton::released, this, &MainWin::onDownloadBtn, Qt::QueuedConnection);

    auto menu = new PopupMenu(ui->m_moreChannelsBtn, this);
    auto renameAction = new QAction("Rename", this);
    menu->addAction(renameAction);
    connect( renameAction, &QAction::triggered, this, [this](bool)
    {
        if ( auto* channelInfo = m_model->currentDownloadChannel(); channelInfo != nullptr )
        {
            std::string channelName = channelInfo->getName();
            std::string channelKey = channelInfo->getKey();

            EditDialog dialog( "Rename channel", "Channel name:", channelName, EditDialog::DownloadChannel, m_model );
            if ( dialog.exec() == QDialog::Accepted )
            {
                qDebug() << "channelName: " << channelName.c_str();
                if ( auto* channelInfo = m_model->findChannel(channelKey); channelInfo != nullptr )
                {
                    qDebug() << "channelName renamed: " << channelName.c_str();

                    channelInfo->setName(channelName);
                    m_model->saveSettings();
                    updateEntityNameOnUi(ui->m_channels, channelName, channelInfo->getKey());
                }
            }
            qDebug() << "channelName?: " << channelName;
        }
    });

    auto topUpAction = new QAction("Top-up", this);
    menu->addAction(topUpAction);
    connect( topUpAction, &QAction::triggered, this, [this](bool)
    {
        DownloadPaymentDialog dialog(m_onChainClient, m_model, this);
        dialog.exec();
    });

    auto copyChannelKeyAction = new QAction("Copy channel key", this);
    menu->addAction(copyChannelKeyAction);
    connect( copyChannelKeyAction, &QAction::triggered, this, [this](bool)
    {
        if ( auto* channelInfo = m_model->currentDownloadChannel(); channelInfo != nullptr )
        {
            std::string channelKey = channelInfo->getKey();

            QClipboard* clipboard = QApplication::clipboard();
            if ( !clipboard ) {
                qWarning() << LOG_SOURCE << "bad clipboard";
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
        auto channel = m_model->currentDownloadChannel();
        if (!channel) {
            qWarning() << "Channel info action. Unknown download channel (Invalid pointer to channel)";
            return;
        }

        m_onChainClient->getBlockchainEngine()->getDownloadChannelById(channel->getKey(),[this, channel]
            (auto remoteChannel, auto isSuccess, auto message, auto code ){
            if (!isSuccess) {
                qWarning() << "Channel info action. Error: " << message.c_str() << " : " << code.c_str();
                return;
            }

            ChannelInfoDialog dialog(this);
            dialog.setName(channel->getName().c_str());
            dialog.setId(channel->getKey().c_str());
            dialog.setDriveId(channel->getDriveKey().c_str());
            dialog.setReplicators(remoteChannel.data.shardReplicators);
            dialog.setKeys(remoteChannel.data.listOfPublicKeys);
            dialog.setPrepaid(remoteChannel.data.downloadSizeMegabytes);
            dialog.init();
            dialog.exec();
        });
    });

    ui->m_moreChannelsBtn->setMenu(menu);

    connect( ui->m_openDownloadFolderBtn, &QPushButton::released, this, [this]
    {
        QDesktopServices::openUrl( QUrl::fromLocalFile( QString::fromStdString( m_settings->downloadFolder().string() )));
    });

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

    m_channelFsTreeTableModel = new FsTreeTableModel( m_model, true, this );

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
}

void MainWin::setupDriveFsTable()
{
    connect( ui->m_driveFsTableView, &QTableView::doubleClicked, this, [this] (const QModelIndex &index)
    {
        if (!index.isValid()) {
            return;
        }
        
        if ( index.row()==0 && m_driveTableModel->currentPath().size()==1 )
        {
            return;
        }

        int toBeSelectedRow = m_driveTableModel->onDoubleClick( index.row() );
        ui->m_driveFsTableView->selectRow( toBeSelectedRow );
        if ( auto* drivePtr = m_model->currentDrive(); drivePtr != nullptr )
        {
            drivePtr->setLastOpenedPath(m_driveTableModel->currentPath());
            std::string path = "";
            auto pathVector = m_driveTableModel->currentPath();
            if ( pathVector.size() <= 1 )
            {
                path = "/";
            }
            else
            {
                for( size_t i=1;  i<pathVector.size(); i++ )
                {
                    path = path + "/" + pathVector[i];
                }
            }
            ui->m_drivePath->setText( "Path: " + QString::fromStdString(path) );

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

    m_driveTableModel = new FsTreeTableModel(m_model, false, this );

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
    auto channel = m_model->currentDownloadChannel();
    if (!channel) {
        qWarning () << "MainWin::onDownloadBtn. Download channel not found! (Invalid pointer)";
        return;
    }

    if (m_channelFsTreeTableModel->getSelectedRows().empty()) {
        qInfo() << "MainWin::onDownload. Select at least one file (folder)";
        onErrorsHandler(ErrorType::InvalidData, "Select at least one file (folder)!");
        return;
    }

    auto updateReplicatorsCallback = [this, channel]()
    {
        for (const auto& selectedRow : m_channelFsTreeTableModel->getSelectedRows(false))
        {
            m_downloadsTableModel->addRow(selectedRow);
        }

        for (const auto& selectedRow : m_channelFsTreeTableModel->getSelectedRows())
        {
            QDir().mkpath(QString::fromStdString(m_model->getDownloadFolder().string() + selectedRow.m_path));

            auto ltHandle = m_model->downloadFile( channel->getKey(),  selectedRow.m_hash );

            qDebug() << "childIt->m_path(2): " << selectedRow.m_path.c_str() << " : " << selectedRow.m_name.c_str() << " : " << m_model->getDownloadFolder();

            DownloadInfo downloadInfo;
            downloadInfo.setHash(selectedRow.m_hash);
            downloadInfo.setDownloadChannelKey(channel->getKey());
            downloadInfo.setFileName(selectedRow.m_name);
            downloadInfo.setSaveFolder(selectedRow.m_path);
            downloadInfo.setDownloadFolder(m_model->getDownloadFolder().string());
            downloadInfo.setChannelOutdated(false);
            downloadInfo.setCompleted(false);
            downloadInfo.setProgress(0);
            downloadInfo.setHandle(ltHandle);

            m_model->downloads().insert( m_model->downloads().begin(), downloadInfo );
            m_model->saveSettings();
        }
    };

    updateReplicatorsForChannel(channel->getKey(), updateReplicatorsCallback);
}

void MainWin::setupEasyDownloads()
{
    m_easyDownloadTableModel = new EasyDownloadTableModel(m_model, this);
    ui->m_easyDownloadTable->setModel( m_easyDownloadTableModel );

    ui->m_easyDownloadTable->setColumnWidth(0,300);
    ui->m_easyDownloadTable->horizontalHeader()->setStretchLastSection(true);
    ui->m_easyDownloadTable->horizontalHeader()->hide();
    ui->m_easyDownloadTable->setGridStyle( Qt::NoPen );
    ui->m_easyDownloadTable->update();
    
    connect( ui->m_downloadDataByLinkBtn, &QPushButton::released, this, [this]
    {
       PasteLinkDialog dialog(this, m_model);
       dialog.exec();

//#ifdef __APPLE__
//        std::array<uint8_t, 32> driveKeyHash = Model::hexStringToHash("84E68E6D280370993650E1DEAB7597FA91E943E74B18682234D18C29224DFE81");;
//        std::string             path = "/";
//        uint64_t                totalSize = 33554477;
//
//        DataInfo testData( "todo", driveKeyHash, path, totalSize );
//
//        startEasyDownload( testData );
//#endif
    });
    
    
    connect( ui->m_deleteDownloadedDataBtn, &QPushButton::released, this, [this]
    {
//        auto rows = ui->m_downloadedDataTable->selectionModel()->selectedRows();
//        if ( rows.empty() )
//        {
//            return;
//        }
//
//        int rowIndex = rows.begin()->row();
//        auto& downloads = m_model->downloads();
//
//        if ( rowIndex >= 0 && rowIndex < downloads.size() )
//        {
//            DownloadInfo dnInfo = downloads[rowIndex];
//
//            QMessageBox msgBox;
//            const QString message = QString::fromStdString("Are you sure you want to permanently delete '" + dnInfo.getFileName() + "'?");
//            msgBox.setText(message);
//            msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
//            auto reply = msgBox.exec();
//
//            if ( reply == QMessageBox::Ok )
//            {
//                m_model->removeFromDownloads( rowIndex );
//                addNotification(message);
//                m_model->saveSettings();
//            }
//        }
    }, Qt::QueuedConnection);


}


void MainWin::setupDownloadsTable()
{
    m_downloadsTableModel = new DownloadsTableModel(m_model, this);

    ui->m_downloadsTableView->setModel( m_downloadsTableModel );
//    ui->m_downloadsTableView->horizontalHeader()->setStretchLastSection(true);
//    ui->m_downloadsTableView->horizontalHeader()->hide();
//    ui->m_downloadsTableView->setGridStyle( Qt::NoPen );
//    ui->m_downloadsTableView->setSelectionBehavior( QAbstractItemView::SelectRows );
//    ui->m_downloadsTableView->setSelectionMode( QAbstractItemView::SingleSelection );
//    ui->m_downloadsTableView->horizontalHeader()->setStretchLastSection(false);
//    ui->m_downloadsTableView->setColumnWidth(1, 60);
    //ui->m_downloadsTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    ui->m_downloadsTableView->setColumnWidth(0,300);
    ui->m_downloadsTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_downloadsTableView->horizontalHeader()->hide();
    ui->m_downloadsTableView->setGridStyle( Qt::NoPen );
    ui->m_downloadsTableView->update();
    // TODO: add variable for downloads
    //ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));

    ui->m_removeDownloadBtn->setEnabled( false );

//    connect( ui->m_downloadsTableView, &QTableView::doubleClicked, this, [this] (const QModelIndex &index)
//    {
//        ui->m_downloadsTableView->selectRow( index.row() );
//    });

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
        auto rows = ui->m_downloadsTableView->selectionModel()->selectedRows();
        if ( rows.empty() )
        {
            return;
        }

        int rowIndex = rows.begin()->row();
        auto& downloads = m_model->downloads();

        if ( rowIndex >= 0 && rowIndex < downloads.size() )
        {
            DownloadInfo dnInfo = downloads[rowIndex];

            QMessageBox msgBox;
            const QString message = QString::fromStdString("Are you sure you want to permanently delete '" + dnInfo.getFileName() + "'?");
            msgBox.setText(message);
            msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
            auto reply = msgBox.exec();

            if ( reply == QMessageBox::Ok )
            {
                m_model->removeFromDownloads( rowIndex );
                addNotification(message);
                m_model->saveSettings();
            }
        }
    });

    connect( ui->m_downloadsTableView, &QTableView::doubleClicked, this, [this] (const QModelIndex &index)
    {
        if (!index.isValid()) {
            return;
        }

        int toBeSelectedRow = m_downloadsTableModel->onDoubleClick( index.row() );
        ui->m_downloadsTableView->selectRow( toBeSelectedRow );
        //ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));
//        if ( auto* channelPtr = m_model->currentDownloadChannel(); channelPtr != nullptr )
//        {
//            channelPtr->setLastOpenedPath(m_channelFsTreeTableModel->currentPath());
//            qDebug() << LOG_SOURCE << "m_lastOpenedPath: " << channelPtr->getLastOpenedPath();
//        }
    }, Qt::QueuedConnection);
}

bool MainWin::requestPrivateKey()
{
    {
        PrivKeyDialog pKeyDialog( m_settings, this );
        pKeyDialog.exec();

        if ( pKeyDialog.result() != QDialog::Accepted )
        {
            qDebug() << "MainWin::requestPrivateKey. Not accepted!";
            return false;
        }

        qDebug() << "MainWin::requestPrivateKey. Accepted!";
    }

    SettingsDialog settingsDialog( m_settings, this, true );
    settingsDialog.exec();

    if ( settingsDialog.result() != QDialog::Accepted )
    {
        return false;
    }

    return true;
}

void MainWin::checkDriveForUpdates(Drive* drive, const std::function<void(bool)>& callback)
{
    if (!drive) {
        qWarning() << "MainWin::checkDriveForUpdates. Invalid pointer to drive";
        callback(false);
        return;
    }

    if (drive->getState() == DriveState::creating || drive->getState() == DriveState::deleting) {
        qInfo() << "MainWin::checkDriveForUpdates. Drive in unacceptable state:" << getPrettyDriveState(drive->getState());
        callback(false);
        return;
    }

    qDebug () << "MainWin::checkDriveForUpdates. Drive key: " << drive->getKey();

    m_onChainClient->getBlockchainEngine()->getDriveById( drive->getKey(),
    [this, drive, callback] (auto remoteDrive, auto isSuccess, auto message, auto code ) {
        bool result = false;
        if (!isSuccess) {
            qWarning() << "MainWin::checkDriveForUpdates:: callback(false): " << message.c_str() << " : " << code.c_str();
            callback(result);
            return;
        }

        drive->setReplicators(remoteDrive.data.replicators);
        m_onChainClient->getStorageEngine()->addReplicators(drive->getReplicators());

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

    if (channel->isCreating()) {
        qInfo() << "MainWin::checkDriveForUpdates. Download channel is creating:" << channel->getKey();
        callback(false);
        return;
    }

    qDebug () << "MainWin::checkDriveForUpdates. Channel key: " << channel->getKey();

    m_onChainClient->getBlockchainEngine()->getDriveById( channel->getDriveKey(),
    [this, channel, callback] (auto remoteDrive, auto isSuccess, auto message, auto code ) {
        bool result = false;
        if (!isSuccess) {
            qWarning() << "MainWin::checkDriveForUpdates:: callback(false): " << message.c_str() << " : " << code.c_str();
            callback(result);
            return;
        }

        qDebug () << "MainWin::checkDriveForUpdates. driveKey: " << channel->getDriveKey();
        qDebug () << "MainWin::checkDriveForUpdates. remoteRootHash: " << remoteDrive.data.rootHash;

        const auto remoteRootHash = rawHashFromHex(remoteDrive.data.rootHash.c_str());
        auto drive = m_model->findDrive(channel->getDriveKey());
        if (drive) {
            drive->setReplicators(remoteDrive.data.replicators);
            m_onChainClient->getStorageEngine()->addReplicators(drive->getReplicators());

            if (remoteRootHash == drive->getRootHash() && remoteRootHash == channel->getFsTreeHash()) {
                m_model->applyFsTreeForChannels(drive->getKey(), drive->getFsTree(), remoteRootHash);
            } else if (remoteRootHash == drive->getRootHash() && remoteRootHash != channel->getFsTreeHash()) {
                channel->setFsTreeHash(remoteRootHash);
                m_model->applyFsTreeForChannels(drive->getKey(), drive->getFsTree(), remoteRootHash);
            } else if (remoteRootHash != drive->getRootHash() && remoteRootHash == channel->getFsTreeHash()) {
                result = true;
            } else if (remoteRootHash != drive->getRootHash() && remoteRootHash != channel->getFsTreeHash()) {
                channel->setFsTreeHash(remoteRootHash);
                result = true;
            } else {
                qWarning () << "MainWin::checkDriveForUpdates. Unresolved case for update!";
            }
        } else {
            if (remoteRootHash != channel->getFsTreeHash()) {
                channel->setFsTreeHash(remoteRootHash);
                result = true;
            }
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

    if (channel->isCreating()) {
        qInfo() << "MainWin::updateReplicatorsForChannel. Download channel is creating:" << channel->getKey();
        callback();
        return;
    }

    qDebug () << "MainWin::updateReplicatorsForChannel. Channel key: " << channelId;

    m_onChainClient->getBlockchainEngine()->getDownloadChannelById( channelId,
    [this, channel, callback] (auto remoteChannel, auto isSuccess, auto message, auto code ) {
        bool result = false;
        if (!isSuccess) {
            qWarning() << "MainWin::updateReplicatorsForChannel::callback(): " << message.c_str() << " : " << code.c_str();
            callback();
            return;
        }

        channel->setReplicators(remoteChannel.data.shardReplicators);
        m_onChainClient->getStorageEngine()->addReplicators(channel->getReplicators());

        callback();
    });
}

void MainWin::onErrorsHandler(int errorType, const QString& errorText)
{
    switch (errorType)
    {
        case ErrorType::Network:
        {
            showNotification(errorText);
            addNotification(errorText);
            qWarning () << "MainWin::onErrorsHandler. Network error: " << errorText;
            break;
        }

        case ErrorType::NetworkInit:
        {
            showNotification(errorText);
            addNotification(errorText);
            qWarning () << "MainWin::onErrorsHandler. NetworkInit error: " << errorText;
            break;
        }

        case ErrorType::InvalidData:
        {
            showNotification(errorText);
            addNotification(errorText);
            qWarning () << "MainWin::onErrorsHandler. InvalidData error: " << errorText;
            break;
        }

        case ErrorType::Storage:
        {
            const auto finalErrorMessage = errorText + " UDP Port: " + QString::fromStdString(m_model->getUdpPort());
            showNotification(finalErrorMessage);
            addNotification(finalErrorMessage);
            qWarning () << "MainWin::onErrorsHandler. Storage error: " << finalErrorMessage;
//            qApp->quit();
//            QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
            //QTimer::singleShot(3000, this, [](){ QApplication::exit(1024); });
            break;
        }

        case ErrorType::Critical:
        {
            qCritical () << "MainWin::onErrorsHandler. Critical error: " << errorText;
            QApplication::exit(3);
        }
    }
}

void MainWin::setDownloadChannelOnUi(const std::string& channelId)
{
    auto channel = m_model->findChannel(channelId);
    if (!channel) {
        qWarning() << "MainWin::setDownloadChannelOnUi. Unknown channel: " << channelId;
        if (ui->m_channels->count() > 0) {
            ui->m_channels->setCurrentIndex(0);
            m_model->setCurrentDownloadChannelKey( ui->m_channels->currentData().toString().toStdString() );
        }

        return;
    }

    const auto currentChannelKey = ui->m_channels->currentData().toString();
    if ( ! boost::iequals(channelId, currentChannelKey.toStdString())) {
        const int index = ui->m_channels->findData(QVariant::fromValue(QString::fromStdString(channelId)), Qt::UserRole, Qt::MatchFixedString);
        if (index != -1) {
            ui->m_channels->setCurrentIndex(index);
        } else {
            qWarning() << "MainWin::setDownloadChannelOnUi. Channel not found in UI: " << channelId;
        }
    }
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

void MainWin::updateDriveWidgets(const std::string& driveKey, int state, bool itIsNewState)
{
    qInfo() << ">> MainWin::onDriveStateChanged. Drive key: " << driveKey << " state: " << getPrettyDriveState(state);

    auto drive = m_model->findDrive(driveKey);
    if (!drive) {
        qWarning() << "MainWin::onDriveStateChanged. Drive not found!";
        return;
    }

    qInfo() << "MainWin::onDriveStateChanged. Drive key: " << drive->getKey() << " state: " << getPrettyDriveState(state);
    
    // may be it is streaming drive
    Drive* streamingDrive = nullptr;
    if ( boost::iequals( driveKey, ui->m_streamDriveCBox->currentData().toString().toStdString() ) )
    {
        streamingDrive = drive;
        m_streamingProgressPanel->updateStreamingMode(drive);
    }

    bool isPanelVisible =   ( isCurrentDrive(drive) && ui->tabWidget->currentIndex() == 2 )
                            || ( ( streamingDrive != nullptr ) &&
                                   ( ui->tabWidget->currentIndex() == 5 &&
                                   ( ui->m_streamingTabView->currentIndex() == 1 || ui->m_streamingTabView->currentIndex() == 2 )));

    // Update drive progress panels
    switch(state)
    {
        case registering:
        case approved:
        case failed:
        case canceling:
        case canceled:
        case uploading:
        {
            if ( !isPanelVisible )
            {
                m_modifyProgressPanel->setVisible(false);
                m_streamingProgressPanel->setVisible(false);
            }
            else
            {
                if ( !drive->isStreaming() )
                {
                    m_modifyProgressPanel->setVisible(true);
                    m_streamingProgressPanel->setVisible(false);
                    lockDrive();
                }
                else
                {
                    m_modifyProgressPanel->setVisible(false);
                    m_streamingProgressPanel->setVisible( true );
                }
                
                switch(state)
                {
                    case approved:
                    case failed:
                    case canceled:
                        lockDrive();
                    default:
                        unlockDrive();
                }
            }
            break;
        }
        case no_modifications:
        case creating:
        case unconfirmed:
        case deleting:
        case deleted:
        default:
        {
            if ( !isPanelVisible )
            {
                m_modifyProgressPanel->setVisible(false);
                m_streamingProgressPanel->setVisible(false);
            }
            unlockDrive();
        }
    }

    switch(state)
    {
        case no_modifications:
        {
            updateDriveStatusOnUi(*drive);
            drive->resetStreamingStatus();

            if (isCurrentDrive(drive)) {
                calculateDiffAsync([this](auto, auto){
                    updateDiffView();
                });

                updateDriveView();
            }

            if (isCurrentDrive(drive) && ui->tabWidget->currentIndex() == 2)  {
                loadBalance();
            }

            break;
        }
        // registering
        case registering:
        {
            if ( isCurrentDrive(drive) || (streamingDrive != nullptr) )
            {
                if ( !drive->isStreaming() )
                {
                    m_modifyProgressPanel->setRegistering();
                }
                else
                {
                    m_streamingProgressPanel->setRegistering();
                }
            }
            break;
        }
        // approved
        case approved:
        {
            if ( isCurrentDrive(drive) || (streamingDrive != nullptr) )
            {
                if ( isCurrentDrive(drive) || (streamingDrive != nullptr) )
                {
                    if ( !drive->isStreaming() )
                    {
                        m_modifyProgressPanel->setApproved();
                    }
                    else
                    {
                        m_streamingProgressPanel->setApproved();
                        wizardUpdateStreamAnnouncementTable();
                        wizardUpdateArchiveTable();
                    }
                }

                if (m_modificationStatusTimer->isActive()) {
                    m_modificationStatusTimer->stop();
                }

                loadBalance();

                if ( itIsNewState )
                {
                    if ( drive->getStreamingStatus() == Drive::ss_streaming )
                    {
                        QString message;
                        message.append("Your streaming was applied. Drive: ");
                        message.append(drive->getName().c_str());
                        addNotification(message);
                    }
                    else
                    {
                        QString message;
                        message.append("Your modification was applied. Drive: ");
                        message.append(drive->getName().c_str());
                        addNotification(message);
                    }
                }
            }

            break;
        }
        // failed
        case failed:
        {
            if ( isCurrentDrive(drive) || (streamingDrive != nullptr) )
            {
                if ( !drive->isStreaming() )
                {
                    m_modifyProgressPanel->setFailed();
                }
                else
                {
                    m_streamingProgressPanel->setFailed();
                }

                loadBalance();

                if ( itIsNewState )
                {
                    if ( !drive->isStreaming() )
                    {
                        m_modifyProgressPanel->setVisible(true);
                        m_streamingProgressPanel->setVisible(false);

                        const QString message = "Your modification was declined.";
                        addNotification(message);
                    }
                    else
                    {
                        m_modifyProgressPanel->setVisible(false);
                        m_streamingProgressPanel->setVisible(true);

                        const QString message = "Your streaming was declined.";
                        addNotification(message);
                    }
                }

                if (m_modificationStatusTimer->isActive()) {
                    m_modificationStatusTimer->stop();
                }
            }

            break;
        }
        // canceling
        case canceling:
        {
            if ( isCurrentDrive(drive) || (streamingDrive != nullptr) )
            {
                if ( !drive->isStreaming() )
                {
                    m_modifyProgressPanel->setCanceling();
                }
                else
                {
                    m_streamingProgressPanel->setCanceling();
                }
            }
            break;
        }
        // canceled
        case canceled:
        {
            if ( isCurrentDrive(drive) || (streamingDrive != nullptr) )
            {
                if ( !drive->isStreaming() )
                {
                    m_modifyProgressPanel->setCanceled();
                }
                else
                {
                    m_streamingProgressPanel->setCanceled();
                }
            }
            break;
        }
        // uploading
        case uploading:
        {
            if ( isCurrentDrive(drive) || (streamingDrive != nullptr) )
            {
                if ( !drive->isStreaming() )
                {
                    m_modifyProgressPanel->setUploading();
                }
                else
                {
                    m_streamingProgressPanel->setUploading();
                }

                if (!m_modificationStatusTimer->isActive()) {
                    m_modificationStatusTimer->start(1000);
                }

                if (!m_modificationStatusTimer->isActive()) {
                    m_modificationStatusTimer->start(1000);
                }
            }
            break;
        }
        // creating
        case creating:
        {
            if ( itIsNewState )
            {
                addEntityToUi(ui->m_driveCBox, drive->getName(), drive->getKey());
                addEntityToUi(ui->m_streamDriveCBox, drive->getName(), drive->getKey());
                setCurrentDriveOnUi(drive->getKey());
                updateDriveStatusOnUi(*drive);

                ui->m_drivePath->setText( "Path: /" );
                lockDrive();

                updateDriveView();
                updateDiffView();
            }
            break;
        }
        // unconfirmed
        case unconfirmed:
        {
            if ( itIsNewState )
            {
                removeEntityFromUi(ui->m_driveCBox, driveKey);
                removeEntityFromUi(ui->m_streamDriveCBox, driveKey);
                m_model->removeDrive(driveKey);

                m_model->applyForChannels(driveKey, [this](auto& channel) {
                    removeEntityFromUi(ui->m_channels, channel.getKey());
                });

                if (m_modificationStatusTimer->isActive()) {
                    m_modificationStatusTimer->stop();
                }

                m_model->removeChannelsByDriveKey(driveKey);
            }
            break;
        }
        // deleting
        case deleting:
        {
            if ( itIsNewState )
            {
                lockDrive();
                updateDriveStatusOnUi(*drive);

                m_model->markChannelsForDelete(drive->getKey(), true);
                auto channel = m_model->currentDownloadChannel();
                if (channel) {
                    lockChannel(channel->getKey());
                    updateDownloadChannelStatusOnUi(*channel);
                }
            }
            break;
        }
        // deleted
        case deleted:
        {
            if ( itIsNewState )
            {
                m_modificationsWatcher->removePath(drive->getLocalFolder().c_str());

                std::string driveName = drive->getName();

                removeEntityFromUi(ui->m_driveCBox, driveKey);
                removeEntityFromUi(ui->m_streamDriveCBox, driveKey);

                m_model->removeDrive(driveKey);
                m_model->applyForChannels(driveKey, [this](auto& channel) {
                    removeEntityFromUi(ui->m_channels, channel.getKey());
                });

                m_model->removeChannelsByDriveKey(driveKey);
                if (m_model->getDownloadChannels().empty()) {
                    m_channelFsTreeTableModel->setFsTree({}, {});
                    ui->m_path->setText( "Path:");
                } else {
                    m_channelFsTreeTableModel->setFsTree(m_model->getDownloadChannels().begin()->second.getFsTree(),
                                                         m_model->getDownloadChannels().begin()->second.getLastOpenedPath());

                    ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));
                }

                if (isCurrentDrive(drive)) {
                    updateDriveView();
                    updateDiffView();

                    if (m_modificationStatusTimer->isActive()) {
                        m_modificationStatusTimer->stop();
                    }
                }

                const QString message = QString::fromStdString( "Drive '" + driveName + "' closed (removed).");
                showNotification(message);
                addNotification(message);

                loadBalance();

                if (m_model->getDrives().empty()) {
                    lockDrive();
                    ui->m_drivePath->setText( "Path: /" );

                    if (m_settings->m_isDriveStructureAsTree) {
                        m_driveTreeModel->updateModel(false);
                        ui->m_driveTreeView->expand(m_driveTreeModel->index(0, 0));
                        m_diffTreeModel->updateModel(false);
                    } else {
                        m_driveTableModel->setFsTree({}, {} );
                        m_diffTableModel->updateModel();
                    }
                }
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

void MainWin::removeDrive( Drive* drive )
{
    m_modificationsWatcher->removePath(drive->getLocalFolder().c_str());

    std::string driveName = drive->getName();

    auto driveKey = drive->getKey();
    
    removeEntityFromUi(ui->m_driveCBox, driveKey);
    removeEntityFromUi(ui->m_streamDriveCBox, driveKey);

    m_model->removeDrive(driveKey);
    m_model->applyForChannels(driveKey, [this](auto& channel) {
        removeEntityFromUi(ui->m_channels, channel.getKey());
    });

    m_model->removeChannelsByDriveKey(driveKey);
    if (m_model->getDownloadChannels().empty()) {
        m_channelFsTreeTableModel->setFsTree({}, {});
        ui->m_path->setText( "Path:");
    } else {
        m_channelFsTreeTableModel->setFsTree(m_model->getDownloadChannels().begin()->second.getFsTree(),
                                             m_model->getDownloadChannels().begin()->second.getLastOpenedPath());

        ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));
    }

    if (isCurrentDrive(drive)) {
        updateDriveView();
        updateDiffView();

        if (m_modificationStatusTimer->isActive()) {
            m_modificationStatusTimer->stop();
        }
    }

    const QString message = QString::fromStdString( "Drive '" + driveName + "' closed (removed).");
    showNotification(message);
    addNotification(message);

    loadBalance();

    if (m_model->getDrives().empty()) {
        lockDrive();

        if (m_settings->m_isDriveStructureAsTree) {
            m_driveTreeModel->updateModel(false);
            ui->m_driveTreeView->expand(m_driveTreeModel->index(0, 0));
            m_diffTreeModel->updateModel(false);
        } else {
            m_driveTableModel->setFsTree({}, {} );
            m_diffTableModel->updateModel();
        }
    }
}

void MainWin::onDriveStateChanged( Drive& drive )
{
    emit driveStateChangedSignal( drive.getKey(), drive.getState(), true );
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
    m_model->setCurrentDownloadChannelKey(channelKey);

    auto drive = m_model->findDrive(driveKey);
    if (drive) {
        m_model->applyFsTreeForChannels(driveKey, drive->getFsTree(), drive->getRootHash());
    }

    addEntityToUi(ui->m_channels, channelName, channelKey);
    updateDownloadChannelStatusOnUi(*m_model->findChannel(channelKey));
    setDownloadChannelOnUi(channelKey);
    lockChannel(channelKey);
}

void MainWin::onChannelCreationConfirmed( const std::string& alias, const std::string& channelKey, const std::string& driveKey )
{
    qDebug() << "MainWin::onChannelCreationConfirmed: alias:" << alias << " channel key: " << channelKey << " drive key: " << driveKey;
    
    auto channel = m_model->findChannel( channelKey );
    if ( channel )
    {
        channel->setCreating(false);
        m_model->saveSettings();

        updateDownloadChannelStatusOnUi(*channel);
        updateDownloadChannelData(channel);

        const QString message = QString::fromStdString( "Channel '" + alias + "' created successfully.");
        if ( ! m_model->viewingFsTreeHandler() )
        {
            showNotification(message);
        }
        addNotification(message);

        unlockChannel(channelKey);
    }
    else
    {
        qWarning() << "MainWin::onChannelCreationConfirmed. Unknown channel " << channelKey;
    }
}

void MainWin::onChannelCreationFailed( const std::string& channelKey, const std::string& errorText )
{
    qDebug() << "MainWin::onChannelCreationFailed: key:" << channelKey << " error: " << errorText;

    auto channel = m_model->findChannel( channelKey );
    if (channel) {
        if ( m_model->viewingFsTreeHandler() )
        {
            (*m_model->viewingFsTreeHandler())( false, "", channel->getDriveKey() );
        }
        const QString message = QString::fromStdString( "Channel creation failed (" + channel->getName() + ")");
        showNotification(message, gErrorCodeTranslator.translate(errorText).c_str());
        addNotification(message);
        unlockChannel(channelKey);
        removeEntityFromUi(ui->m_channels, channelKey);
        m_model->removeChannel(channelKey);

    } else {
        qWarning() << "MainWin::onChannelCreationFailed. Unknown channel: " << channelKey;
    }
}

void MainWin::onDriveCreationConfirmed( const std::string &driveKey )
{
    qDebug() << "MainWin::onDriveCreationConfirmed: key:" << driveKey;
    auto drive = m_model->findDrive( driveKey );
    if ( drive )
    {
        m_modificationsWatcher->addPath(drive->getLocalFolder().c_str());
        drive->updateDriveState(no_modifications);
        m_model->saveSettings();

        const QString message = QString::fromStdString("Drive '" + drive->getName() + "' created successfully.");
        showNotification(message);
        addNotification(message);

        if ( m_modalDialog )
        {
            m_modalDialog->closeModal( true );
        }

    } else {
        qWarning() << "MainWin::onDriveCreationConfirmed. Unknown drive: " << driveKey;
    }
}

void MainWin::onDriveCreationFailed(const std::string& driveKey, const std::string& errorText)
{
    qDebug() << "MainWin::onDriveCreationFailed: key:" << driveKey << " error: " << errorText;

    auto drive = m_model->findDrive(driveKey);
    if (drive) {
        const QString message = QString::fromStdString( "Drive creation failed (" + drive->getName() + ")");
        showNotification(message, gErrorCodeTranslator.translate(errorText).c_str());
        addNotification(message);
        drive->updateDriveState(unconfirmed);

        if ( m_modalDialog )
        {
            m_modalDialog->closeModal( false );
        }

    } else {
        qWarning() << "MainWin::onDriveCreationFailed. Unknown drive: " << driveKey;
    }
}

void MainWin::onDriveCloseConfirmed(const std::array<uint8_t, 32>& driveKey) {
    const auto driveKeyHex = sirius::drive::toString(driveKey);

    qDebug() << "MainWin::onDriveCloseConfirmed: key:" << driveKeyHex;

    auto drive = m_model->findDrive(driveKeyHex);
    if (drive) {
        drive->updateDriveState(deleted);
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
        drive->updateDriveState(no_modifications);
    } else {
        qWarning () << "MainWin::onDriveCloseFailed. Unknown drive: " << driveId << " error: " << errorText;
    }

    QString message = "The drive '";
    message.append(alias.c_str());
    message.append("' was not close by reason: ");
    message.append( QString::fromStdString( gErrorCodeTranslator.translate(errorText.toStdString())));
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

    auto driveKeyHex = rawHashFromHex(drive->getKey().c_str());
    m_onChainClient->applyDataModification(driveKeyHex, actionList);
    drive->updateDriveState(registering);
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

    updateDownloadChannelData(channel);
}

void MainWin::onDownloadChannelCloseConfirmed(const std::array<uint8_t, 32> &channelId) {
    std::string channelName;
    const auto channelKey = sirius::drive::toString(channelId);
    auto channel = m_model->findChannel( channelKey );
    if (channel)
    {
        channelName = channel->getName();
        const QString message = QString::fromStdString( "Channel '" + channelName + "' closed (removed).");
        showNotification(message);
        addNotification(message);
    }

    removeEntityFromUi(ui->m_channels, channelKey);
    unlockChannel("");

    loadBalance();
    m_model->removeChannel( sirius::drive::toString(channelId) );

    if (m_model->getDownloadChannels().empty()) {
        m_channelFsTreeTableModel->setFsTree( {}, {} );
        ui->m_path->setText( "Path:");
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
    message.append( QString::fromStdString( gErrorCodeTranslator.translate(errorText.toStdString())));
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

    const QString message = QString::fromStdString( "Your payment for the following channel was successful: '" + alias + "'");
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

    const QString message = QString::fromStdString( "Your payment for the following channel was UNSUCCESSFUL: '" + alias  + "' ");
    showNotification(message, QString::fromStdString(gErrorCodeTranslator.translate(errorText.toStdString())));
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

    const QString message = QString::fromStdString( "Your payment for the following drive was successful: '" + alias  + "'");
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

    const QString message = QString::fromStdString( "Your payment for the following drive was UNSUCCESSFUL: '" + alias  + "' ");
    showNotification(message, QString::fromStdString(gErrorCodeTranslator.translate(errorText.toStdString())));
    addNotification(message);
}

void MainWin::onDataModificationTransactionConfirmed(const std::array<uint8_t, 32>& driveKey, const std::array<uint8_t, 32> &modificationId) {
    qDebug () << "MainWin::onDataModificationTransactionConfirmed. Your last modification was registered: '" + rawHashToHex(modificationId);
    if ( auto drive = m_model->findDrive( sirius::drive::toString(driveKey)); drive != nullptr )
    {
        drive->setModificationHash( modificationId );
        drive->updateDriveState(uploading);
    }
    else
    {
        qDebug () << "MainWin::onDataModificationTransactionConfirmed. Your last modification was NOT registered: !!! drive not found";
    }

    loadBalance();
}

void MainWin::onDataModificationTransactionFailed(const std::array<uint8_t, 32>& driveKey, const std::array<uint8_t, 32> &modificationId, const QString& status) {
    qDebug () << "MainWin::onDataModificationTransactionFailed. Your last modification was declined: '" + rawHashToHex(modificationId);
    auto message = gErrorCodeTranslator.translate(status.toStdString());

    showNotification("Data modification failed: ", message.c_str());
    addNotification(message.c_str());

    if ( auto drive = m_model->findDrive( sirius::drive::toString(driveKey)); drive != nullptr )
    {
        drive->updateDriveState(failed);
    }
}

void MainWin::onDataModificationApprovalTransactionConfirmed(const std::array<uint8_t, 32> &driveId,
                                                             const std::string &fileStructureCdi) {
    std::string driveAlias = rawHashToHex(driveId).toStdString();
    auto drive = m_model->findDrive(driveAlias);
    if (drive) {
        drive->updateDriveState(approved);
        driveAlias = drive->getName();
    } else {
        qWarning () << LOG_SOURCE << "bad drive (alias not found): " << driveAlias;
    }

    qDebug () << LOG_SOURCE << "Your modification was APPLIED. Drive: " +
                 QString::fromStdString(driveAlias) + " CDI: " +
                 QString::fromStdString(fileStructureCdi);
}

void MainWin::onDataModificationApprovalTransactionFailed(const std::array<uint8_t, 32> &driveId, const std::string &fileStructureCdi, uint8_t code) {
    std::string driveAlias = rawHashToHex(driveId).toStdString();
    auto drive = m_model->findDrive(driveAlias);
    if (drive) {
        driveAlias = drive->getName();
        drive->updateDriveState(failed);
    } else {
        qWarning () << LOG_SOURCE << "bad drive (alias not found): " << driveAlias;
    }

    QString error;
    auto status = static_cast<sirius::drive::ModificationStatus>(code);
    if (code == 2) { // ACTION_LIST_IS_ABSENT
        error = " because action list is absent.";
    } else if (code == 4) { // DOWNLOAD_FAILED
        error = " because download data failed.";
    } else if (code == 1) { // INVALID_ACTION_LIST
        error = " because action list is incorrect.";
    } else if (code == 3) { // NOT_ENOUGH_SPACE
        error = " because there is not enough free space.";
    } else {
        error = " because of unknown internal error.";
    }

    QString message;
    message.append("Your modification was DECLINED, Drive: ");
    message.append(driveAlias.c_str());
    showNotification(message, error);
	addNotification(message);
}

void MainWin::onCancelModificationTransactionConfirmed(const std::array<uint8_t, 32> &driveId, const QString &modificationId) {
    std::string driveRawId = rawHashToHex(driveId).toStdString();
    auto drive = m_model->findDrive(driveRawId);
    if (drive) {
        drive->updateDriveState(canceled);
    } else {
        qWarning () << LOG_SOURCE << "bad drive: " << driveRawId.c_str() << " modification id: " << modificationId;
    }
}

void MainWin::onCancelModificationTransactionFailed(const std::array<uint8_t, 32> &driveId, const QString &modificationId, const QString& error) {
    std::string driveRawId = rawHashToHex(driveId).toStdString();
    auto drive = m_model->findDrive(driveRawId);
    if (drive) {
        drive->updateDriveState(failed);
    } else {
        qWarning () << LOG_SOURCE << "bad drive: " << driveRawId.c_str() << " modification id: " << modificationId;
    }

    auto errorText = gErrorCodeTranslator.translate(error.toStdString());
    showNotification(errorText.c_str());
    addNotification(errorText.c_str());
}

void MainWin::onReplicatorOnBoardingTransactionConfirmed(const QString& replicatorPublicKey) {
    qInfo () << "MainWin::onReplicatorOnBoardingTransactionConfirmed" << replicatorPublicKey;

    m_onChainClient->getBlockchainEngine()->getReplicatorById(replicatorPublicKey.toStdString(), [this] (auto replicator, auto isSuccess, auto message, auto code ) {
        if (!isSuccess) {
            qWarning() << "MainWin::setupMyReplicatorTab::getReplicatorById. Error: " << message.c_str() << " : " << code.c_str();
            return;
        }

        m_myReplicatorsModel->setupModelData({ replicator }, m_model);
    });

    QString message;
    message.append("Replicator onboarded successful: ");
    message.append(replicatorPublicKey);
    showNotification(message);
    addNotification(message);
}

void MainWin::onReplicatorOnBoardingTransactionFailed(const QString& replicatorPublicKey, const QString& replicatorPrivateKey, const QString& error) {
    qInfo () << "MainWin::onReplicatorOnBoardingTransactionFailed" << replicatorPublicKey;

    m_model->removeMyReplicator(replicatorPrivateKey.toStdString());
    m_model->saveSettings();

    QString message;
    message.append("Replicator onboarded FAILED: ");
    message.append(replicatorPublicKey);
    showNotification(message, QString::fromStdString(gErrorCodeTranslator.translate(error.toStdString())));
    addNotification(message);
}

void MainWin::onReplicatorOffBoardingTransactionConfirmed(const QString& replicatorPublicKey) {
    qInfo () << "MainWin::onReplicatorOffBoardingTransactionConfirmed" << replicatorPublicKey;

    QString message;
    message.append("Replicator off-boarded successful: ");
    message.append(replicatorPublicKey);
    showNotification(message);
    addNotification(message);
}

void MainWin::onReplicatorOffBoardingTransactionFailed(const QString& replicatorPublicKey, const QString& error) {
    qInfo () << "MainWin::onReplicatorOffBoardingTransactionFailed" << replicatorPublicKey;

    QString message;
    message.append("Replicator off-boarded FAILED: ");
    message.append(replicatorPublicKey);
    showNotification(message, QString::fromStdString(gErrorCodeTranslator.translate(error.toStdString())));
    addNotification(message);
}

void MainWin::loadBalance() {
    auto publicKey = m_model->getClientPublicKey();
    m_onChainClient->getBlockchainEngine()->getAccountInfo(
            publicKey, [this](xpx_chain_sdk::AccountInfo info, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            qWarning() << "MainWin::loadBalance. GetAccountInfo: " << message.c_str() << " : " << code.c_str();
            return;
        }

        auto mosaicIterator = std::find_if( info.mosaics.begin(), info.mosaics.end(), [this]( auto mosaic )
        {
            return mosaic.id == m_XPX_MOSAIC_ID;
        });

        const uint64_t balance = mosaicIterator == info.mosaics.end() ? 0 : mosaicIterator->amount;
        ui->m_balanceValue->setText(QString::number(balance));
    });
}

void MainWin::onCurrentChannelChanged( int index )
{
    if (index < 0) {
        qWarning () << "MainWin::onCurrentChannelChanged: invalid index!";
        return;
    }

    const auto channelKey = ui->m_channels->currentData().toString();
    m_model->setCurrentDownloadChannelKey(channelKey.toStdString());

    auto channel = m_model->currentDownloadChannel();
    if ( !channel )
    {
        qWarning() << "MainWin::onCurrentChannelChanged. Download channel not found!";
        m_channelFsTreeTableModel->setFsTree( {}, {} );
        return;
    }

    updateDownloadChannelData(channel);
    lockChannel(channel->getKey());
    unlockChannel(channel->getKey());
}

void MainWin::onDriveLocalDirectoryChanged(const QString& path) {
    auto drive = m_model->currentDrive();
    if (drive && drive->getLocalFolder() == path.toStdString()) {
        calculateDiffAsync([this](auto, auto){
            updateDiffView();
        });
    }
}

void MainWin::onCurrentDriveChanged( int index )
{
    if (index >= 0) {
        const auto driveKey = ui->m_driveCBox->currentData().toString();
        m_model->setCurrentDriveKey( driveKey.toStdString() );

        auto drive = m_model->currentDrive();
        if (drive) {
            updateDriveWidgets(drive->getKey(), drive->getState(), false);
            updateDriveView();
            updateDiffView();
            ui->m_drivePath->setText( "Path: /" );
        } else {
            qWarning() << "MainWin::onCurrentDriveChanged. Drive not found (Invalid pointer to drive)";
            m_driveTableModel->setFsTree({}, {} );
        }
    }
}

std::vector<DataInfo> MainWin::readDataInfoList() //wizardReadStreamInfoList
{
    std::vector<DataInfo> dataInfoVector;
    // Drive* drive = m_model->currentDrive();
    // drive.
    // for( auto& driveInfo : drives )
    // {
    //     auto infoVector = readStreamInfoList(driveInfo.second);
    //     streamInfoVector.insert(streamInfoVector.end(), infoVector.begin(), infoVector.end());
    // }
    return dataInfoVector;
}

void MainWin::setupDrivesTab()
{
    ui->m_driveCBox->addItem( "Loading..." );
    ui->m_streamDriveCBox->addItem( "Loading..." );
    setupDriveFsTable();
    connect( ui->m_openDriveLocalFolderBtn, &QPushButton::released, this, [this]
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
            QDesktopServices::openUrl( QUrl::fromLocalFile( QString::fromStdString( driveInfo->getLocalFolder() )));
#else
//            QProcess fileManagerProcess;
//            fileManagerProcess.startDetached("xdg-open", QStringList() << QString::fromStdString( driveInfo->getLocalFolder() ));
//            if (!fileManagerProcess.waitForStarted())
//            {
//                QMessageBox::critical(nullptr, "Error", "Failed to open folder: " +
//                                                            QString::fromStdString( driveInfo->getLocalFolder() ));
//            }
            qDebug() << "TRY OPEN " << driveInfo->getLocalFolder();
            QDesktopServices::openUrl( QUrl::fromLocalFile( QString::fromStdString( driveInfo->getLocalFolder() )));
#endif
        }
    });

    connect(ui->m_copyLinkToDataBtn,&QPushButton::released, this, [this]()
    {
        std::string             path;
        std::string             itemName;
        sirius::drive::Folder*  folder = nullptr;
        
        auto rows = ui->m_driveFsTableView->selectionModel()->selectedRows();
        if ( rows.size() == 0 )
        {
            folder = m_driveTableModel->currentSelectedItem( -1, path, itemName );
        }
        else
        {
            folder = m_driveTableModel->currentSelectedItem( rows.at(0).row(), path, itemName );
        }

        qDebug() << "copyLinkToData: key: " << m_model->currentDrive()->getKey().c_str();
        qDebug() << "copyLinkToData: driveName: " << m_model->currentDrive()->getName();
        qDebug() << "copyLinkToData: path: " << path.c_str();
        qDebug() << "copyLinkToData: itemName: " << itemName.c_str();

        auto fsTree = m_model->currentDrive()->getFsTree();
        if ( folder == nullptr )
        {
            folder = &fsTree;
        }
        
        size_t dataSize = 0;

        if ( ! itemName.empty() )
        {
            auto* child = folder->findChild( itemName );
            if ( child == nullptr )
            {
                qWarning() << "CopyLink: Internal error: child == nullptr";
                return;
            }
            if ( sirius::drive::isFolder(*child) )
            {
                folder = &sirius::drive::getFolder(*child);
            }
            else
            {
                const auto* file = &sirius::drive::getFile(*child);
                //qDebug() << "copyLinkToData: file: " << file->size() << " " << file->name().c_str();
                dataSize += file->size();
            }
        }

        if ( dataSize == 0 )
        {
            folder->iterate( [&dataSize] (const auto& file ) -> bool
            {
                //qDebug() << "copyLinkToData: iterate: " << file.size() << " " << file.name().c_str();
                dataSize += file.size();
                return false;
            });
        }
        qDebug() << "copyLinkToData: dataSize: " << dataSize;

        DataInfo dataInfo( Model::hexStringToHash(m_model->currentDrive()->getKey())
                          , m_model->currentDrive()->getName()
                          , path
                          , dataSize );
        
        ConfirmLinkDialog dialog(this, dataInfo);
        dialog.exec();
    });

    connect( ui->m_calcDiffBtn, &QPushButton::released, this, [this]
    {
        auto drive = m_model->currentDrive();
        if (!drive) {
            qWarning () << "MainWin::m_calcDiffBtn::released. Invalid pointer to drive!";
            return;
        }

        auto callback = [this, drive](bool isOutdated) {
            if (isOutdated) {
                const auto rootHash = rawHashToHex(drive->getRootHash()).toStdString();
                onDownloadFsTreeDirect(drive->getKey(), rootHash);
            } else {
                if (isCurrentDrive(drive)) {
                    calculateDiffAsync([this](auto, auto){
                        updateDiffView();
                    });
                } else {
                    calculateDiffAsync({});
                }
            }
        };

        checkDriveForUpdates(drive, callback);
    }, Qt::QueuedConnection);

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
        if ( auto* driveInfo = m_model->currentDrive(); driveInfo != nullptr )
        {
            std::string driveName = driveInfo->getName();
            std::string driveKey = driveInfo->getKey();

            EditDialog dialog( "Rename drive", "Drive name:", driveName, EditDialog::Drive, m_model );
            if ( dialog.exec() == QDialog::Accepted )
            {
                if ( auto* driveInfo = m_model->findDrive(driveKey); driveInfo != nullptr )
                {
                    driveInfo->setName(driveName);
                    m_model->saveSettings();
                    updateEntityNameOnUi(ui->m_driveCBox, driveInfo->getName(), driveInfo->getKey());
                }
            }
        }
    });

    auto changeFolderAction = new QAction("Change local folder", this);
    menu->addAction(changeFolderAction);
    connect( changeFolderAction, &QAction::triggered, this, [this](bool)
    {
        if ( auto* driveInfo = m_model->currentDrive(); driveInfo != nullptr )
        {
            std::string driveName = driveInfo->getName();
            std::string driveKey = driveInfo->getKey();

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
                if ( auto driveInfo = m_model->findDrive(driveKey); driveInfo )
                {
                    if (path.toStdString() != driveInfo->getLocalFolder()) {
                        m_modificationsWatcher->removePath(driveInfo->getLocalFolder().c_str());
                        m_modificationsWatcher->addPath(path.toStdString().c_str());
                        driveInfo->setLocalFolder(path.toStdString());
                        driveInfo->setLocalFolderExists(true);
                        updateDriveWidgets(driveInfo->getKey(), driveInfo->getState(), false);
                        m_model->saveSettings();
                        ui->m_drivePath->setText( "Path: /" );
                    }
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
        if ( auto* driveInfo = m_model->currentDrive(); driveInfo != nullptr )
        {
            std::string driveKey = driveInfo->getKey();

            QClipboard* clipboard = QApplication::clipboard();
            if ( !clipboard ) {
                qWarning() << LOG_SOURCE << "bad clipboard";
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
        if ( auto* driveInfo = m_model->currentDrive(); driveInfo != nullptr )
        {
            DriveInfoDialog dialog( *driveInfo, this);
            dialog.exec();
        }
    });

    ui->m_moreDrivesBtn->setMenu(menu);
}

void MainWin::setupMyReplicatorTab() {
    m_myReplicatorsModel = new ReplicatorTreeModel(this);
    ui->m_myReplicators->setModel(m_myReplicatorsModel);
    ui->m_myReplicators->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    for (const auto& r : m_settings->accountConfig().m_myReplicators) {
        m_onChainClient->getBlockchainEngine()->getReplicatorById(r.second.getPublicKey(), [this, r] (auto replicator, auto isSuccess, auto message, auto code ) {
            if (!isSuccess) {
                qWarning() << "MainWin::setupMyReplicatorTab::getReplicatorById. Error: " << message.c_str() << " : " << code.c_str();
                m_model->removeMyReplicator(r.first);
                return;
            }

            m_myReplicatorsModel->setupModelData({ replicator }, m_model);
        });
    }

    bool haveReplicators = ! m_model->getMyReplicators().empty();
    ui->m_myReplicators->setVisible( haveReplicators );
    ui->m_offBoardReplicatorBtn->setVisible( haveReplicators );
    ui->m_replicatorsLabel->setVisible( haveReplicators );

    connect(this, &MainWin::refreshMyReplicatorsTable, this, [this]()
    {
        m_myReplicatorsModel->setupModelData(m_model->getMyReplicators());

        bool haveReplicators = ! m_model->getMyReplicators().empty();
        ui->m_myReplicators->setVisible( haveReplicators );
        ui->m_offBoardReplicatorBtn->setVisible( haveReplicators );
        ui->m_replicatorsLabel->setVisible( haveReplicators );
    }, Qt::QueuedConnection);

    connect(ui->m_myReplicators, &QTreeView::doubleClicked, this, [this](QModelIndex index){
        if (!index.isValid()) {
            return;
        }

        auto item = static_cast<ReplicatorTreeItem*>(index.internalPointer());
        if (!item) {
            qWarning() << "MainWin::setupMyReplicatorTab. Invalid item";
            return;
        }

        auto dialog = new ReplicatorInfoDialog(item->getPublicKey().toStdString(), m_model, this);
        dialog->open();

        m_onChainClient->getBlockchainEngine()->getReplicatorById(item->getPublicKey().toStdString() ,[this, dialog] (auto r, auto isSuccess, auto m, auto c) {
            if (!isSuccess) {
                qWarning() << "ReplicatorInfoDialog::ReplicatorInfoDialog. Error: " << m << " : Code: " << c;
                return;
            }

            if (dialog && dialog->isVisible()) {
                dialog->updateChannelsAmount(r.data.downloadChannels.size());
            }

            if (dialog && dialog->isVisible()) {
                dialog->updateDrivesAmount(r.data.drivesInfo.size());
            }

            getMosaicIdByName(r.data.key.c_str(), "so", [this, r, dialog](auto id) {
                m_onChainClient->getBlockchainEngine()->getAccountInfo(
                        r.data.key, [this, r, id, dialog](xpx_chain_sdk::AccountInfo info, auto isSuccess, auto message, auto code) {
                            if (!isSuccess) {
                                qWarning() << "loadBalance. ReplicatorInfoDialog: " << message.c_str() << " : " << code.c_str();
                                return;
                            }

                            auto mosaicIterator = std::find_if( info.mosaics.begin(), info.mosaics.end(), [id]( auto mosaic )
                            {
                                return mosaic.id == id;
                            });

                            const uint64_t freeSpace = mosaicIterator == info.mosaics.end() ? 0 : mosaicIterator->amount;
                            if (dialog && dialog->isVisible()) {
                                dialog->updateFreeSpace(freeSpace);
                            }

                            // Can be called few times
                            m_onChainClient->calculateUsedSpaceOfReplicator(r.data.key.c_str(), [dialog](auto index, auto usedSpace){
                                if (dialog && dialog->isVisible()) {
                                    dialog->updateTotalSpace(index, usedSpace);
                                }
                            });
                        });
            });
        });
    });
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

void MainWin::showSettingsDialog()
{
    SettingsDialog settingsDialog( m_settings, this );
    connect(&settingsDialog, &SettingsDialog::closeLibtorrentPorts, this, [this]()
    {
        m_onChainClient->getStorageEngine()->restart();
        QCoreApplication::exit(1024);
    });
    settingsDialog.exec();
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

void MainWin::updateEntityNameOnUi(QComboBox* box, const std::string& name, const std::string& key)
{
    const int index = box->findData(QVariant::fromValue(QString::fromStdString(key)), Qt::UserRole, Qt::MatchFixedString);
    if (index != -1) {
        box->setItemText(index, QString::fromStdString(name));
    } else {
        qWarning () << "MainWin::updateEntityNameOnUi. Unknown entity. Name: " << name << " Key: " << key;
    }
}

void MainWin::updateDownloadChannelStatusOnUi(const DownloadChannel& channel)
{
    const int index = ui->m_channels->findData(QVariant::fromValue(QString::fromStdString(channel.getKey())), Qt::UserRole, Qt::MatchFixedString);
    if (index != -1) {
        if (channel.isCreating()) {
            ui->m_channels->setItemText(index, QString::fromStdString(channel.getName() + "(creating...)"));
        } else if (channel.isDeleting()) {
            ui->m_channels->setItemText(index, QString::fromStdString(channel.getName() + "(...deleting)"));
        } else {
            ui->m_channels->setItemText(index, QString::fromStdString(channel.getName()));
        }
    }
}

void MainWin::updateDriveStatusOnUi(const Drive& drive)
{
    const int index = ui->m_driveCBox->findData(QVariant::fromValue(QString::fromStdString(drive.getKey())), Qt::UserRole, Qt::MatchFixedString);
    if (index != -1) {
        if ( drive.getState() == creating ) {
            ui->m_driveCBox->setItemText(index, QString::fromStdString(drive.getName() + "(creating...)"));
            ui->m_streamDriveCBox->setItemText(index, QString::fromStdString(drive.getName() + "(creating...)"));
        } else if (drive.getState() == deleting) {
            ui->m_driveCBox->setItemText(index, QString::fromStdString(drive.getName() + "(...deleting)"));
            ui->m_streamDriveCBox->setItemText(index, QString::fromStdString(drive.getName() + "(...deleting)"));
        } else if (drive.getState() == no_modifications) {
            ui->m_driveCBox->setItemText(index, QString::fromStdString(drive.getName()));
            ui->m_streamDriveCBox->setItemText(index, QString::fromStdString(drive.getName()));

//            if ( m_modalDialog )
//            {
//                m_modalDialog->closeModal();
//            }
        }
    }
}

void MainWin::addEntityToUi(QComboBox* box, const std::string& name, const std::string& key)
{
    const int index = box->findData(QVariant::fromValue(QString::fromStdString(key)), Qt::UserRole, Qt::MatchFixedString);
    if (index == -1) {
        box->addItem(QString::fromStdString(name), QString::fromStdString(key));
        box->model()->sort(0);
    }
}

void MainWin::removeEntityFromUi(QComboBox* box, const std::string& key)
{
    const int index = box->findData(QVariant::fromValue(QString::fromStdString(key)), Qt::UserRole, Qt::MatchFixedString);
    if (index != -1) {
        box->removeItem(index);
    }
}

void MainWin::lockChannel(const std::string &channelId) {
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
    ui->m_openDriveLocalFolderBtn->setDisabled(true);
    ui->m_applyChangesBtn->setDisabled(true);
    ui->m_diffTableView->setDisabled(true);
    ui->m_calcDiffBtn->setDisabled(true);
}

void MainWin::unlockDrive() {
    ui->m_closeDrive->setEnabled(true);
    ui->m_moreDrivesBtn->setEnabled(true);
    ui->m_driveTreeView->setEnabled(true);
    ui->m_driveFsTableView->setEnabled(true);
    ui->m_openDriveLocalFolderBtn->setEnabled(true);
    ui->m_applyChangesBtn->setEnabled(true);
    ui->m_diffTableView->setEnabled(true);
    ui->m_calcDiffBtn->setEnabled(true);
}

void MainWin::networkDataHandler(const QString networkName)
{
    qDebug() << "MainWin::networkDataHandler. Network layer initialized successfully. Network name: " << networkName;
    getMosaicIdByName(m_model->getClientPublicKey().c_str(), "xpx",[this](auto id)
    {
        m_XPX_MOSAIC_ID = id;
        loadBalance();
    });

    ui->m_networkName->setText(networkName);

    xpx_chain_sdk::DrivesPageOptions options;
    options.owner = m_model->getClientPublicKey();
    m_onChainClient->loadDrives(options);

    if (m_model->getDownloadChannels().empty())
    {
        lockChannel("");
    }
    else
    {
        unlockChannel("");
    }

    if (m_model->getDrives().empty())
    {
        lockDrive();
    }
    else
    {
        unlockDrive();
    }
}

void MainWin::onCloseChannel()
{
    auto channel = m_model->currentDownloadChannel();
    if (!channel) {
        qWarning() << LOG_SOURCE << "Bad current download channel. Channels count: " << m_model->getDownloadChannels().size();
        return;
    }

    CloseChannelDialog dialog(m_onChainClient, channel->getKey().c_str(), channel->getName().c_str(), this);
    auto rc = dialog.exec();
    if ( rc == QMessageBox::Ok )
    {
        channel->setDeleting(true);
        updateDownloadChannelStatusOnUi(*channel);
        lockChannel(channel->getKey());
    }
}

void MainWin::onCloseDrive()
{
    auto drive = m_model->currentDrive();
    if (!drive) {
        qWarning() << LOG_SOURCE << "bad drive";
        return;
    }

    CloseDriveDialog dialog(m_onChainClient, drive, this);
    dialog.exec();
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
    m_onChainClient->getStorageEngine()->downloadFsTree(driveId, channelId, rawHashFromHex(fileStructureCdi.c_str()));
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
        m_onChainClient->getStorageEngine()->downloadFsTree(channel->getDriveKey(), channelId, rawHashFromHex(fileStructureCdi.c_str()));
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
        
        if (isCurrentDrive(drive)) {
            calculateDiffAsync([this](auto, auto){
                updateDiffView();
            });

            updateDriveView();
        }

        // inform stream annotations about possible changes
        onFsTreeReceivedForStreamAnnotations( *drive );
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
    ui->m_addDrive->setDisabled(state);
}

void MainWin::closeEvent(QCloseEvent *event) {
    if (event) {
        m_model->setWindowGeometry(frameGeometry());
        m_model->saveSettings();
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
        return boost::iequals(drive->getKey(), currentDrive->getKey());
    }

    return false;
}

bool MainWin::isCurrentDownloadChannel(DownloadChannel* channel)
{
    auto currentChannel = m_model->currentDownloadChannel();
    if (channel && currentChannel) {
        return boost::iequals(channel->getKey(), currentChannel->getKey());
    }

    return false;
}

void MainWin::updateDiffView() {
    if (m_settings->m_isDriveStructureAsTree) {
        m_diffTreeModel->updateModel(false);
        ui->m_diffTreeView->expand(m_diffTreeModel->index(0, 0));
    } else {
        m_diffTableModel->updateModel();
    }
}

void MainWin::updateDriveView() {
    if (m_settings->m_isDriveStructureAsTree) {
        m_driveTreeModel->updateModel(false);
        ui->m_driveTreeView->expand(m_driveTreeModel->index(0, 0));
    } else {
        auto drive = m_model->currentDrive();
        if (drive) {
            m_driveTableModel->setFsTree(drive->getFsTree(), { drive->getLocalFolder() } );
        } else if(m_model->getDrives().empty()) {
            m_driveTableModel->setFsTree({}, {} );
        } else {
            qWarning() << "MainWin::updateDriveView: invalid drive";
        }
    }
}

void MainWin::updateDownloadChannelData(DownloadChannel* channel) {
    if (!channel) {
        qWarning () << "MainWin::updateDownloadChannelData. Invalid download channel!";
        return;
    }

    auto updateReplicatorsCallback = [this, channel]() {
        auto driveUpdatesCallback = [this, channel](bool isOutdated) {
            if (isOutdated) {
                const auto rootHash = rawHashToHex(channel->getFsTreeHash()).toStdString();
                downloadFsTreeByChannel(channel->getKey(), rootHash);
            } else {
                if (isCurrentDownloadChannel(channel)) {
                    m_channelFsTreeTableModel->setFsTree( channel->getFsTree(), channel->getLastOpenedPath() );
                    ui->m_path->setText( "Path: " + QString::fromStdString(m_channelFsTreeTableModel->currentPathString()));
                }
            }

            loadBalance();
        };

        checkDriveForUpdates(channel, driveUpdatesCallback);
        loadBalance();
    };

    updateReplicatorsForChannel(channel->getKey(), updateReplicatorsCallback);
}

void MainWin::getMosaicIdByName(const QString& accountPublicKey, const QString& mosaicName, std::function<void(uint64_t mosaicId)> callback) {
    m_onChainClient->getBlockchainEngine()->getAccountInfo(
            accountPublicKey.toStdString(), [this, mosaicName, callback](xpx_chain_sdk::AccountInfo info, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            qWarning() << "MainWin::getMosaicIdByName. GetAccountInfo: " << message.c_str() << " : " << code.c_str();
            const QString clientPublicKey = QString::fromStdString(message);
            if (clientPublicKey.contains(m_model->getClientPublicKey().c_str(), Qt::CaseInsensitive))
            {
                qWarning() << "Account info not found. The account can be new! Balance set to 0!";
            }
            else
            {
                onErrorsHandler(ErrorType::InvalidData, message.c_str());
            }

            return callback(0);
        }

        std::vector<xpx_chain_sdk::MosaicId> mosaicIds;
        for (const xpx_chain_sdk::Mosaic& mosaic : info.mosaics)
        {
            mosaicIds.push_back(mosaic.id);
        }

        m_onChainClient->getBlockchainEngine()->getMosaicsNames(
                mosaicIds, [mosaicName, info, callback](xpx_chain_sdk::MosaicNames mosaicDescriptors, auto isSuccess, auto message, auto code) {
            if (!isSuccess)
            {
                qWarning() << "MainWin::getMosaicIdByName. GetMosaicsNames: " << message.c_str() << " : " << code.c_str();
                return;
            }

            uint64_t id = 0;
            bool isMosaicFound = false;
            for (const auto& rawInfo : mosaicDescriptors.names) {
                for (const auto& name : rawInfo.names) {
                    QString currentName(name.c_str());
                    if (currentName.contains(mosaicName, Qt::CaseInsensitive)) {
                        id = rawInfo.mosaicId;
                        isMosaicFound = true;
                        break;
                    }
                }

                if (isMosaicFound) {
                    break;
                }
            }

            callback(id);
        });
    });
}

void MainWin::initWorker() {
    mpWorker->moveToThread(mpThread);
    connect(mpThread, &QThread::started, mpWorker, [w = mpWorker]() { w->init(0, false); }, Qt::QueuedConnection);
    connect(mpWorker, &Worker::done, this, &MainWin::callbackResolver, Qt::QueuedConnection);
    connect(mpThread, &QThread::finished, mpThread, &QThread::deleteLater);
    connect(mpThread, &QThread::finished, mpWorker, &Worker::deleteLater);
    connect(this, &MainWin::addResolver, this, &MainWin::onAddResolver, Qt::QueuedConnection);
    connect(this, &MainWin::removeResolver, this, &MainWin::onRemoveResolver, Qt::QueuedConnection);
    connect(this, &MainWin::runProcess, mpWorker, &Worker::process, Qt::QueuedConnection);
    mpThread->start();
}

void MainWin::onAddResolver(const QUuid &id, const std::function<void(QVariant)>& resolver) {
    mResolvers.emplace(id, resolver);
}

void MainWin::onRemoveResolver(const QUuid &id) {
    mResolvers.erase(id);
}

void MainWin::callbackResolver(const QUuid& id, const QVariant& data) {
    mResolvers[id](data);
}

void MainWin::calculateDiffAsync(const std::function<void(int, std::string)>& callback) {
    auto taskCalcDiff = [this]() {
        try {
            if ( auto* drive = m_model->currentDrive(); drive != nullptr )
            {
                Model::calcDiff( *drive );
            }
            return QVariant::fromValue(0);
        } catch (std::exception& e) {
            return QVariant::fromValue(std::string(e.what()));
        }
    };

    const QUuid id = QUuid::createUuid();
    auto resolver = [this, id, callback] (QVariant data) {
        emit removeResolver(id);
        if (callback) {
            typeResolver<int>(data, callback);
        }
    };

    emit addResolver(id, resolver);
    emit runProcess(id, taskCalcDiff);
}

void MainWin::dataModificationsStatusHandler(const sirius::drive::ReplicatorKey &replicatorKey,
                                             const sirius::Hash256 &modificationHash,
                                             const sirius::drive::ModifyTrafficInfo &msg,
                                             lt::string_view,
                                             bool isModificationQueued,
                                             bool isModificationFinished,
                                             const std::string &error) {
    if (error == "not found" || isModificationQueued) {
        return;
    }

    if (!error.empty()) {
        qInfo() << "MainWin::dataModificationsStatusHandler. Error: " << error;
        return;
    }

    uint64_t receivedSize = 0;
    for (const auto& replicatorInfo : msg.m_modifyTrafficMap) {
        receivedSize += replicatorInfo.second.m_receivedSize;
    }

    if (receivedSize > 0) {
        emit updateUploadedDataAmount(receivedSize,msg.m_modifyTrafficMap.size());
    }

    if (isModificationFinished) {
        emit modificationFinishedByReplicators();
    }
}

ContractDeploymentData* MainWin::contractDeploymentData() {
    if ( ui->m_contractDriveCBox->currentIndex() == -1 ) {
        return nullptr;
    }
    auto driveKey = ui->m_contractDriveCBox->currentData().toString().toStdString();
    auto& contractDrives = m_model->driveContractModel().getContractDrives();
    auto contractDriveIt = contractDrives.find( driveKey );
    if ( contractDriveIt == contractDrives.end()) {
        return nullptr;
    }
    return &contractDriveIt->second;
}

void
MainWin::onDeployContractTransactionConfirmed( std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId ) {
    if ( auto drive = m_model->findDrive( sirius::drive::toString( driveKey )); drive != nullptr ) {
        drive->updateDriveState( contract_deploying );
    }
}

void
MainWin::onDeployContractTransactionFailed( std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId ) {
    if ( auto drive = m_model->findDrive( sirius::drive::toString( driveKey )); drive != nullptr ) {
        drive->updateDriveState( no_modifications );
    }
}

void MainWin::onDeployContractApprovalTransactionConfirmed( std::array<uint8_t, 32> driveKey,
                                                            std::array<uint8_t, 32> contractId ) {
    if ( auto drive = m_model->findDrive( sirius::drive::toString( driveKey )); drive != nullptr ) {
        drive->updateDriveState( contract_deployed );
    }
}

void MainWin::onDeployContractApprovalTransactionFailed( std::array<uint8_t, 32> driveKey,
                                                         std::array<uint8_t, 32> contractId ) {
    if ( auto drive = m_model->findDrive( sirius::drive::toString( driveKey )); drive != nullptr ) {
        drive->updateDriveState( no_modifications );
    }
}

void MainWin::onDeployContract() {
    if ( ui->m_contractDriveCBox->currentIndex() == -1 ) {
        return;
    }
    auto driveKey = ui->m_contractDriveCBox->currentData().toString().toStdString();
    auto& contractDrives = m_model->driveContractModel().getContractDrives();
    auto contractDriveIt = contractDrives.find( driveKey );
    if ( contractDriveIt == contractDrives.end()) {
        return;
    }

    m_onChainClient->deployContract(rawHashFromHex(QString::fromStdString(contractDriveIt->first)),
                                    contractDriveIt->second);
}

void MainWin::onRunContract() {
    m_onChainClient->runContract(m_model->driveContractModel().getContractManualCallData());
    ui->m_contractCallKey->clear();
    ui->m_contractCallFile->clear();
    ui->m_contractCallFunction->clear();
    ui->m_contractCallParameters->clear();
    ui->m_contractCallExecutionPayment->setValue(0);
    ui->m_contractCallDownloadPayment->setValue(0);
    ui->m_contractCallMosaicTable->setRowCount(0);
    m_model->driveContractModel().getContractManualCallData() = ContractManualCallData{};
}

//void MainWin::validateContractDrive() {
//    bool isContractValid =
//            ui->m_contractAssignee->property( "is_valid" ).toBool() &&
//            ui->m_contractFile->property( "is_valid" ).toBool() &&
//            ui->m_contractFunction->property( "is_valid" ).toBool();
//            ui->m_contractParameters->property( "is_valid" ).toBool();
//    std::cout << "is valid " << ui->m_contractAssignee->property( "is_valid" ).toBool() << " " <<
//                                ui->m_contractFile->property( "is_valid" ).toBool() << " " <<
//                                ui->m_contractFunction->property( "is_valid" ).toBool() << " " <<
//                                ui->m_contractParameters->property( "is_valid" ).toBool() << " "
//    << std::endl;
//    ui->m_contractDeployBtn->setDisabled( !isContractValid );
//}


void MainWin::on_m_streamingTabView_currentChanged(int index)
{
    wizardUpdateStreamAnnouncementTable();
    wizardUpdateArchiveTable();
}


std::string str_toupper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
    return s;
}

void MainWin::startEasyDownload( const DataInfo& dataInfo )
{
    // 'max_metadata_size' should be extended (now it is 30 MB)
    
    //
    // Add easyDownload to table
    //
    m_easyDownloadTableModel->beginResetModel();
    uint64_t& downloadIdRef = m_model->lastUniqueIdOfEasyDownload();
    downloadIdRef++;
    uint64_t downloadId = downloadIdRef;
    auto it = m_model->easyDownloads().insert( m_model->easyDownloads().begin(), EasyDownloadInfo( downloadIdRef, dataInfo) );
    m_easyDownloadTableModel->endResetModel();
    return;

    std::string driveKeyStr = str_toupper(sirius::drive::toString( dataInfo.m_driveKey ));
    
    //
    // create channel
    //
    connect( m_onChainClient->getDialogSignalsEmitter(), &DialogSignals::addDownloadChannel, this, &MainWin::addChannel, Qt::SingleShotConnection );

    const auto channelName = "download-channel7"; //TODO
    
    // this callback receives channel key (tx)
    //
    auto callback = [client=m_onChainClient, downloadId, dataInfo, channelName, driveKeyStr = driveKeyStr,this] (std::string channelKey)
    {
        channelKey = QString::fromStdString(channelKey).toUpper().toStdString();
        qDebug() << "AddDownloadChannelDialog::accept::addChannelHash: " << channelKey.c_str();

        //
        // Save channelKey into EasyDownloadInfo
        //
        auto it = std::find_if( m_model->easyDownloads().begin(), m_model->easyDownloads().end(), [&downloadId](auto& info) {
            return info.m_uniqueId = downloadId;
        });
        assert( it != m_model->easyDownloads().end() );
        it->m_channelKey = channelKey;
        
        //
        // Send tx and wait confirmation of channel creation
        //
        m_model->addChannelFsTreeHandler( [key=channelKey,downloadId,dataInfo,this] ( bool success, const std::string& channelKey, const std::string& driveKey ) -> bool
        {
            qDebug() << "startEasyDownload(callback): downloadId = " << downloadId;

            if ( channelKey != key  )
            {
                qDebug() << "Skip other channels: " <<  channelKey.c_str() << " != " << channelKey.c_str();
                return false;
            }
            
            if ( !success )
            {
                qDebug() << "Channel creation faled: driveKey: " << driveKey.c_str();
                displayError( "Channel creation faled: todo" );
                
                // true - remove handler
                return true;
            }
            
            auto* channel = m_model->findChannel(channelKey);
            assert( channel );
            if ( channel->isCreating() )
            {
                displayError( "channel->isCreating(): todo" );
                qDebug() << "channel->isCreating(): " <<  channelKey.c_str() << " != " << channelKey.c_str();
                return false;
            }

            m_modalDialog->reject();
            delete m_modalDialog;
            m_modalDialog = nullptr;
            
            if ( channel != nullptr )
            {
                continueEasyDownload( downloadId, dataInfo, channelKey );
            }
            else
            {
                displayError( "Internal error: channel not found" );
            }
            
            // true - remove handler
            return true;
        });

        
        emit client->getDialogSignalsEmitter()->addDownloadChannel( channelName, channelKey, driveKeyStr, {});
    };
    
    uint64_t prepaidSizeMB = 2 * (dataInfo.m_totalSize + (1024*1024-1)) / (1024*1024);

    m_onChainClient->addDownloadChannel( channelName,
                                        {}, dataInfo.m_driveKey,
                                        prepaidSizeMB,
                                        0,
                                        callback); // feedback is unused for now

    m_modalDialog = new ModalDialog( "Information", "Channel is creating..." );
    
    m_modalDialog->exec();
}

void MainWin::continueEasyDownload( uint64_t downloadId, const DataInfo& dataInfo, const std::string& channelKey )
{
    auto downloadIt = std::find_if( m_model->easyDownloads().begin(), m_model->easyDownloads().end(), [=] (const auto& downloadInfo)
    {
        qDebug() << "continueEasyDownload: downloadInfo.m_uniqueId = " << downloadInfo.m_uniqueId << " " << downloadId;
        return downloadInfo.m_uniqueId == downloadId;
    });
    
    if ( downloadIt == m_model->easyDownloads().end() )
    {
        qDebug() << "continueEasyDownload: easyDownloadInfo not found: downloadId = " << downloadId;
        return;
    }
    
    auto* channel = m_model->findChannel(channelKey);
    assert( channel != nullptr );

    channel->getFsTree().dbgPrint();
    downloadIt->init( channel->getFsTree() );

    //
    // Check info
    //
    if ( downloadIt->m_downloadingFolder == nullptr && downloadIt->m_downloadingFile == nullptr )
    {
        displayError( "Invalid data link: item not found: " + dataInfo.m_itemName , dataInfo.m_path );
        return;
    }
    
    //
    // Check item size
    //
    if ( dataInfo.m_totalSize < downloadIt->m_size )
    {
        displayError( "Invalid data link: data size is more than stated in link" );
        return;
    }
    

    sirius::drive::ReplicatorList replicatorList = channel->getReplicators();
    qDebug() << "replicatorList: " << replicatorList.size();

    // Request replicator list
    //
    updateReplicatorsForChannel( channel->getKey(), [this, channelKey, downloadId]()
    {
        // Continue download
        //
        auto* channel = m_model->findChannel(channelKey);
        assert( channel != nullptr );

        sirius::drive::ReplicatorList replicatorList = channel->getReplicators();
        qDebug() << "replicatorList2: " << replicatorList.size();

        auto downloadIt = std::find_if( m_model->easyDownloads().begin(), m_model->easyDownloads().end(), [=] (const auto& downloadInfo)
        {
            qDebug() << "continueEasyDownload: downloadInfo.m_uniqueId = " << downloadInfo.m_uniqueId << " " << downloadId;
            return downloadInfo.m_uniqueId == downloadId;
        });
        
        assert( downloadIt != m_model->easyDownloads().end() );
        
        for( auto childIt = downloadIt->m_childs.begin(); childIt != downloadIt->m_childs.end(); childIt++ )
        {
            qDebug() << "childIt->m_path: " << childIt->m_path.c_str() << " : " << childIt->m_fileName.c_str() << " : " << m_model->getDownloadFolder();
            QDir().mkpath(QString::fromStdString( m_model->getDownloadFolder().string() + childIt->m_path ));

            childIt->m_ltHandle = m_model->downloadFile( channel->getKey(), childIt->m_hash );

            DownloadInfo downloadInfo;
            downloadInfo.setHash( childIt->m_hash );
            downloadInfo.setDownloadChannelKey( channel->getKey() );
            downloadInfo.setFileName( childIt->m_fileName );
            downloadInfo.setSaveFolder( childIt->m_path );
            downloadInfo.setDownloadFolder( m_model->getDownloadFolder().string() );
            downloadInfo.setChannelOutdated(false);
            downloadInfo.setCompleted(false);
            downloadInfo.setProgress(0);
            downloadInfo.setHandle( childIt->m_ltHandle );

            m_model->downloads().insert( m_model->downloads().begin(), downloadInfo );
        }
        m_model->saveSettings();
    });
}


//
