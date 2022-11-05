//#include "moc_mainwin.cpp"

#include "mainwin.h"
#include "./ui_mainwin.h"

#include "Model.h"
#include "Fstreemodel.h"
#include "FsTreeTableModel.h"
#include "DownloadsTableModel.h"
#include "DriveTreeModel.h"
#include "DiffTableModel.h"

#include "Settings.h"
#include "SettingsDialog.h"
#include "PrivKeyDialog.h"
#include "AddDownloadChannelDialog.h"
#include "CloseChannelDialog.h"
#include "AddDriveDialog.h"
#include "CloseDriveDialog.h"
#include "ManageDrivesDialog.h"
#include "ManageChannelsDialog.h"
#include "ProgressBarDialog.h"
#include "OnChainClient.h"

#include "crypto/Signer.h"
#include "utils/HexParser.h"

#include <qdebug.h>
#include <QFileIconProvider>
#include <QScreen>
#include <QComboBox>
#include <QMessageBox>
#include <QDesktopServices>
#include <thread>

MainWin::MainWin(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWin)
{
    ui->setupUi(this);

    if ( Model::homeFolder() != "/Users/alex" )
    {
        ALEX_LOCAL_TEST = false;
    }

    if ( Model::homeFolder() != "/home/????" )
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
        if ( ALEX_LOCAL_TEST )
        {
            gSettings.m_replicatorBootstrap = "192.168.2.101:5001";
        }
        else
        {
            gSettings.m_replicatorBootstrap = "15.206.164.53:7904";
        }
    }

    Model::startStorageEngine();

    setupDownloadsTab();
    setupDrivesTab();

    // Set current download channel
    int dnChannelIndex = Model::currentDnChannelIndex();
    if ( dnChannelIndex >= 0 )
    {
        ui->m_channels->setCurrentIndex( dnChannelIndex );
        onCurrentChannelChanged( dnChannelIndex );
    }

    // Start update-timer for downloads
    m_downloadUpdateTimer = new QTimer();
    connect( m_downloadUpdateTimer, &QTimer::timeout, this, [this] {m_downloadsTableModel->updateProgress();} );
    m_downloadUpdateTimer->start(500); // 2 times per second

    connect( ui->m_settingsBtn, &QPushButton::released, this, [this]
    {
        SettingsDialog settingsDialog( this );
        settingsDialog.exec();
    });

    connect(ui->m_manageChannels, &QPushButton::released, this, [this] () {
        ManageChannelsDialog dialog(this);
        dialog.exec();
    });

    connect(ui->m_manageDrives, &QPushButton::released, this, [this] () {
        ManageDrivesDialog dialog(this);
        dialog.exec();
    });

    const std::string privateKey = gSettings.config().m_privateKeyStr;
    const auto gatewayEndpoint = QString(gSettings.m_restBootstrap.c_str()).split(":");
    const std::string address = gatewayEndpoint[0].toStdString();
    const std::string port = gatewayEndpoint[1].toStdString();

    if ( !ALEX_LOCAL_TEST )
    {
        m_onChainClient = new OnChainClient(gStorageEngine, privateKey, address, port, this);

        connect(ui->m_addChannel, &QPushButton::released, this, [this] () {
            AddDownloadChannelDialog dialog(m_onChainClient, this);
            connect(&dialog, &AddDownloadChannelDialog::addDownloadChannel, this, &MainWin::addChannel);
            dialog.exec();
        });

        //const QStringList driveStructureHeaders = {tr("Name"), tr("Size (Bytes)"), tr("Type"), tr("Date Modified")};
        //m_fsTreeModel->setHeaders(driveStructureHeaders);

        //ui->m_fsTreeTableView->setModel( m_fsTreeModel );
        //ui->m_fsTreeTableView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

        connect(m_onChainClient, &OnChainClient::downloadChannelsLoaded, this, [this](const auto& channels)
        {
            qDebug() << "downloadChannelsLoaded2: " << channels;

            m_onChainClient->getBlockchainEngine()->getDownloadChannelById(channels[0].toStdString(), [this](auto channel, auto isSuccess, auto message, auto code) {
                if (!isSuccess) {
                    qWarning() << "message: " << message.c_str() << " code: " << code.c_str();
                    return;
                }

                Model::onChannelLoaded( channel.data.id, channel.data.drive, channel.data.listOfPublicKeys );
                updateChannelsCBox();

                m_onChainClient->getBlockchainEngine()->getDriveById(channel.data.drive, [this, channel](auto drive, auto isSuccess, auto message, auto code) {
                    if (!isSuccess) {
                        qWarning() << "message: " << message.c_str() << " code: " << code.c_str();
                        return;
                    }

                    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

                    ChannelInfo* channelInfo = Model::findChannel( channel.data.id );
                    if ( channelInfo )
                    {
                        onFsTreeHashReceived( *channelInfo, rawHashFromHex(drive.data.rootHash.c_str()) );
                    }
                });
            });
        });

        connect(m_onChainClient, &OnChainClient::drivesLoaded, this,[this](const auto& drives)
        {
            // add drives created on another devices
            Model::onDrivesLoaded(drives);
            updateDrivesCBox();

            for (int i = 0; i < ui->m_driveCBox->count(); i++) {
                m_onChainClient->loadDownloadChannels(ui->m_driveCBox->itemText(i));
            }
        }, Qt::QueuedConnection);

        connect(m_onChainClient, &OnChainClient::downloadChannelOpenTransactionConfirmed, this, [this](auto alias, auto channelKey, auto driveKey) {
            onChannelCreationConfirmed( alias, sirius::drive::toString( channelKey ), sirius::drive::toString(driveKey) );
        }, Qt::QueuedConnection);

        connect(m_onChainClient, &OnChainClient::downloadChannelOpenTransactionFailed, this, [this](auto& channelKey, auto& errorText) {
               onChannelCreationFailed( channelKey.toStdString(), errorText.toStdString() );
        }, Qt::QueuedConnection);

        connect(ui->m_closeChannel, &QPushButton::released, this, [this] () {
            CloseChannelDialog dialog(m_onChainClient, ui->m_channels->currentText(), this);
            dialog.exec();
        }, Qt::QueuedConnection);

        connect(ui->m_addDrive, &QPushButton::released, this, [this] () {
            AddDriveDialog dialog(m_onChainClient, this);
            dialog.exec();
        });

        connect(ui->m_closeDrive, &QPushButton::released, this, [this] () {
            CloseDriveDialog dialog(m_onChainClient, ui->m_driveCBox->currentText(), this);
            dialog.exec();
        });

        connect(m_onChainClient, &OnChainClient::prepareDriveTransactionConfirmed, this, [this](auto alias, auto driveKey) {
            onDriveCreationConfirmed( alias, sirius::drive::toString( driveKey ) );
        }, Qt::QueuedConnection);

        connect(m_onChainClient, &OnChainClient::prepareDriveTransactionFailed, this, [this](auto alias, auto driveKey, auto errorText) {
            onDriveCreationFailed( alias, sirius::drive::toString( driveKey ), errorText.toStdString() );
        }, Qt::QueuedConnection);

        connect( ui->m_applyChangesBtn, &QPushButton::released, this, [this]
        {
            auto* drive = Model::currentDriveInfoPtr();
            if (drive)
            {
                auto& actionList = drive->m_actionList;
                if ( actionList.empty() )
                {
                    QMessageBox msgBox;
                    msgBox.setText( QString::fromStdString( "There is no differece.") );
                    msgBox.setStandardButtons( QMessageBox::Ok );
                    msgBox.exec();
                    return;
                }

                if (ui->m_channels->count() == 0) {
                    qWarning() << "bad download channel";
                    return;
                }

                auto actions = drive->m_actionList;
                if (actions.empty()) {
                    qWarning() << "no actions to apply";
                    return;
                }

                auto channelId = rawHashFromHex(ui->m_channels->currentText());
                const std::string sandbox = settingsFolder().string() + "/" + drive->m_driveKey + "/modify_drive_data";
                auto driveKeyHex = rawHashFromHex(drive->m_driveKey.c_str());
                m_onChainClient->applyDataModification(driveKeyHex, actions, channelId, sandbox);
            }
        });

        m_onChainClient->loadDrives();
    }

    connect(ui->m_refresh, &QPushButton::released, this, [this]() {
        m_onChainClient->loadDrives();
        ui->m_fsTreeTableView->update();
    });
}

