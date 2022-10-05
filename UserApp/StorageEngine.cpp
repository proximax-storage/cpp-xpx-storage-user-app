#include "StorageEngine.h"
#include "Settings.h"
#include "mainwin.h""
#include "drive/ClientSession.h"
#include "drive/Session.h"
#include "utils/HexParser.h"

#include <QMetaObject>

#include <boost/algorithm/string.hpp>

void StorageEngine::start()
{
        endpoint_list bootstraps;
        std::vector<std::string> addressAndPort;
        boost::split( addressAndPort, gSettings.config().m_replicatorBootstrap, [](char c){ return c==':'; } );
        bootstraps.push_back( { boost::asio::ip::make_address(addressAndPort[0]),
                               (uint16_t)atoi(addressAndPort[1].c_str()) } );

    if ( !STANDALONE_TEST )
    {
        gStorageEngine->initClientSession( *gSettings.config().m_keyPair,
                                           "0.0.0.0:" + gSettings.config().m_udpPort,
                                           bootstraps );
    }
    else
    {
        gStorageEngine->initClientSession( *gSettings.config().m_keyPair,
                                           "192.168.2.201:2001",
                                           bootstraps );
    }
}

void StorageEngine::restart()
{
    //todo
}

void StorageEngine::initClientSession(  const sirius::crypto::KeyPair&  keyPair,
                                        const std::string&              address,
                                        const endpoint_list&            bootstraps )
{
    if ( STANDALONE_TEST )
    {
        m_session = sirius::drive::createClientSession( keyPair,
                                                        "192.168.2.200:2000",
                                                        []( const lt::alert* ) {},
                                                        bootstraps,
                                                        false,
                                                        "client0" );
        lt::settings_pack pack;
        pack.set_int( lt::settings_pack::download_rate_limit, 500*1024 );
        m_session->setSessionSettings( pack, true );
    }
    else
    {
        m_session = sirius::drive::createClientSession(  keyPair,
                                                         address,
                                                         []( const lt::alert* ) {},
                                                         bootstraps,
                                                         false,
                                                         "client_session" );
    }

    m_session->setTorrentDeletedHandler( [this] ( lt::torrent_handle handle )
    {
        gSettings.onDownloadCompleted( handle );
    });
}

void StorageEngine::downloadFsTree( Settings::ChannelInfo&          channelInfo,
                                    const std::array<uint8_t,32>&   fsTreeHash,
                                    FsTreeHandler                   onFsTreeReceived )
{
    qDebug() << "downloadFsTree(): channelId: " << channelInfo.m_hash.c_str();

    std::array<uint8_t,32> channelId;
    sirius::utils::ParseHexStringIntoContainer( channelInfo.m_hash.c_str(), 64, channelId );

    if ( channelInfo.m_tmpRequestingFsTreeTorrent )
    {
        qDebug() << "downloadFsTree(): channelInfo.m_tmpRequestingFsTreeTorrent ";

        if ( channelInfo.m_tmpRequestingFsTreeHash == fsTreeHash )
        {
            qDebug() << "downloadFsTree(): channelInfo.m_tmpRequestingFsTreeHash == fsTreeHash";
            return;
        }

        // remove previouse request
        std::vector<std::array<uint8_t,32>> torrents;
        torrents.push_back( channelInfo.m_tmpRequestingFsTreeHash );
        m_session->removeTorrents( torrents );
    }

    channelInfo.m_tmpRequestingFsTreeHash = fsTreeHash;

    m_session->addDownloadChannel( channelId );

    qDebug() << "downloadFsTree(): m_session->download(...";
    channelInfo.m_tmpRequestingFsTreeTorrent =
                    m_session->download( sirius::drive::DownloadContext(
                                            sirius::drive::DownloadContext::fs_tree,
                                            [ onFsTreeReceived, driveHash=channelInfo.m_driveHash, fsTreeSaveFolder=settingsFolder()/channelInfo.m_hash ]
                                             ( sirius::drive::download_status::code code,
                                                const sirius::drive::InfoHash& infoHash,
                                                const std::filesystem::path /*filePath*/,
                                                size_t /*downloaded*/,
                                                size_t /*fileSize*/,
                                                const std::string& /*errorText*/)
                                            {
                                                qDebug() << "fstree received: " << std::string(fsTreeSaveFolder);
                                                sirius::drive::FsTree fsTree;
                                                fsTree.deserialize( fsTreeSaveFolder / FS_TREE_FILE_NAME );
                                                onFsTreeReceived( driveHash, infoHash.array(), fsTree );
                                            },
                                            fsTreeHash,
                                            channelId, 0 ),
                                       channelId,
                                       settingsFolder() / channelInfo.m_hash,
                                       "",
                                       {});
}

