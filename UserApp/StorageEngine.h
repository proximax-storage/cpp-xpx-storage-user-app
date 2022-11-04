#ifndef STORAGEENGINE_H
#define STORAGEENGINE_H

#include <memory>
#include <string>
#include <functional>
#include "crypto/KeyPair.h"
#include "drive/FsTree.h"
#include "drive/ActionList.h"

#include "Model.h"

namespace sirius { namespace drive {
    class ClientSession;
}}

using  endpoint_list  = std::vector<boost::asio::ip::tcp::endpoint>;

class StorageEngine
{
    std::shared_ptr<sirius::drive::ClientSession>   m_session;

    //std::vector<lt::torrent_handle>                 m_fsTreeLtHandles;
    std::recursive_mutex                            m_mutex;

public:
    StorageEngine() {}

    sirius::drive::InfoHash addActions(const sirius::drive::ActionList& actions,
                                       const sirius::Key& driveId,
                                       const std::string& sandboxFolder,
                                       uint64_t& modifySize);

    void start();

    void restart();

    void downloadFsTree( const std::string&             driveHash,
                         const std::string&             dnChannelId,
                         const std::array<uint8_t,32>&  fsTreeHash,
                         FsTreeHandler                  onFsTreeReceived );

    sirius::drive::lt_handle downloadFile( const std::array<uint8_t,32>&  channelInfo,
                                           const std::array<uint8_t,32>&  fileHash );

    void removeTorrentSync( sirius::drive::InfoHash infoHash );

private:
    void init(const sirius::crypto::KeyPair&  keyPair,
              const std::string&              address,
              const endpoint_list&            bootstraps );

    void torrentDeletedHandler( const sirius::drive::InfoHash& infoHash );
};

inline std::shared_ptr<StorageEngine> gStorageEngine;

#endif // STORAGEENGINE_H
