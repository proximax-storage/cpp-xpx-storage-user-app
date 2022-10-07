#pragma once

#include <vector>
#include <string>
#include <array>

#include "drive/ClientSession.h"
#include "drive/FsTree.h"

#ifdef __APPLE__
inline bool STANDALONE_TEST = true;
#else
inline bool STANDALONE_TEST = false;
#endif

inline std::recursive_mutex gSettingsMutex;

namespace fs = std::filesystem;

struct DriveInfo;

using  FsTreeHandler  = std::function<void( const std::string&           driveHash,
                                            const std::array<uint8_t,32> fsTreeHash,
                                            const sirius::drive::FsTree& fsTree )>;

//
// ChannelInfo
//
struct ChannelInfo
{
    std::string     m_name;
    std::string     m_hash;
    std::string     m_driveHash;
    endpoint_list   m_endpointList = {};

    bool                                m_waitingFsTree              = true;
    std::optional<lt::torrent_handle>   m_tmpRequestingFsTreeTorrent = {};
    std::array<uint8_t,32>              m_tmpRequestingFsTreeHash    = {};


    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_hash, m_name, m_driveHash );
    }
};

//
// DownloadInfo
//
struct DownloadInfo
{
    std::array<uint8_t,32>  m_hash;
    std::string             m_fileName;
    std::string             m_saveFolder;
    bool                    m_isCompleted = false;

    int                      m_progress = 0; // m_progress==1001 means completed
    sirius::drive::lt_handle m_ltHandle;

    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_hash, m_fileName, m_saveFolder, m_isCompleted );
    }

    bool isCompleted() const { return m_isCompleted; }
};

//
// DriveInfo
//
struct DriveInfo
{
    std::string m_driveHash;
    std::string m_name;
    std::string m_localDriveFolder;

public: // tmp
    std::optional<std::array<uint8_t,32>>   m_rootHash;
    sirius::drive::FsTree                   m_fsTree;
    // diff
    sirius::drive::ActionList               m_actionList;

    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_driveHash, m_name, m_localDriveFolder );
    }
};

//
// Model
//
class Model
{
public:

    //
    // Settings
    //
    static bool     loadSettings();
    static fs::path downloadFolder();

    static int&     currentDnChannelIndex();

    static std::vector<ChannelInfo>& dnChannels();

    static std::vector<DownloadInfo>& downloads();

    static ChannelInfo* currentChannelInfoPtr();

    //
    // StorageEngine
    //
    static void startStorageEngine();

    static void downloadFsTree( const std::string&             driveHash,
                                const std::string&             dnChannelId,
                                const std::array<uint8_t,32>&  fsTreeHash,
                                FsTreeHandler                  onFsTreeReceived );

    static sirius::drive::lt_handle downloadFile( ChannelInfo&                  channelInfo,
                                                  const std::array<uint8_t,32>& fileHash );


    //
    // Drives
    //
    static std::vector<DriveInfo>&  drives();
    static DriveInfo*               currentDriveInfoPtr();
    static void                     removeFromDownloads( int rowIndex );

    static void                     onFsTreeForDriveReceived( const std::string&           driveHash,
                                                              const std::array<uint8_t,32> fsTreeHash,
                                                              const sirius::drive::FsTree& fsTree );

    //
    // Standalone test
    //
    static void stestInitChannels();
    static void stestInitDrives();
};
