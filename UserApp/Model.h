#pragma once

#include <QObject>
#include <QStringList>
#include <QtStateMachine>
#include <QCoreApplication>

#include <vector>
#include <string>
#include <array>
#include <chrono>

#include <cereal/types/chrono.hpp>

#include "drive/ClientSession.h"
#include "drive/FsTree.h"
#include "xpxchaincpp/model/storage/drives_page.h"
#include "xpxchaincpp/model/storage/download_channels_page.h"
#include "DriveModificationTransition.h"
#include "Drive.h"
#include "DownloadInfo.h"
#include "DownloadChannel.h"

inline bool ALEX_LOCAL_TEST = false;
inline bool VICTOR_LOCAL_TEST = false;

inline const char* ROOT_HASH1 = "096975e6f49b924078e443e6c208283034d043dd42b4db9ccd1dffd795577e5d";
inline const char* ROOT_HASH2 = "83f8349b1623008b58fd9e2ee678e47842787834e0671e4cd2f6634d8ebfd2e6";

namespace fs = std::filesystem;

class Settings;
struct LocalDriveItem;

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
        sirius::crypto::KeyPair& getKeyPair();
        void onDownloadCompleted( lt::torrent_handle handle );

        //
        // Channels
        //
        void addDownloadChannel(const DownloadChannel& channel);
        std::map<std::string, DownloadChannel>& getDownloadChannels();
        void setCurrentDownloadChannelKey(const std::string& channelKey);
        std::string currentDownloadChannelKey();
        DownloadChannel*                 currentDownloadChannel();
        DownloadChannel*                 findChannel( const std::string& channelKey );
        void                         removeChannel( const std::string& channelKey );
        void                         applyForChannels( const std::string& driveKey, std::function<void(DownloadChannel&)> );
        void                         applyFsTreeForChannels( const std::string& driveKey, const sirius::drive::FsTree& fsTree, const std::array<uint8_t, 32>& fsTreeHash );
        std::vector<DownloadInfo>&   downloads();
        void markChannelsForDelete(const std::string& driveId, bool state);
        bool isDownloadChannelsLoaded();
        void setDownloadChannelsLoaded(bool state);


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

        Drive *findDriveByModificationId(const std::array<uint8_t, 32> &modificationId);

        void removeDrive(const std::string &driveKey);

        void removeChannelByDriveKey(const std::string &driveKey);

        void removeFromDownloads(int rowIndex);

        void calcDiff();

        //
        // StorageEngine
        //
        void startStorageEngine( std::function<void()> addressAlreadyInUseHandler );
        void endStorageEngine();

        sirius::drive::lt_handle downloadFile( const std::string&            channelId,
                                               const std::array<uint8_t,32>& fileHash );

        static std::array<uint8_t,32>   hexStringToHash( const std::string& str );

        //
        // Standalone test
        //
        void stestInitChannels();
        void stestInitDrives();

    signals:
        void driveStateChanged(const std::string& driveKey, int state);

    private:
        void onDriveStateChanged(const Drive& drive);

    private:
        Settings* m_settings;
        uint64_t m_loadedDrivesCount;
        uint64_t m_outdatedDriveNumber;
};
