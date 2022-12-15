#include "StorageEngine.h"
#include "TransactionsEngine.h"
#include "Settings.h"
#include "mainwin.h"
#include "drive/ClientSession.h"
#include "drive/Session.h"
#include "utils/HexParser.h"

#include <QMetaObject>
#include <QMessageBox>

#include <boost/algorithm/string.hpp>

StorageEngine::StorageEngine(QObject *parent)
    : QObject(parent)
{}

sirius::drive::InfoHash StorageEngine::addActions(const sirius::drive::ActionList &actions,
                                                  const sirius::Key &driveId,
                                                  const std::string &sandboxFolder,
                                                  uint64_t &modifySize) {
    return m_session->addActionListToSession(actions, driveId, {}, sandboxFolder, modifySize);
}

void StorageEngine::start( std::function<void()> addressAlreadyInUseHandler )
{

    if ( ALEX_LOCAL_TEST )
    {
        endpoint_list bootstraps;
        bootstraps.push_back( { boost::asio::ip::make_address("192.168.2.101"), 5001 } );

        gStorageEngine->init(*gSettings.config().m_keyPair,
                             "192.168.2.201:2001",
                             bootstraps,
                             addressAlreadyInUseHandler );
    }
    else if ( VICTOR_LOCAL_TEST )
    {
        endpoint_list bootstraps;
        bootstraps.push_back( { boost::asio::ip::make_address("192.168.20.20"), 7914 } );

        gStorageEngine->init(*gSettings.config().m_keyPair,
                             "192.168.20.30:" + gSettings.m_udpPort,
                             bootstraps,
                             addressAlreadyInUseHandler );
    }
    else
    {
        endpoint_list bootstraps;
        std::vector<std::string> addressAndPort;
        boost::split( addressAndPort, gSettings.m_replicatorBootstrap, [](char c){ return c==':'; } );
        bootstraps.push_back( { boost::asio::ip::make_address(addressAndPort[0]),
                               (uint16_t)atoi(addressAndPort[1].c_str()) } );

        gStorageEngine->init(*gSettings.config().m_keyPair,
                             "0.0.0.0:" + gSettings.m_udpPort,
                             bootstraps,
                             addressAlreadyInUseHandler );
    }
}

void StorageEngine::restart()
{
    //todo
}

void StorageEngine::init(const sirius::crypto::KeyPair&  keyPair,
                         const std::string&              address,
                         const endpoint_list&            bootstraps,
                         std::function<void()>           addressAlreadyInUseHandler )
{
    qDebug() << LOG_SOURCE << "createClientSession: address: " << address.c_str();
    qDebug() << LOG_SOURCE << "createClientSession: bootstraps[0] address: " << bootstraps[0].address().to_string().c_str();
    m_session = sirius::drive::createClientSession(  keyPair,
                                                     address,
                                                     [addressAlreadyInUseHandler]( const lt::alert* alert )
                                                     {
                                                         qDebug() << LOG_SOURCE << "createClientSession: socketError?" << alert->message();

                                                         addressAlreadyInUseHandler();
                                                     },
                                                     bootstraps,
                                                     false,
                                                     "client_session" );

    m_session->setTorrentDeletedHandler( [] ( lt::torrent_handle handle )
    {
        gSettings.onDownloadCompleted( handle );
    });
}

void StorageEngine::downloadFsTree( const std::string&              driveHash,
                                    const std::string&              dnChannelId,
                                    const std::array<uint8_t,32>&   fsTreeHash,
                                    FsTreeHandler                   onFsTreeReceived )
{
    qDebug() << LOG_SOURCE << "downloadFsTree(): driveHash: " << driveHash;

    std::unique_lock<std::recursive_mutex> lock( m_mutex );

    if ( Model::isZeroHash(fsTreeHash) )
    {
        qDebug() << LOG_SOURCE << "zero fstree received";
        onFsTreeReceived( driveHash, fsTreeHash, {} );
        return;
    }

    std::array<uint8_t,32> channelId;
    sirius::utils::ParseHexStringIntoContainer( dnChannelId.c_str(), 64, channelId );

//    qDebug() << LOG_SOURCE << "downloadFsTree(): 1";
    m_session->addDownloadChannel( channelId );

    qDebug() << LOG_SOURCE << "downloadFsTree(): m_session->download(...";

    auto fsTreeSaveFolder = fsTreesFolder()/sirius::drive::toString(fsTreeHash);

    //TODO:  !!!
     endpoint_list epList;
     boost::asio::ip::address e1 = boost::asio::ip::make_address("54.251.236.214");
     epList.push_back( {e1,7904} );

//    auto tmpRequestingFsTreeTorrent =
                    m_session->download( sirius::drive::DownloadContext(
                                            sirius::drive::DownloadContext::fs_tree,

                                            [ onFsTreeReceived, driveHash, fsTreeSaveFolder ]
                                             ( sirius::drive::download_status::code code,
                                                const sirius::drive::InfoHash& infoHash,
                                                const std::filesystem::path /*filePath*/,
                                                size_t /*downloaded*/,
                                                size_t /*fileSize*/,
                                                const std::string& /*errorText*/)
                                            {
                                                qDebug() << LOG_SOURCE << "fstree received: " << std::string(fsTreeSaveFolder);
                                                sirius::drive::FsTree fsTree;
                                                try
                                                {
                                                    fsTree.deserialize( fsTreeSaveFolder / FS_TREE_FILE_NAME );
                                                } catch (const std::runtime_error& ex )
                                                {
                                                    qDebug() << LOG_SOURCE << "Invalid fsTree: " << ex.what();
                                                    fsTree = {};
                                                    fsTree.addFile( {}, std::string("!!! bad FsTree: ") + ex.what(),{},0);
                                                }
                                                onFsTreeReceived( driveHash, infoHash.array(), fsTree );
                                            },
                                            fsTreeHash,
                                            channelId, 0),
                                       channelId,
                                       fsTreeSaveFolder,
                                       "",
                                       epList);
                                       //{});
}

sirius::drive::lt_handle StorageEngine::downloadFile( const std::array<uint8_t,32>& channelId,
                                                      const std::array<uint8_t,32>& fileHash )
{
    qDebug() << LOG_SOURCE << "downloadFile(): " << sirius::drive::toString(fileHash).c_str();

    m_session->addDownloadChannel( channelId );

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
                       gSettings.downloadFolder(),
                       "",
                       {});
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
