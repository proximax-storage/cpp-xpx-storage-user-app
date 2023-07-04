#include "StorageEngine.h"
#include "mainwin.h"
#include "drive/ViewerSession.h"
#include "drive/ClientSession.h"
#include "drive/Session.h"
#include "utils/HexParser.h"
#include "Model.h"
#include "Settings.h"

#include <QMetaObject>
#include <QMessageBox>

#include <boost/algorithm/string.hpp>

StorageEngine::StorageEngine(Model* model, QObject *parent)
    : QObject(parent)
    , mp_model(model)
{}

sirius::drive::InfoHash StorageEngine::addActions(const sirius::drive::ActionList &actions,
                                                  const sirius::Key &driveId,
                                                  const std::string &sandboxFolder,
                                                  uint64_t &modifySize,
                                                  const std::vector<std::string>& replicators)
{
    auto driveKey = rawHashToHex(driveId.array()).toStdString();
    if (replicators.empty()) {
        qWarning () << "StorageEngine::addActions. Replicators list is empty! Drive key: " << driveKey;
    }

    auto drive = mp_model->findDrive(driveKey);
    if (!drive) {
        qCritical () << "StorageEngine::addActions. Drive not found! Drive key: " << driveKey;
        return {};
    }

    drive->setReplicators(replicators);
    m_session->addReplicatorList( drive->getReplicators() );

    return m_session->addActionListToSession(actions, driveId, drive->getReplicators(), sandboxFolder, modifySize);
}

void StorageEngine::start( std::function<void()> addressAlreadyInUseHandler )
{

    if ( ALEX_LOCAL_TEST )
    {
        endpoint_list bootstraps;
        bootstraps.emplace_back( boost::asio::ip::make_address("192.168.2.101"), 5001 );

        gStorageEngine->init(mp_model->getKeyPair(),
                             "192.168.2.201:2001",
                             bootstraps,
                             addressAlreadyInUseHandler );
    }
    else if ( VICTOR_LOCAL_TEST )
    {
        endpoint_list bootstraps;
        bootstraps.emplace_back( boost::asio::ip::make_address("192.168.20.20"), 7914 );

        gStorageEngine->init(mp_model->getKeyPair(),
                             "192.168.20.30:" + mp_model->getUdpPort(),
                             bootstraps,
                             addressAlreadyInUseHandler );
    }
    else
    {
        endpoint_list bootstraps;
        std::vector<std::string> addressAndPort;
        std::string bootstrapReplicatorEndpoint = mp_model->getBootstrapReplicator();
        boost::split( addressAndPort, bootstrapReplicatorEndpoint, [](char c){ return c==':'; } );
        bootstraps.emplace_back( boost::asio::ip::make_address(addressAndPort[0]),
                               (uint16_t)atoi(addressAndPort[1].c_str()) );

        gStorageEngine->init(mp_model->getKeyPair(),
                             "0.0.0.0:" + mp_model->getUdpPort(),
                             bootstraps,
                             addressAlreadyInUseHandler );
    }
}

void StorageEngine::restart()
{
    //todo
}

void StorageEngine::addReplicators( const sirius::drive::ReplicatorList& replicators)
{
    m_session->addReplicatorList( replicators );
}

