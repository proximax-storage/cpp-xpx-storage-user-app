#ifndef STORAGEENGINE_H
#define STORAGEENGINE_H

#include <QObject>
#include <memory>
#include <string>
#include <functional>
#include "crypto/KeyPair.h"
#include "drive/FsTree.h"
#include "drive/ActionList.h"

namespace sirius::drive {
    class ClientSession;
}

using  endpoint_list  = std::vector<boost::asio::ip::tcp::endpoint>;

class Model;

class StorageEngine : public QObject
{
    Q_OBJECT

    std::shared_ptr<sirius::drive::ClientSession>   m_session;
    std::recursive_mutex                            m_mutex;

public:
    StorageEngine(Model* model, QObject* parent = nullptr);

    sirius::drive::InfoHash addActions(const sirius::drive::ActionList& actions,
                                       const sirius::Key& driveId,
                                       const std::string& sandboxFolder,
                                       uint64_t& modifySize);

    void start( std::function<void()> addressAlreadyInUseHandler );

    void restart();

    void addReplicatorList( const sirius::drive::ReplicatorList& );

    void downloadFsTree( const std::string&                     driveHash,
                         const std::string&                     dnChannelId,
                         const std::array<uint8_t,32>&          fsTreeHash,
                         const sirius::drive::ReplicatorList&   replicatorList );

    sirius::drive::lt_handle downloadFile( const std::array<uint8_t,32>&  channelInfo,
                                           const std::array<uint8_t,32>&  fileHash );

    void removeTorrentSync( sirius::drive::InfoHash infoHash );

signals:
    void fsTreeReceived(const std::string& myDriveId, const std::array<uint8_t, 32>& fsTreeHash, const sirius::drive::FsTree& fsTree);

private:
    void init(const sirius::crypto::KeyPair&  keyPair,
              const std::string&              address,
              const endpoint_list&            bootstraps,
              std::function<void()>           addressAlreadyInUseHandler );

    void torrentDeletedHandler( const sirius::drive::InfoHash& infoHash );

private:
    Model* mp_model;
};

inline std::shared_ptr<StorageEngine> gStorageEngine;

#endif // STORAGEENGINE_H
