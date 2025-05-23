#include "StorageEngine.h"
#include "mainwin.h"
#ifndef WA_APP
#include "drive/ViewerSession.h"
#include "drive/ClientSession.h"
#include "drive/StreamerSession.h"
#include "drive/Session.h"
#endif
#include "utils/HexParser.h"
#include "Models/Model.h"
#include "Entities/Settings.h"
#include "Utils.h"

#include <QMetaObject>
#include <QMessageBox>

#include <boost/algorithm/string.hpp>

StorageEngine::StorageEngine(Model* model, QObject *parent)
    : QObject(parent)
    , mp_model(model)
{}

sirius::drive::InfoHash StorageEngine::addActions(const sirius::drive::ActionList &actions,
                                                  const sirius::Key &driveId,
                                                  const QString &sandboxFolder,
                                                  uint64_t &modifySize,
                                                  const std::vector<QString>& replicators)
{
    auto driveKey = rawHashToHex(driveId.array());
    qDebug() << "StorageEngine::addActions. Drive key: " << driveKey << " Path to sandbox: " << sandboxFolder;
    if (replicators.empty()) {
        qWarning () << "StorageEngine::addActions. Replicators list is empty! Drive key: " << driveKey;
    }

    auto drive = mp_model->findDrive(driveKey);
    if (!drive) {
        qCritical () << "StorageEngine::addActions. Drive not found! Drive key: " << driveKey;
        return {};
    }

    drive->setReplicators(replicators);

#ifdef WA_APP
//TODO_WA
    sirius::drive::InfoHash hash;
    return hash;
#else
    m_session->addReplicatorList( drive->getReplicators() );

    std::error_code ec;
    m_lastModifySize = 0;
    auto hash = m_session->addActionListToSession(actions, driveId, drive->getReplicators(), qStringToStdStringUTF8(sandboxFolder), modifySize, {}, ec);
    if (ec) {
        qCritical () << "StorageEngine::addActionListToSession. Error: " << ec.message().c_str() << " code: " << ec.value();
        emit newError(ErrorType::Storage, ec.message().c_str());
    }
    
    qDebug() << "modifySize: " << modifySize;
    m_lastModifySize = modifySize;

    return hash;
#endif
}

void StorageEngine::start()
{
    auto errorsCallback = [this]()
    {
        emit newError(ErrorType::Storage , "Port already in use: ");
#ifdef WA_APP
        //TODO_WA
#else
        m_session->stop();
#endif
    };

    auto settings= mp_model->getSettings();
    auto isHostMatched = [this](std::tuple<QString, QString, QString> item)
    {
        const auto host = mp_model->getBootstrapReplicator();
        const auto ip = host.split(":");

        return ip.size() == 2 && std::get<0>(item).toStdString() == ip[0].toStdString();
    };

    endpoint_list bootstraps;
    if (std::find_if( settings->MAINNET_REPLICATORS.begin(),settings->MAINNET_REPLICATORS.end(), isHostMatched) != settings->MAINNET_REPLICATORS.end()) {
        for (auto& [host, port, ip] : settings->MAINNET_REPLICATORS) {
            bootstraps.emplace_back(boost::asio::ip::make_address(ip.toStdString()), port.toUShort());
        }
    } else if (std::find_if( settings->TESTNET_REPLICATORS.begin(),settings->TESTNET_REPLICATORS.end(), isHostMatched) != settings->TESTNET_REPLICATORS.end()) {
        for (auto& [host, port, ip] : settings->TESTNET_REPLICATORS) {
            bootstraps.emplace_back(boost::asio::ip::make_address(ip.toStdString()), port.toUShort());
        }
    } else {
        QString bootstrapReplicator = mp_model->getBootstrapReplicator();
        if (!isResolvedToIpAddress(bootstrapReplicator)) {
            emit newError(ErrorType::NetworkInit , "Cannot resolve host(3): " + bootstrapReplicator);
            return;
        }

        qDebug() << "StorageEngine::start: REPLICATORS bootstrap ip addresses: " << bootstrapReplicator;

        const auto host = bootstrapReplicator.split(":");
        const auto ip = host[0].toStdString();
        const auto port = host[1].toUShort();
        bootstraps.emplace_back(boost::asio::ip::make_address(ip), port);
    }

    QString ltAddress = "0.0.0.0:";
#ifdef __APPLE__
    QString bootstrapReplicator = mp_model->getBootstrapReplicator();
    if (!isResolvedToIpAddress(bootstrapReplicator)) {
        emit newError(ErrorType::NetworkInit , "Cannot resolve host(3): " + bootstrapReplicator);
        return;
    }

    const auto host = bootstrapReplicator.split(":");
    const auto ip = host[0].toStdString();
    if ( ip.size() > 7 && ip.substr(0,7) == "192.168")
    {
        ltAddress = "192.168.20.30:";
    }
#endif
    gStorageEngine->init(mp_model->getKeyPair(), ltAddress + mp_model->getUdpPort(), bootstraps, errorsCallback );
}

