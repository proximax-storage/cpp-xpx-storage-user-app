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
#include "Common.h"

//#define USE_CLIENT_SESSION

namespace sirius::drive {
    class ClientSession;
    class ViewerSession;
    class FinishStreamInfo;
    struct DriveKey;
}

namespace fs = std::filesystem;

using  endpoint_list  = std::vector<boost::asio::ip::udp::endpoint>;

class Model;
struct StreamInfo;

using StreamStatusResponseHandler = std::function<void( const sirius::drive::DriveKey&  driveKey,
                                                        const sirius::Key&              streamerKey,
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
    uint64_t                                        m_lastModifySize;

public:
    StorageEngine(Model* model, QObject* parent = nullptr);

    sirius::drive::InfoHash addActions(const sirius::drive::ActionList& actions,
                                       const sirius::Key& driveId,
                                       const QString& sandboxFolder,
                                       uint64_t& modifySize,
                                       const std::vector<QString>& replicators);

    void start();

    void restart();

    void addReplicators( const sirius::drive::ReplicatorList& replicators);

    void downloadFsTree( const QString&                     driveHash,
                         const QString&                     dnChannelId,
                         const std::array<uint8_t,32>&      fsTreeHash);

    sirius::drive::lt_handle downloadFile( const std::array<uint8_t,32>&        channelInfo,
                                           const std::array<uint8_t,32>&        fileHash,
                                           std::filesystem::path                outputFolder = {} );

    void removeTorrentSync( sirius::drive::InfoHash infoHash );
    
    void requestStreamStatus( const std::array<uint8_t,32>& driveKey,
                              const sirius::drive::ReplicatorList&,
                              StreamStatusResponseHandler streamStatusResponseHandler );
    
    std::shared_ptr<sirius::drive::ViewerSession>  getViewerSession()
    {
        return m_session;
    }

    void init(const sirius::crypto::KeyPair&  keyPair,
              QString                         address,
              const endpoint_list&            bootstraps,
              std::function<void()>           addressAlreadyInUseHandler );

    void torrentDeletedHandler( const sirius::drive::InfoHash& infoHash );
    
    void streamStatusHandler( const std:: string&);
    
    std::optional<boost::asio::ip::udp::endpoint> getEndpoint( const sirius::Key& key );

    void startStreaming( const sirius::Hash256&  streamId,
                         const QString&      streamFolderName,
                         const sirius::Key&      driveKey,
                         const fs::path&         m3u8Playlist,
                         const fs::path&         driveLocalFolder,
                         const fs::path&         torrentLocalFolder,
                         sirius::drive::StreamingStatusHandler streamingStatusHandler,
                         const endpoint_list&    endPointList );

    void finishStreaming( const QString& driveKey, std::function<void(const sirius::Key& driveKey, const sirius::drive::InfoHash& streamId, const sirius::drive::InfoHash& actionListHash, uint64_t streamBytes)> backCall );
    void cancelStreaming();

signals:
    void newError(int errorType, const QString& errorText);

private:
    void addTorrentFileToSession(const QString &torrentFilename,
                                 const QString &folderWhereFileIsLocated,
                                 const std::array<uint8_t, 32>& driveKey,
                                 const std::array<uint8_t, 32>& modifyTx);

signals:
    void fsTreeReceived(const QString& myDriveId, const std::array<uint8_t, 32>& fsTreeHash, const sirius::drive::FsTree& fsTree);

private:
    Model* mp_model;
};

inline std::shared_ptr<StorageEngine> gStorageEngine;

#endif // STORAGEENGINE_H
