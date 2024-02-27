#pragma once

#include <QObject>
#include <QStringList>
#include <QCoreApplication>

#include <vector>
#include <string>
#include <array>
#include <chrono>

#include <cereal/types/chrono.hpp>

#include "drive/ViewerSession.h"
#include "drive/FsTree.h"
#include "drive/FlatDrive.h"
#include "xpxchaincpp/model/storage/drives_page.h"
#include "xpxchaincpp/model/storage/download_channels_page.h"
#include "Entities/Drive.h"
#include "Entities/DownloadInfo.h"
#include "Entities/DownloadChannel.h"
#include "Entities/CachedReplicator.h"
#include "Entities/StreamInfo.h"
#include "Models/DriveContractModel.h"

inline const char* ROOT_HASH1 = "096975e6f49b924078e443e6c208283034d043dd42b4db9ccd1dffd795577e5d";
inline const char* ROOT_HASH2 = "83f8349b1623008b58fd9e2ee678e47842787834e0671e4cd2f6634d8ebfd2e6";

namespace fs = std::filesystem;

class Settings;
struct LocalDriveItem;

enum ViewerStatus
{
    vs_no_viewing,
    vs_waiting_channel_creation,
    vs_waiting_stream_start,
    vs_viewing,
};

enum StreamerStatus
{
    ss_no_streaming,
//    vs_channel_creation,
//    vs_waiting_stream_id,
//    vs_viewing,
};

using ModificationStatusResponseHandler = std::function<void( const sirius::drive::ReplicatorKey&       replicatorKey,
                                                              const sirius::Hash256&                    modificationHash,
                                                              const sirius::drive::ModifyTrafficInfo&   msg,
                                                              lt::string_view                           currentTask,
                                                              bool                                      isModificationQueued,
                                                              bool                                      isModificationFinished,
                                                              const std::string&                        error )>;

class Model : public QObject
{
    Q_OBJECT

    public:
        explicit Model(Settings* settings, QObject* parent = nullptr);
        ~Model() override = default;

    public:
        static bool     isZeroHash( const std::array<uint8_t,32>& hash );
        static fs::path homeFolder();

        //
        // Settings
        //
        bool     loadSettings();
        void     saveSettings();
        fs::path getDownloadFolder();
        std::string getAccountName();
        void setCurrentAccountIndex(int index);
        std::string getBootstrapReplicator();
        void setBootstrapReplicator(const std::string& address);
        std::string getUdpPort();
        std::string getGatewayIp();
        std::string getGatewayPort();
        double getFeeMultiplier();
        const sirius::crypto::KeyPair& getKeyPair();
        void onDownloadCompleted( lt::torrent_handle handle );

        //
        // Channels
        //
        void addDownloadChannel(const DownloadChannel& channel);
        std::map<std::string, DownloadChannel>& getDownloadChannels();
        void setCurrentDownloadChannelKey(const std::string& channelKey);
        std::string currentDownloadChannelKey();
        DownloadChannel*             currentDownloadChannel();
        DownloadChannel*             findChannel( const std::string& channelKey );
        CachedReplicator             findReplicatorByPublicKey(const std::string& replicatorPublicKey) const;
        void                         updateReplicatorAlias(const std::string& replicatorPublicKey, const std::string& alias) const;
        void                         removeChannel( const std::string& channelKey );
        void                         applyForChannels( const std::string& driveKey, std::function<void(DownloadChannel&)> );
        void                         applyFsTreeForChannels( const std::string& driveKey, const sirius::drive::FsTree& fsTree, const std::array<uint8_t, 32>& fsTreeHash );
        std::vector<DownloadInfo>&   downloads();
        void markChannelsForDelete(const std::string& driveId, bool state);
        bool isDownloadChannelsLoaded();
        void setDownloadChannelsLoaded(bool state);

        std::map<std::string, CachedReplicator> getMyReplicators() const;
        void addMyReplicator(const CachedReplicator& replicator);
        void removeMyReplicator(const std::string& replicator);

