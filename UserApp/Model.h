#pragma once

#include <QStringList>

#include <vector>
#include <string>
#include <array>
#include <chrono>

#include <cereal/types/chrono.hpp>


#include "drive/ClientSession.h"
#include "drive/FsTree.h"

#ifdef __APPLE__
inline bool ALEX_LOCAL_TEST = false;
inline bool VICTOR_LOCAL_TEST = true;

inline const char* ROOT_HASH1 = "096975e6f49b924078e443e6c208283034d043dd42b4db9ccd1dffd795577e5d";
inline const char* ROOT_HASH2 = "83f8349b1623008b58fd9e2ee678e47842787834e0671e4cd2f6634d8ebfd2e6";
#else
inline const char* ROOT_HASH1 = "096975e6f49b924078e443e6c208283034d043dd42b4db9ccd1dffd795577e5d";
inline const char* ROOT_HASH2 = "83f8349b1623008b58fd9e2ee678e47842787834e0671e4cd2f6634d8ebfd2e6";
inline bool STANDALONE_TEST = false;
#endif

inline std::recursive_mutex gSettingsMutex;

namespace fs = std::filesystem;

struct DriveInfo;
struct LocalDriveItem;

using  FsTreeHandler  = std::function<void( const std::string&           driveHash,
                                            const std::array<uint8_t,32> fsTreeHash,
                                            const sirius::drive::FsTree& fsTree )>;

//
// ChannelInfo
//
struct ChannelInfo
{
    using timepoint = std::chrono::time_point<std::chrono::steady_clock>;

    std::string                 m_name;
    std::string                 m_hash;
    std::string                 m_driveHash;
    std::vector<std::string>    m_allowedPublicKeys;
    bool                        m_isCreating = true;
    bool                        m_isDeleting = false;
    timepoint                   m_creationTimepoint;
    timepoint                   m_tobeDeletedTimepoint;
    endpoint_list               m_endpointList = {};

    bool                                m_waitingFsTree              = true;
    std::optional<lt::torrent_handle>   m_tmpRequestingFsTreeTorrent = {};
    std::array<uint8_t,32>              m_tmpRequestingFsTreeHash    = {};


    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_hash,
            m_name,
            m_driveHash,
            m_allowedPublicKeys,
            m_isCreating,
            m_isDeleting,
            m_creationTimepoint,
            m_tobeDeletedTimepoint );
    }
};

//
// DownloadInfo
//
struct DownloadInfo
{
    std::array<uint8_t,32>  m_hash;
    std::string             m_channelHash;
    std::string             m_fileName;
    std::string             m_saveFolder;
    bool                    m_isCompleted = false;

    int                      m_progress = 0; // m_progress==1001 means completed
    sirius::drive::lt_handle m_ltHandle;

    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_hash, m_channelHash, m_fileName, m_saveFolder, m_isCompleted );
    }

    bool isCompleted() const { return m_isCompleted; }
};

//
// DriveInfo
//
struct DriveInfo
{
    std::string m_driveKey;
    std::string m_name;
    std::string m_localDriveFolder;
    uint64_t    m_maxDriveSize = 0;
    uint32_t    m_replicatorNumber = 0;
    bool        m_isCreating = true;
    bool        m_isDeleting = false;

public: // tmp
    std::optional<std::array<uint8_t,32>>   m_rootHash;
    sirius::drive::FsTree                   m_fsTree;
    // diff
    std::shared_ptr<LocalDriveItem>         m_localDrive;
    sirius::drive::ActionList               m_actionList;

    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_driveKey,
            m_name,
            m_localDriveFolder,
            m_maxDriveSize,
            m_replicatorNumber,
            m_isCreating,
            m_isDeleting );
    }
};

//
// Model
//
class Model
{
public:

    static bool     isZeroHash( const std::array<uint8_t,32>& hash );
    static fs::path homeFolder();

    //
    // Settings
    //
    static bool     loadSettings();
    static void     saveSettings();
    static fs::path downloadFolder();

    //
    // Channels
    //
    static void                         onChannelLoaded( const std::string& channelKey,
                                                         const std::string& driveKey,
                                                         const std::vector<std::string>& listOfPublicKeys );

    static std::vector<ChannelInfo>&    dnChannels();
    static void                         setCurrentDnChannelIndex( int );
    static int                          currentDnChannelIndex();
    static ChannelInfo*                 currentChannelInfoPtr();
    static ChannelInfo*                 findChannel( const std::string& channelKey );

    static std::vector<DownloadInfo>&   downloads();


    //
    // Drives
    //
    static void                     onDrivesLoaded( const QStringList& drives );

    static void                     addDrive( const std::string& driveHash,
                                              const std::string& driveName,
                                              const std::string& localFolder );

    static void                     setCurrentDriveIndex( int );
    static int                      currentDriveIndex();


    static std::vector<DriveInfo>&  drives();
    static DriveInfo*               currentDriveInfoPtr();
    static void                     removeFromDownloads( int rowIndex );

    static void                     calcDiff();

    //
    // StorageEngine
    //
    static void startStorageEngine();

    static void downloadFsTree( const std::string&             driveHash,
                                const std::string&             dnChannelId,
                                const std::array<uint8_t,32>&  fsTreeHash,
                                FsTreeHandler                  onFsTreeReceived );

    static sirius::drive::lt_handle downloadFile( const std::string&            channelId,
                                                  const std::array<uint8_t,32>& fileHash );

    static void                     onFsTreeForDriveReceived( const std::string&           driveHash,
                                                              const std::array<uint8_t,32> fsTreeHash,
                                                              const sirius::drive::FsTree& fsTree );

    //
    // Standalone test
    //
    static void stestInitChannels();
    static void stestInitDrives();
};