MainWin::~MainWin()
{
    delete ui;
}

void MainWin::setupDownloadsTab()
{
    if ( ALEX_LOCAL_TEST )
    {
        Model::stestInitChannels();
    }

    setupFsTreeTable();
    setupDownloadsTable();
    updateChannelsCBox();

    connect( ui->m_channels, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] (int index)
    {
        qDebug() << index;
        onCurrentChannelChanged( index );
    });

    connect( ui->m_downloadBtn, &QPushButton::released, this, [this] ()
    {
        onDownloadBtn();
    });

//    connect( ui->m_downloadBtn, &QPushButton::released, this, [this] ()
//    {
//        onDownloadBtn();
//    });
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

    if (!m_fsTreeTableModel->m_rows.empty()) {
        ui->m_downloadBtn->setEnabled( ! m_fsTreeTableModel->m_rows[index].m_isFolder );
    }
}

void MainWin::onDownloadBtn()
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto& downloads = Model::downloads();

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

        auto channelId = Model::currentChannelInfoPtr();
        if ( channelId != nullptr )
        {
            auto ltHandle = Model::downloadFile( channelId->m_hash,  hash );
            m_downloadsTableModel->beginResetModel();
            Model::downloads().emplace_back( DownloadInfo{ hash, channelId->m_hash, name,
                                                           Model::downloadFolder(),
                                                           false, 0, ltHandle } );
            m_downloadsTableModel->m_selectedRow = int(Model::downloads().size()) - 1;
            m_downloadsTableModel->endResetModel();
        }
    }
}