        //
        // Drives
        //
        
        void addDrive(const Drive& drive);
        void                     onMyOwnChannelsLoaded(const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& channelsPages);
        void                     onSponsoredChannelsLoaded(const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& channelsPages);
        void                     onDrivesLoaded( const std::vector<xpx_chain_sdk::drives_page::DrivesPage>& drivesPages );
        void                     setCurrentDriveKey( const std::string& driveKey );
        std::string              currentDriveKey();
        bool isDriveWithNameExists(const QString& driveName) const;
        bool isChannelWithNameExists(const QString& channelName) const;
        bool isReplicatorWithNameExists(const QString& replicatorName) const;
        QRect getWindowGeometry() const;
        void setWindowGeometry(const QRect& geometry);
        void initForTests();
        std::string getClientPublicKey();
        std::string getClientPrivateKey();
        bool isDrivesLoaded();
        void setDrivesLoaded(bool state);
        void setLoadedDrivesCount(uint64_t count);
        uint64_t getLoadedDrivesCount() const;
        void setOutdatedDriveNumber(uint64_t count);
        uint64_t getOutdatedDriveNumber() const;

        std::map<std::string, Drive>& getDrives();

        Drive* currentDrive();

        Drive *findDrive(const std::string &driveKey);

        Drive *findDriveByNameOrPublicKey(const std::string &value);

        Drive *findDriveByModificationId(const std::array<uint8_t, 32> &modificationId);

        CachedReplicator findReplicatorByNameOrPublicKey( const std::string& value );

        void removeDrive(const std::string &driveKey);

        void removeChannelsByDriveKey(const std::string &driveKey);

        void removeFromDownloads(int rowIndex);

        static void calcDiff(  Drive& drive );

        //
        // StorageEngine
        //
        void initStorageEngine();
        void startStorageEngine();
        void endStorageEngine();
    
        uint64_t lastModificationSize() const;

        sirius::drive::lt_handle downloadFile( const std::string&            channelId,
                                               const std::array<uint8_t,32>& fileHash );

        void                     removeTorrentSync( sirius::drive::InfoHash infoHash );

        static std::array<uint8_t,32>   hexStringToHash( const std::string& str );
    
        void                            setModificationStatusResponseHandler( ModificationStatusResponseHandler handler );
        void                            requestModificationStatus(  const std::string&      replicatorKey,
                                                                    const std::string&      driveKey,
                                                                    const std::string&      modificationHash,
                                                                    std::optional<boost::asio::ip::udp::endpoint> replicatorEndpoint = {} );

        // Super contract
        DriveContractModel&             driveContractModel();

        //
        // Streaming
        //
        const std::vector<StreamInfo>&  streamerAnnouncements() const;
        std::vector<StreamInfo>&        streamerAnnouncements();
        std::string&                    streamFolder();

        //
        // Viewing
        //
        void                            addStreamRef( const StreamInfo& streamInfo );
        void                            deleteStreamRef( int index );
        const std::vector<StreamInfo>&  streamRefs() const;
        void                            setCurrentStreamInfo( std::optional<StreamInfo> info );
        std::optional<StreamInfo>       currentStreamInfo() const;
        const StreamInfo*               getStreamRef( int index ) const;
        StreamInfo*                     getStreamRef( int index );

        void                            requestStreamStatus( const StreamInfo& streamInfo, StreamStatusResponseHandler );

    signals:
        void addTorrentFileToStorageSession(const std::string &torrentFilename,
                                            const std::string &folderWhereFileIsLocated,
                                            const std::array<uint8_t, 32>& driveKey,
                                            const std::array<uint8_t, 32>& modifyTx);

    private:
        Settings* m_settings;
        uint64_t m_loadedDrivesCount;
        uint64_t m_outdatedDriveNumber;
    
    public:
        ViewerStatus    m_viewerStatus   = vs_no_viewing;
        StreamInfo      m_currentStreamInfo;
};
