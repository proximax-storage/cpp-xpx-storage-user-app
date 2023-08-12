#ifndef STORAGEENGINE_H
#define STORAGEENGINE_H

#include <QObject>
#include <memory>
#include <string>
#include <functional>
#include <filesystem>
#include "crypto/KeyPair.h"
#include "drive/FsTree.h"
#include "drive/ActionList.h"
#include "types.h"

//#define USE_CLIENT_SESSION

namespace sirius::drive {
    class ClientSession;
    class ViewerSession;
    struct DriveKey;
}

namespace fs = std::filesystem;

using  endpoint_list  = std::vector<boost::asio::ip::tcp::endpoint>;

class Model;
struct StreamInfo;

using StreamStatusResponseHandler = std::function<void( const sirius::drive::DriveKey&  driveKey,
                                                        bool                            isStreaming,
                                                        const std::array<uint8_t,32>&   streamId )>;

class StorageEngine : public QObject
{
    Q_OBJECT

public:
#ifdef USE_CLIENT_SESSION
    std::shared_ptr<sirius::drive::ClientSession>   m_session;
#else
    std::shared_ptr<sirius::drive::ViewerSession>   m_session;
#endif
    std::recursive_mutex                            m_mutex;

public:
    StorageEngine(Model* model, QObject* parent = nullptr);

    sirius::drive::InfoHash addActions(const sirius::drive::ActionList& actions,
                                       const sirius::Key& driveId,
                                       const std::string& sandboxFolder,
                                       uint64_t& modifySize,
                                       const std::vector<std::string>& replicators);

    void start( std::function<void()> addressAlreadyInUseHandler );

    void restart();

    void addReplicators( const sirius::drive::ReplicatorList& replicators);

    void downloadFsTree( const std::string&                     driveHash,
                         const std::string&                     dnChannelId,
                         const std::array<uint8_t,32>&          fsTreeHash);

    sirius::drive::lt_handle downloadFile( const std::array<uint8_t,32>&  channelInfo,
                                           const std::array<uint8_t,32>&  fileHash);

    void removeTorrentSync( sirius::drive::InfoHash infoHash );
    
    void requestStreamStatus( const std::array<uint8_t,32>& driveKey,
                              const sirius::drive::ReplicatorList&,
                              StreamStatusResponseHandler streamStatusResponseHandler );

    void init(const sirius::crypto::KeyPair&  keyPair,
              const std::string&              address,
              const endpoint_list&            bootstraps,
              std::function<void()>           addressAlreadyInUseHandler );

    void torrentDeletedHandler( const sirius::drive::InfoHash& infoHash );
    
    std::optional<boost::asio::ip::tcp::endpoint> getEndpoint( const sirius::Key& key );

    void startStreaming( const sirius::Hash256&  streamId,
                         const sirius::Key&      driveKey,
                         const fs::path&         m3u8Playlist,
                         const fs::path&         chunksFolder,
                         const fs::path&         torrentsFolder,
                         const endpoint_list&    endPointList );

    void finishStreaming();
    void cancelStreaming();

private:
    void addTorrentFileToSession(const std::string &torrentFilename,
                                 const std::string &folderWhereFileIsLocated,
                                 const std::array<uint8_t, 32>& driveKey,
                                 const std::array<uint8_t, 32>& modifyTx);

signals:
    void fsTreeReceived(const std::string& myDriveId, const std::array<uint8_t, 32>& fsTreeHash, const sirius::drive::FsTree& fsTree);

private:
    Model* mp_model;
};

inline std::shared_ptr<StorageEngine> gStorageEngine;

#endif // STORAGEENGINE_H