void StorageEngine::init(const sirius::crypto::KeyPair&  keyPair,
                         const std::string&              address,
                         const endpoint_list&            bootstraps,
                         std::function<void()>           addressAlreadyInUseHandler )
{
    qDebug() << LOG_SOURCE << "createClientSession: address: " << address.c_str();
    qDebug() << LOG_SOURCE << "createClientSession: bootstraps[0] address: " << bootstraps[0].address().to_string().c_str();
#ifdef USE_CLIENT_SESSION
    m_session = sirius::drive::createClientSession(  keyPair,
#else
    m_session = sirius::drive::createViewerSession(  keyPair,
#endif
                                                     address,
                                                     [addressAlreadyInUseHandler]( const lt::alert* alert )
                                                     {
                                                         qDebug() << LOG_SOURCE << "createClientSession: socketError?" << alert->message();

                                                         addressAlreadyInUseHandler();
                                                     },
                                                     bootstraps,
                                                     false,
                                                     "client_session" );

    m_session->setTorrentDeletedHandler( [this] ( lt::torrent_handle handle )
    {
        mp_model->onDownloadCompleted( handle );
    });

    connect(mp_model, &Model::addTorrentFileToStorageSession, this, &StorageEngine::addTorrentFileToSession, Qt::QueuedConnection);
}

void StorageEngine::downloadFsTree( const std::string&                      driveId,
                                    const std::string&                      dnChannelId,
                                    const std::array<uint8_t,32>&           fsTreeHash )
{
    qDebug() << LOG_SOURCE << "downloadFsTree(): driveHash: " << driveId;

    std::unique_lock<std::recursive_mutex> lock( m_mutex );

    if ( Model::isZeroHash(fsTreeHash) )
    {
        qDebug() << LOG_SOURCE << "zero fstree received";
        emit fsTreeReceived(driveId, fsTreeHash, {});
        return;
    }

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
    sirius::utils::ParseHexStringIntoContainer( dnChannelId.c_str(), 64, channelId );

    m_session->addDownloadChannel( channelId );

    qDebug() << LOG_SOURCE << "downloadFsTree(): m_session->download(...";

    const auto fsTreeHashUpperCase = QString::fromStdString(sirius::drive::toString(fsTreeHash)).toUpper().toStdString();
    auto fsTreeSaveFolder = getFsTreesFolder()/fsTreeHashUpperCase;

    auto notification = [this, driveId, fsTreeSaveFolder](sirius::drive::download_status::code code,
                                                            const sirius::drive::InfoHash& infoHash,
                                                            const std::filesystem::path /*filePath*/,
                                                            size_t /*downloaded*/,
                                                            size_t /*fileSize*/,
                                                            const std::string& /*errorText*/) {
        qDebug() << LOG_SOURCE << "fstree received: " << fsTreeSaveFolder.string();
        sirius::drive::FsTree fsTree;
        try
        {
            fsTree.deserialize( (fsTreeSaveFolder / FS_TREE_FILE_NAME).string() );
        } catch (const std::runtime_error& ex )
        {
            qDebug() << LOG_SOURCE << "Invalid fsTree: " << ex.what();
            fsTree = {};
            fsTree.addFile( {}, std::string("!!! bad FsTree: ") + ex.what(),{},0);
        }

        emit fsTreeReceived(driveId, infoHash.array(), fsTree);
    };

    sirius::drive::DownloadContext downloadContext(sirius::drive::DownloadContext::fs_tree, notification, fsTreeHash, channelId, 0);
    m_session->download(std::move(downloadContext), channelId, fsTreeSaveFolder.string(), "", {}, replicators);
}

sirius::drive::lt_handle StorageEngine::downloadFile( const std::array<uint8_t,32>& channelId,
                                                      const std::array<uint8_t,32>& fileHash)
{
    qDebug() << LOG_SOURCE << "downloadFile(): " << sirius::drive::toString(fileHash).c_str();

    m_session->addDownloadChannel( channelId );

    sirius::drive::ReplicatorList replicators;
    auto channel = mp_model->findChannel(rawHashToHex(channelId).toStdString());
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

    auto handle = m_session->download( sirius::drive::DownloadContext(
                                    sirius::drive::DownloadContext::file_from_drive,

                                    []( sirius::drive::download_status::code code,
                                        const sirius::drive::InfoHash& infoHash,
                                        const std::filesystem::path filePath,
                                        size_t downloaded,
                                        size_t fileSize,
                                        const std::string& /*errorText*/)
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
                       mp_model->getDownloadFolder().string(),
                       "",
                       {},
                       replicators );
    return handle;
}

void StorageEngine::removeTorrentSync( sirius::drive::InfoHash infoHash )
{
    std::vector<std::array<uint8_t,32>> torrents{ infoHash.array() };
    m_session->removeTorrents( torrents );
}

void StorageEngine::torrentDeletedHandler( const sirius::drive::InfoHash& infoHash )
{
}

void StorageEngine::addTorrentFileToSession(const std::string &torrentFilename,
                                            const std::string &folderWhereFileIsLocated,
                                            const std::array<uint8_t, 32>& driveKey,
                                            const std::array<uint8_t, 32> &modifyTx)
{
    qDebug() << "StorageEngine::addTorrentFileToSession. torrentFilename: " << torrentFilename;
    qDebug() << "StorageEngine::addTorrentFileToSession. folderWhereFileIsLocated: " << folderWhereFileIsLocated;
    qDebug() << "StorageEngine::addTorrentFileToSession. driveKey: " << rawHashToHex(driveKey);
    qDebug() << "StorageEngine::addTorrentFileToSession. modifyTx: " << rawHashToHex(modifyTx);

    auto drive = mp_model->findDrive(rawHashToHex(driveKey).toStdString());
    if (!drive) {
        qWarning () << "StorageEngine::addTorrentFileToSession. Drive not found!";
        return;
    }

    m_session->addTorrentFileToSession(torrentFilename, folderWhereFileIsLocated, driveKey, modifyTx, drive->getReplicators());
}

void StorageEngine::requestStreamStatus( const std::array<uint8_t,32>&          driveKey,
                                         const sirius::drive::ReplicatorList&   replicatorList,
                                         StreamStatusResponseHandler            streamStatusResponseHandler )
{
    std::unique_lock<std::recursive_mutex> lock( m_mutex );

#ifndef USE_CLIENT_SESSION
    m_session->requestStreamStatus( driveKey, replicatorList, streamStatusResponseHandler );
#endif
}