void MainWin::setupDownloadsTable()
{
    m_downloadsTableModel = new DownloadsTableModel( this, [this](int row) { ui->m_downloadsTableView->selectRow(row); });

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

        qDebug() << "removeDownloadBtn pressed: " << ui->m_downloadsTableView->selectionModel()->selectedRows();

        auto rows = ui->m_downloadsTableView->selectionModel()->selectedRows();

        if ( rows.size() <= 0 )
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
            msgBox.setText( QString::fromStdString( "'" + dnInfo.m_fileName + "' will be removed.") );
            msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
            auto reply = msgBox.exec();

            if ( reply == QMessageBox::Ok )
            {
                Model::removeFromDownloads( rowIndex );
            }
            selectDownloadRow(-1);
        }
    });

    ui->m_downloadsTableView->setModel( m_downloadsTableModel );
    ui->m_downloadsTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_downloadsTableView->horizontalHeader()->hide();
    ui->m_downloadsTableView->setGridStyle( Qt::NoPen );
    ui->m_downloadsTableView->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->m_downloadsTableView->setSelectionMode( QAbstractItemView::SingleSelection );

    //ui->m_downloadsTableView->update();
    ui->m_removeDownloadBtn->setEnabled( false );
}

void MainWin::selectDownloadRow( int row )
{
    //qDebug() << "selectDownloadRow: " << row;
    m_downloadsTableModel->m_selectedRow = row;
    ui->m_downloadsTableView->selectRow( row );
    ui->m_removeDownloadBtn->setEnabled( row >= 0 );
}

