#pragma once

#include <QObject>
#include <QStringList>
#include <QCoreApplication>

#include <vector>
#include <string>
#include <array>
#include <chrono>

#include <cereal/types/chrono.hpp>

#ifndef WA_APP
#include "drive/ViewerSession.h"
#endif

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
#include "Common.h"

namespace fs = std::filesystem;

class Settings;
struct LocalDriveItem;
class AddDownloadChannelDialog;

struct EasyDownloadInfo;

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
        QString getDownloadFolder();
        QString getAccountName();
        void setCurrentAccountIndex(int index);
        QString getBootstrapReplicator();
        void setBootstrapReplicator(const QString& address);
        QString getUdpPort();
        QString getGatewayIp();
        QString getGatewayPort();
        double getFeeMultiplier();
        Settings* getSettings();
        const sirius::crypto::KeyPair& getKeyPair();
        void onDownloadCompleted( lt::torrent_handle handle );

        //
        // Channels
        //
        void addDownloadChannel(const DownloadChannel& channel);
        std::map<QString, DownloadChannel>& getDownloadChannels();
        void setCurrentDownloadChannelKey(const QString& channelKey);
        QString currentDownloadChannelKey();
        DownloadChannel*             currentDownloadChannel();
        DownloadChannel*             findChannel( const QString& channelKey );
        CachedReplicator             findReplicatorByPublicKey(const QString& replicatorPublicKey) const;
        void                         updateReplicatorAlias(const QString& replicatorPublicKey, const QString& alias) const;
        void                         removeChannel( const QString& channelKey );
        void                         applyForChannels( const QString& driveKey, std::function<void(DownloadChannel&)> );
        void                         applyFsTreeForChannels( const QString& driveKey, const sirius::drive::FsTree& fsTree, const std::array<uint8_t, 32>& fsTreeHash );
        std::vector<DownloadInfo>&   downloads();
        void markChannelsForDelete(const QString& driveId, bool state);
        bool isDownloadChannelsLoaded();
        void setDownloadChannelsLoaded(bool state);

        std::map<QString, CachedReplicator> getMyReplicators() const;
        void addMyReplicator(const CachedReplicator& replicator);
        void removeMyReplicator(const QString& replicator);

        //
        // Drives
        //
        
        void addDrive(const Drive& drive);
        void                     onMyOwnChannelsLoaded(const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& channelsPages);
        void                     onSponsoredChannelsLoaded(const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& channelsPages);
        void                     onDrivesLoaded( const std::vector<xpx_chain_sdk::drives_page::DrivesPage>& drivesPages );
        void                     setCurrentDriveKey( const QString& driveKey );
        QString              currentDriveKey();
        bool isDriveWithNameExists(const QString& driveName) const;
        bool isChannelWithNameExists(const QString& channelName) const;
        bool isReplicatorWithNameExists(const QString& replicatorName) const;
        QRect getWindowGeometry() const;
        void setWindowGeometry(const QRect& geometry);
        void initForTests();
        QString getClientPublicKey();
        QString getClientPrivateKey();
        bool isDrivesLoaded();
        void setDrivesLoaded(bool state);
        void setLoadedDrivesCount(uint64_t count);
        uint64_t getLoadedDrivesCount() const;
        void setOutdatedDriveNumber(uint64_t count);
        uint64_t getOutdatedDriveNumber() const;

        std::map<QString, Drive>& getDrives();

        Drive* currentDrive();

        Drive *findDrive(const QString &driveKey);

        Drive *findDriveByNameOrPublicKey(const QString &value);

        Drive *findDriveByModificationId(const std::array<uint8_t, 32> &modificationId);

        CachedReplicator findReplicatorByNameOrPublicKey( const QString& value );

        void removeDrive(const QString &driveKey);

        void removeChannelsByDriveKey(const QString &driveKey);

        void removeFromDownloads(int rowIndex);

        static void calcDiff(  Drive& drive );

        //
        // StorageEngine
        //
        void initStorageEngine();
        void startStorageEngine();
        void endStorageEngine();
    
        uint64_t lastModificationSize() const;

        sirius::drive::lt_handle downloadFile( const QString&                channelId,
                                               const std::array<uint8_t,32>& fileHash,
                                               std::filesystem::path         outputFolder = {});

        void                     removeTorrentSync( sirius::drive::InfoHash infoHash );

        static std::array<uint8_t,32>   hexStringToHash( const QString& str );
    
        void                            setModificationStatusResponseHandler( ModificationStatusResponseHandler handler );
        void                            requestModificationStatus(  const QString&      replicatorKey,
                                                                    const QString&      driveKey,
                                                                    const QString&      modificationHash,
                                                                    std::optional<boost::asio::ip::udp::endpoint> replicatorEndpoint = {} );

        // Super contract
        DriveContractModel&             driveContractModel();

        //
        // Streaming
        //
        const std::vector<StreamInfo>&  streamerAnnouncements() const;
        std::vector<StreamInfo>&        getStreams();
    
        Drive*                          currentStreamingDrive() const;

        //
        // Viewing
        //
        using ViewingFsTreeHandler = std::optional<std::function<void( bool success, const QString& channelKey, const QString& driveKey )>>;
        void                            setViewingFsTreeHandler( ViewingFsTreeHandler handler ) { m_viewingFsTreeHandler = handler; }
        void                            resetViewingFsTreeHandler() { m_viewingFsTreeHandler.reset(); }
        ViewingFsTreeHandler&           viewingFsTreeHandler() { return m_viewingFsTreeHandler; }
        using FsTreeHandler = std::function<bool( bool success, const QString& channelKey, const QString& driveKey )>;
        void                            addChannelFsTreeHandler( FsTreeHandler handler ) { m_channelFsTreeHandler.push_front(handler); }
        std::list<FsTreeHandler>&       channelFsTreeHandler() { return m_channelFsTreeHandler; }
       
        void                            addStreamRef( const StreamInfo& streamInfo );
        void                            deleteStreamRef( int index );
        const std::vector<StreamInfo>&  streamRefs() const;
        void                            setCurrentStreamInfo( std::optional<StreamInfo> info );
        std::optional<StreamInfo>       currentStreamInfo() const;
        const StreamInfo*               getStreamRef( int index ) const;
        StreamInfo*                     getStreamRef( int index );

        void                            requestStreamStatus( const StreamInfo& streamInfo, StreamStatusResponseHandler );
    
        //
        // EasyDownloads
        //
        const std::vector<EasyDownloadInfo>&    easyDownloads() const;
        std::vector<EasyDownloadInfo>&          easyDownloads();
        uint64_t&                               lastUniqueIdOfEasyDownload();

    signals:
        void addTorrentFileToStorageSession(const QString &torrentFilename,
                                            const QString &folderWhereFileIsLocated,
                                            const std::array<uint8_t, 32>& driveKey,
                                            const std::array<uint8_t, 32>& modifyTx);

    private:
        Settings* m_settings;
        uint64_t m_loadedDrivesCount;
        uint64_t m_outdatedDriveNumber;
    
        ViewingFsTreeHandler                m_viewingFsTreeHandler;
        std::list<FsTreeHandler>            m_channelFsTreeHandler;

    public:
        ViewerStatus    m_viewerStatus   = vs_no_viewing;
        StreamInfo      m_currentStreamInfo;
};