void StorageEngine::downloadFsTree( Settings::DriveInfo&            driveInfo,
                                    const std::array<uint8_t,32>&   fsTreeHash,
                                    FsTreeHandler                   onFsTreeReceived )
{
    qDebug() << "downloadFsTree(): driveHash: " << driveInfo.m_driveHash;

    std::array<uint8_t,32> channelId;
//    sirius::utils::ParseHexStringIntoContainer( channelInfo.m_hash.c_str(), 64, channelId );

    if ( driveInfo.m_tmpRequestingFsTreeTorrent )
    {
        qDebug() << "downloadFsTree(): channelInfo.m_tmpRequestingFsTreeTorrent ";

        if ( driveInfo.m_tmpRequestingFsTreeHash == fsTreeHash )
        {
            qDebug() << "downloadFsTree(): channelInfo.m_tmpRequestingFsTreeHash == fsTreeHash";
            return;
        }

        // remove previouse request
        std::vector<std::array<uint8_t,32>> torrents;
        torrents.push_back( driveInfo.m_tmpRequestingFsTreeHash );
        m_session->removeTorrents( torrents );
    }

    driveInfo.m_tmpRequestingFsTreeHash = fsTreeHash;

//    m_session->addDownloadChannel( channelId );

    qDebug() << "downloadFsTree(): m_session->download(...";
    driveInfo.m_tmpRequestingFsTreeTorrent =
                    m_session->download( sirius::drive::DownloadContext(
                                            sirius::drive::DownloadContext::fs_tree,
                                            [ onFsTreeReceived, driveHash=driveInfo.m_driveHash, fsTreeSaveFolder=settingsFolder()/driveInfo.m_driveHash ]
                                             ( sirius::drive::download_status::code code,
                                                const sirius::drive::InfoHash& infoHash,
                                                const std::filesystem::path /*filePath*/,
                                                size_t /*downloaded*/,
                                                size_t /*fileSize*/,
                                                const std::string& /*errorText*/)
                                            {
                                                qDebug() << "fstree received: " << std::string(fsTreeSaveFolder);
                                                sirius::drive::FsTree fsTree;
                                                fsTree.deserialize( fsTreeSaveFolder / FS_TREE_FILE_NAME );
                                                onFsTreeReceived( driveHash, infoHash.array(), fsTree );
                                            },
                                            fsTreeHash,
                                            channelId, 0 ),
                                       channelId,
                                       settingsFolder()/driveInfo.m_driveHash,
                                       "",
                                       {});
}


sirius::drive::lt_handle StorageEngine::downloadFile( Settings::ChannelInfo&         channelInfo,
                                                      const std::array<uint8_t,32>&  fileHash )
{
    qDebug() << "downloadFile(): " << sirius::drive::toString(fileHash).c_str();

    std::array<uint8_t,32> channelId;
    sirius::utils::ParseHexStringIntoContainer( channelInfo.m_hash.c_str(), 64, channelId );

    m_session->addDownloadChannel( channelId );

    qDebug() << "downloadFile(): m_session->download(...";
    qDebug() << "downloadFile(): " << sirius::drive::toString(fileHash).c_str();
    auto handle = m_session->download( sirius::drive::DownloadContext(
                                    sirius::drive::DownloadContext::file_from_drive,

                                    []( sirius::drive::download_status::code code,
                                        const sirius::drive::InfoHash& infoHash,
                                        const std::filesystem::path filePath,
                                        size_t downloaded,
                                        size_t fileSize,
                                        const std::string& /*errorText*/)
                                    {
//                                        qDebug() << "file downloaded: " << downloaded << "/" << fileSize << " " << std::string(filePath).c_str();
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