bool MainWin::requestPrivateKey()
{
    {
        PrivKeyDialog pKeyDialog( this );

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

void MainWin::updateChannelsCBox()
{
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

    if ( Model::currentChannelInfoPtr() == nullptr &&  Model::dnChannels().size() > 0 )
    {
        onCurrentChannelChanged(0);
    }

    ui->m_channels->setCurrentIndex( Model::currentDnChannelIndex() );
}

void MainWin::addChannel( const std::string&              channelName,
                          const std::string&              channelKey,
                          const std::string&              driveKey,
                          const std::vector<std::string>& allowedPublicKeys )
{
    auto& channels = Model::dnChannels();
    auto creationTime = std::chrono::steady_clock::now();
    channels.emplace_back( ChannelInfo{ channelName,
                                        channelKey,
                                        driveKey,
                                        allowedPublicKeys,
                                        true, false, // isCreating, isDeleting
                                        creationTime
                           } );

    Model::setCurrentDnChannelIndex( (int)channels.size()-1 );

    updateChannelsCBox();
}

void MainWin::onChannelCreationConfirmed( const std::string& alias, const std::string& channelKey, const std::string& driveKey )
{
    ChannelInfo channelInfo;
    channelInfo.m_name = alias;
    channelInfo.m_hash = channelKey;
    channelInfo.m_driveHash = driveKey;

    QMessageBox msgBox;
    msgBox.setText( QString::fromStdString( "Channel '" + alias + "' created successfully.") );
    msgBox.setStandardButtons( QMessageBox::Ok );
    msgBox.exec();

    channelInfo.m_isCreating = false;

    Model::dnChannels().push_back(channelInfo);
    updateChannelsCBox();
}

void MainWin::onChannelCreationFailed( const std::string& channelKey, const std::string& errorText )
{
    auto* channelInfo = Model::findChannel( channelKey );

    QMessageBox msgBox;
    msgBox.setText( QString::fromStdString( "Channel creation failed (" + channelInfo->m_name + ")\n It will be removed.") );
    msgBox.setInformativeText( QString::fromStdString( errorText ) );
    msgBox.setStandardButtons( QMessageBox::Ok );
    msgBox.exec();

    //todo!!!
}

void MainWin::onDriveCreationConfirmed(const std::string &alias, const std::string &driveKey) {
    DriveInfo driveInfo;
    driveInfo.m_name = alias;
    driveInfo.m_driveKey = driveKey;

    QMessageBox msgBox;
    msgBox.setText( QString::fromStdString( "Drive '" + alias + "' created successfully.") );
    msgBox.setStandardButtons( QMessageBox::Ok );
    msgBox.exec();

    driveInfo.m_isCreating = false;

    Model::drives().push_back(driveInfo);
    updateDrivesCBox();
}

void MainWin::onDriveCreationFailed(const std::string& alias, const std::string& driveKey, const std::string& errorText) {
    QMessageBox msgBox;
    msgBox.setText( QString::fromStdString( "Drive creation failed (" + alias + ")\n It will be removed.") );
    msgBox.setInformativeText( QString::fromStdString( errorText ) );
    msgBox.setStandardButtons( QMessageBox::Ok );
    msgBox.exec();

    //todo!!!
}

void MainWin::onCurrentChannelChanged( int index )
{
    Model::setCurrentDnChannelIndex( index );

    if ( ALEX_LOCAL_TEST )
    {
        Model::currentChannelInfoPtr()->m_waitingFsTree = true;

        std::thread( [this,index]
        {
            qDebug() << "onChannelChanged: " << index;

            std::array<uint8_t,32> fsTreeHash;

            if ( index == 0 )
            {
                sirius::utils::ParseHexStringIntoContainer( ROOT_HASH1,
                                                            64, fsTreeHash );
            }
            else if ( index == 1 )
            {
                sirius::utils::ParseHexStringIntoContainer( ROOT_HASH2,
                                                            64, fsTreeHash );
            }

            sleep(1);
            onFsTreeHashReceived( Model::dnChannels()[index], fsTreeHash );
        }).detach();
    }

    m_fsTreeTableModel->setFsTree( {}, {} );
}

void MainWin::onFsTreeHashReceived( const ChannelInfo& channel, const std::array<uint8_t,32>& fsTreeHash )
{
    qDebug() << "onFsTreeHashReceived: " << sirius::drive::toString(fsTreeHash).c_str();

    std::lock_guard<std::recursive_mutex> lock( gSettingsMutex );
    Model::downloadFsTree( channel.m_driveHash,
                           channel.m_hash,
                           fsTreeHash,
                           [this] ( const std::string&  driveHash,
                                    const std::array<uint8_t,32> fsTreeHash,
                                    const sirius::drive::FsTree& fsTree )
    {
        qDebug() << "m_fsTreeTableModel->setFsTree( fsTree, {} );";

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        auto channelInfo = Model::currentChannelInfoPtr();
        if ( channelInfo != nullptr && channelInfo->m_driveHash == driveHash )
        {
            channelInfo->m_tmpRequestingFsTreeTorrent.reset();
            channelInfo->m_waitingFsTree = false;
            lock.unlock();

            m_fsTreeTableModel->setFsTree( fsTree, {} );
        }

//        connect(this, &MainWin::updateFsTree, m_fsTreeTableModel, [this, fsTree](){
            m_fsTreeTableModel->setFsTree( fsTree, {} );
//        });

//        emit updateFsTree();
    });
}

void MainWin::setupDrivesTab()
{
    //ui->m_calcDiffBtn->setEnabled(false);

    connect( ui->m_openLocalFolderBtn, &QPushButton::released, this, [this]
    {
        qDebug() << "openLocalFolderBtn";

        auto* driveInfo = Model::currentDriveInfoPtr();
        if ( driveInfo != nullptr )
        {
            qDebug() << "driveInfo->m_localDriveFolder: " << driveInfo->m_localDriveFolder.c_str();
            std::error_code ec;
            if ( ! fs::exists(driveInfo->m_localDriveFolder, ec) )
            {
                fs::create_directories( driveInfo->m_localDriveFolder, ec );
            }
//            QDesktopServices::openUrl( QString::fromStdString( "file://" + driveInfo->m_localDriveFolder));
            QDesktopServices::openUrl( QUrl::fromLocalFile( QString::fromStdString( driveInfo->m_localDriveFolder )));
        }
    });

    connect( ui->m_calcDiffBtn, &QPushButton::released, this, [this]
    {
        Model::calcDiff();
        m_driveTreeModel->updateModel();
        ui->m_driveTreeView->expandAll();
        m_diffTableModel->updateModel();
    });

    if ( ALEX_LOCAL_TEST )
    {
        Model::stestInitDrives();
    }

    updateDrivesCBox();

    // request FsTree info-hash
    if ( ALEX_LOCAL_TEST )
    {
        auto* driveInfoPtr = Model::currentDriveInfoPtr();
        std::array<uint8_t,32> fsTreeHash;
        sirius::utils::ParseHexStringIntoContainer( ROOT_HASH1,
                                                        64, fsTreeHash );

        qDebug() << "driveHash: " << driveInfoPtr->m_driveKey.c_str();

        Model::downloadFsTree( driveInfoPtr->m_driveKey,
                               "0000000000000000000000000000000000000000000000000000000000000000",
                               fsTreeHash,
                               [this] ( const std::string&           driveHash,
                                        const std::array<uint8_t,32> fsTreeHash,
                                        const sirius::drive::FsTree& fsTree )
        {
            Model::onFsTreeForDriveReceived( driveHash, fsTreeHash, fsTree );

            qDebug() << "driveHash: " << driveHash.c_str();

            if ( driveHash == Model::currentDriveInfoPtr()->m_driveKey )
            {
                qDebug() << "m_calcDiffBtn->setEnabled: " << driveHash.c_str();
                ui->m_calcDiffBtn->setEnabled(true);
            }
        });
    }

    m_driveTreeModel = new DriveTreeModel();
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
    ui->m_diffTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_diffTableView->horizontalHeader()->hide();
    ui->m_diffTableView->horizontalHeader()->resizeSection(0, 300);
    ui->m_diffTableView->horizontalHeader()->resizeSection(1, 300);
    ui->m_diffTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    //ui->m_diffTableView->setGridStyle( Qt::NoPen );
}

void MainWin::updateDrivesCBox()
{
    ui->m_driveCBox->clear();
    for( const auto& channelInfo: Model::drives() )
    {
        ui->m_driveCBox->addItem( QString::fromStdString( channelInfo.m_name) );
    }

    if ( Model::currentDriveInfoPtr() == nullptr &&  Model::drives().size() > 0 )
    {
        Model::setCurrentDriveIndex(0);
    }

    ui->m_driveCBox->setCurrentIndex( Model::currentDriveIndex() );
}

