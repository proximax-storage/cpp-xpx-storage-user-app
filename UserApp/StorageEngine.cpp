#include "StorageEngine.h"
#include "Settings.h"
#include "drive/ClientSession.h"
#include "drive/Session.h"
#include "utils/HexParser.h"

#include <boost/algorithm/string.hpp>

void StorageEngine::start()
{
        endpoint_list bootstraps;
        std::vector<std::string> addressAndPort;
        boost::split( addressAndPort, gSettings.config().m_replicatorBootstrap, [](char c){ return c==':'; } );
        bootstraps.push_back( { boost::asio::ip::make_address(addressAndPort[0]),
                               (uint16_t)atoi(addressAndPort[1].c_str()) } );

#ifndef STANDALONE_TEST
        gStorageEngine->initClientSession( *gSettings.config().m_keyPair,
                                           "0.0.0.0:" + gSettings.config().m_udpPort,
                                           bootstraps );
#else
        gStorageEngine->initClientSession( *gSettings.config().m_keyPair,
                                           "192.168.2.201:2001",
                                           bootstraps );
#endif
}

void StorageEngine::restart()
{
    //todo
}

void StorageEngine::initClientSession(  const sirius::crypto::KeyPair&  keyPair,
                                        const std::string&              address,
                                        const endpoint_list&            bootstraps )
{

#ifdef STANDALONE_TEST
    m_session = sirius::drive::createClientSession( keyPair,
                                                    "192.168.2.200:2000",
                                                    []( const lt::alert* ) {},
                                                    bootstraps,
                                                    false,
                                                    "client0" );
#else
    m_session = sirius::drive::createClientSession(  keyPair,
                                                     address,
                                                     []( const lt::alert* ) {},
                                                     bootstraps,
                                                     false,
                                                     "client_session" );

#endif
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
                                            [onFsTreeReceived,channelId=channelInfo.m_hash]( sirius::drive::download_status::code code,
                                                const sirius::drive::InfoHash& infoHash,
                                                const std::filesystem::path /*filePath*/,
                                                size_t /*downloaded*/,
                                                size_t /*fileSize*/,
                                                const std::string& /*errorText*/)
                                            {
                                                qDebug() << "fstree received: " << std::string(settingsFolder()/channelId.c_str()).c_str();
                                                sirius::drive::FsTree fsTree;
                                                fsTree.deserialize( settingsFolder()/channelId.c_str() / FS_TREE_FILE_NAME );
                                                onFsTreeReceived( channelId, infoHash.array(), fsTree );
                                            },
                                            fsTreeHash,
                                            channelId, 0 ),
                                       channelId,
                                       settingsFolder() / channelInfo.m_hash,
                                       "",
                                       {});
}