void StorageEngine::restart()
{
#ifdef WA_APP
//TODO_WA
#else
    m_session->stop();
    m_session.reset();
#endif
}

void StorageEngine::addReplicators( const sirius::drive::ReplicatorList& replicators)
{
#ifdef WA_APP
//TODO_WA
#else
    m_session->addReplicatorList( replicators );
#endif
}

void StorageEngine::streamStatusHandler( const std:: string&)
{

}

void StorageEngine::init(const sirius::crypto::KeyPair&  keyPair,
                         QString                         address,
                         const endpoint_list&            bootstraps,
                         std::function<void()>           addressAlreadyInUseHandler )
{
    qDebug() <<"StorageEngine::init createClientSession: address: " << address;

    QString endpoints;
    std::vector<sirius::drive::ReplicatorInfo> bootstrapReplicators;
    for (const auto& endpoint : bootstraps) {
        bootstrapReplicators.push_back({ endpoint, {} });
        endpoints.append(endpoint.address().to_string().c_str());
        endpoints.append(":");
        endpoints.append(QString::number(endpoint.port()));
        endpoints.append(" ");
    }

    qDebug() << "StorageEngine::init createClientSession: replicators bootstrap addresses: " << endpoints;

#ifdef WA_APP
//TODO_WA
#else

#ifdef USE_CLIENT_SESSION
    m_session = sirius::drive::createClientSession(  keyPair,
#else
    m_session = sirius::drive::createViewerSession(  keyPair,
#endif
                                                     address.toStdString(),
                                                     [addressAlreadyInUseHandler]( const lt::alert* alert )
                                                     {
                                                         qDebug() << LOG_SOURCE << "createClientSession: socketError?" << alert->message().c_str();

                                                         addressAlreadyInUseHandler();
                                                     },
                                                     bootstrapReplicators,
                                                     "client_session" );

    m_session->setTorrentDeletedHandler( [this] ( lt::torrent_handle handle )
    {
        mp_model->onDownloadCompleted( handle );
    });
#endif

    connect(mp_model, &Model::addTorrentFileToStorageSession, this, &StorageEngine::addTorrentFileToSession, Qt::QueuedConnection);
}

void StorageEngine::downloadFsTree( const QString&                      driveId,
                                    const QString&                      dnChannelId,
                                    const std::array<uint8_t,32>&       fsTreeHash )
{
    qDebug() << "StorageEngine::downloadFsTree. DownloadFsTree(): driveHash: " << driveId;

    std::unique_lock<std::recursive_mutex> lock( m_mutex );

    if ( Model::isZeroHash(fsTreeHash) )
    {
        qDebug() << "StorageEngine::downloadFsTree. Zero fstree received";
        emit fsTreeReceived(driveId, fsTreeHash, {});
        return;
    }

#ifdef WA_APP
//TODO_WA
#else
    sirius::drive::ReplicatorList replicators;
    auto drive = mp_model->findDrive(driveId);
    if (drive) {
        replicators = drive->getReplicators();
    } else {
        auto channel = mp_model->findChannel(dnChannelId);
        if (channel) {
            replicators = channel->getReplicators();
        } else {
            qWarning () << "StorageEngine::downloadFile. Download channel not found! Channel key: " << dnChannelId;
        }
    }

    if (replicators.empty()) {
        qWarning () << "StorageEngine::downloadFsTree. Replicators list is empty! Channel key: " << dnChannelId;
    } else {
        m_session->addReplicatorList( replicators );
    }

    std::array<uint8_t,32> channelId{ 0 };
    sirius::utils::ParseHexStringIntoContainer( dnChannelId.toStdString().c_str(), 64, channelId );

    m_session->addDownloadChannel( channelId );

    qDebug() << "StorageEngine::downloadFsTree. downloadFsTree(): m_session->download(...";

    const auto fsTreeHashUpperCase = QString::fromStdString(sirius::drive::toString(fsTreeHash)).toUpper().toStdString();
    auto fsTreeSaveFolder = getFsTreesFolder() / fsTreeHashUpperCase;

    auto notification = [this, driveId, fsTreeSaveFolder](sirius::drive::download_status::code code,
                                                            const sirius::drive::InfoHash& infoHash,
                                                            const std::filesystem::path /*filePath*/,
                                                            size_t /*downloaded*/,
                                                            size_t /*fileSize*/,
                                                            const std::string& /*errorText*/) {
        qDebug() <<"StorageEngine::downloadFsTree. fstree received: " << fsTreeSaveFolder.string().c_str();
        sirius::drive::FsTree fsTree;
        try
        {
            fsTree.deserialize( (fsTreeSaveFolder / FS_TREE_FILE_NAME).make_preferred() );
        } catch (const std::runtime_error& ex )
        {
            qDebug() << "StorageEngine::downloadFsTree. Invalid fsTree: " << ex.what();
            fsTree = {};
            fsTree.addFile( {}, std::string("!!! bad FsTree: ") + ex.what(),{},0);
        }

        emit fsTreeReceived(driveId, infoHash.array(), fsTree);
    };

    sirius::drive::DownloadContext downloadContext(sirius::drive::DownloadContext::fs_tree, notification, fsTreeHash, channelId, 0);
    m_session->download(std::move(downloadContext), channelId, fsTreeSaveFolder, {}, {}, replicators);
#endif
}

sirius::drive::lt_handle StorageEngine::downloadFile( const std::array<uint8_t,32>& channelId,
                                                      const std::array<uint8_t,32>& fileHash,
                                                      std::filesystem::path         outputFolder )
{
    qDebug() << LOG_SOURCE << "downloadFile(): " << sirius::drive::toString(fileHash).c_str();

#ifdef WA_APP
//TODO_WA
    sirius::drive::lt_handle handle;
    return handle;
#else
    m_session->addDownloadChannel( channelId );

    sirius::drive::ReplicatorList replicators;
    auto channel = mp_model->findChannel(rawHashToHex(channelId));
    if (!channel) {
        qWarning () << "StorageEngine::downloadFile. Download channel not found! Channel key: " << rawHashToHex(channelId);
    } else {
        replicators = channel->getReplicators();
    }

    if (replicators.empty()) {
        qWarning () << "StorageEngine::downloadFile. Replicators list is empty! Channel Key: " << rawHashToHex(channelId);
    } else {
        m_session->addReplicatorList( replicators );
    }

    qDebug() << LOG_SOURCE << "downloadFile(): m_session->download(...";
    qDebug() << LOG_SOURCE << "downloadFile(): " << sirius::drive::toString(fileHash).c_str();

    if ( outputFolder.empty() )
    {
        outputFolder = qStringToStdStringUTF8(mp_model->getDownloadFolder());
    }
        
    auto handle = m_session->download( sirius::drive::DownloadContext(
                                    sirius::drive::DownloadContext::file_from_drive,

                                    [=]( sirius::drive::download_status::code code,
                                        const sirius::drive::InfoHash& infoHash,
                                        const std::filesystem::path filePath,
                                        size_t downloaded,
                                        size_t fileSize,
                                        const std::string& errorText )
                                    {
//                                        qDebug() << LOG_SOURCE << "file downloaded: " << downloaded << "/" << fileSize << " " << std::string(filePath).c_str();
//                                        QMetaObject::invokeMethod( &MainWin::instanse(), "onDownloadCompleted", Qt::QueuedConnection,
//                                            Q_ARG( QString, QString::fromStdString( sirius::drive::toString(infoHash) )));
                                    },

                                    fileHash,
                                    {},//channelId,
                                    0, false,
                                    ""
                                ),
                       channelId,
                       outputFolder.make_preferred(),
                       {},
                       {},
                       replicators );
    return handle;
#endif
}

void StorageEngine::removeTorrentSync( sirius::drive::InfoHash infoHash )
{
#ifdef WA_APP
//TODO_WA
#else
    std::vector<std::array<uint8_t,32>> torrents{ infoHash.array() };
    m_session->removeTorrents( torrents );
#endif
}

void StorageEngine::torrentDeletedHandler( const sirius::drive::InfoHash& infoHash )
{
}

void StorageEngine::addTorrentFileToSession(const QString &torrentFilename,
                                            const QString &folderWhereFileIsLocated,
                                            const std::array<uint8_t, 32>& driveKey,
                                            const std::array<uint8_t, 32> &modifyTx)
{
    qDebug() << "StorageEngine::addTorrentFileToSession. torrentFilename: " << torrentFilename;
    qDebug() << "StorageEngine::addTorrentFileToSession. folderWhereFileIsLocated: " << folderWhereFileIsLocated;
    qDebug() << "StorageEngine::addTorrentFileToSession. driveKey: " << rawHashToHex(driveKey);
    qDebug() << "StorageEngine::addTorrentFileToSession. modifyTx: " << rawHashToHex(modifyTx);

#ifdef WA_APP
//TODO_WA
#else
    auto drive = mp_model->findDrive(rawHashToHex(driveKey));
    if (!drive) {
        qWarning () << "StorageEngine::addTorrentFileToSession. Drive not found!";
        return;
    }

    m_session->addTorrentFileToSession(qStringToStdStringUTF8(torrentFilename),
                                       qStringToStdStringUTF8(folderWhereFileIsLocated),
                                       driveKey, modifyTx, drive->getReplicators());
#endif
}

void StorageEngine::requestStreamStatus( const std::array<uint8_t,32>&          driveKey,
                                         const sirius::drive::ReplicatorList&   replicatorList,
                                         StreamStatusResponseHandler            streamStatusResponseHandler )
{
#ifdef WA_APP
//TODO_WA
#else
    std::unique_lock<std::recursive_mutex> lock( m_mutex );

#ifndef USE_CLIENT_SESSION
    m_session->requestStreamStatus( driveKey, replicatorList, streamStatusResponseHandler );
#endif

#endif
}
                                                   
std::optional<boost::asio::ip::udp::endpoint> StorageEngine::getEndpoint( const sirius::Key& key )
{
#ifdef WA_APP
//TODO_WA
    return std::optional<boost::asio::ip::udp::endpoint>();
#else
    return m_session->getEndpoint( key.array() );
#endif
}

void StorageEngine::startStreaming( const sirius::Hash256&  streamId,
                                    const QString&          streamFolderName,
                                    const sirius::Key&      driveKey,
                                    const fs::path&         m3u8Playlist,
                                    const fs::path&         driveLocalFolder,
                                    const fs::path&         torrentLocalFolder,
                                    sirius::drive::StreamingStatusHandler streamingStatusHandler,
                                    const endpoint_list&    endPointList )
{
#ifdef WA_APP
//TODO_WA
#else
    m_session->initStream( streamId, qStringToStdStringUTF8(streamFolderName), driveKey, m3u8Playlist, driveLocalFolder, torrentLocalFolder, streamingStatusHandler, endPointList );
#endif
}

void StorageEngine::finishStreaming( const QString& driveKey, std::function<void(const sirius::Key& driveKey, const sirius::drive::InfoHash& streamId, const sirius::drive::InfoHash& actionListHash, uint64_t streamBytes)> backCall )
{
#ifdef WA_APP
//TODO_WA
#else
    if ( auto drive = mp_model->findDrive(driveKey); drive != nullptr )
    {
        auto replicators = drive->getReplicators();

        if ( replicators.empty() )
        {
            qCritical() << "StorageEngine::finishStreaming. Replicators list is empty! Channel key: " << driveKey;
            return;
        }
        else
        {
            m_session->addReplicatorList( replicators );
        }
        auto pathToSandbox = getSettingsFolder().string() + "/" + driveKey.toStdString() + CLIENT_SANDBOX_FOLDER;
        m_session->finishStream( backCall, replicators, pathToSandbox, {} );
    }
    else
    {
        qWarning () << "StorageEngine::finishStreaming. Replicators list is empty! Channel key: " << driveKey;
    }
#endif
}
                                                   
void StorageEngine::cancelStreaming()
{
#ifdef WA_APP
//TODO_WA
#else
    m_session->cancelStream();
#endif
}


void StorageEngine::modificationHasBeenRegistered( const sirius::drive::ReplicatorList& list )
{
        m_session->modificationHasBeenRegistered( list );
}
