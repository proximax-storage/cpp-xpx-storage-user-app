#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QObject>

#include <string>
#include <vector>
#include <optional>

#include "Drive.h"
#include "StreamInfo.h"
#include "DownloadInfo.h"
#include "DownloadChannel.h"
#include "CachedReplicator.h"
#include "drive/Utils.h"
#include "Models/DriveContractModel.h"
#include "Entities/EasyDownloadInfo.h"

#define STREAM_ROOT_FOLDER_NAME "video"
#define STREAM_INFO_FILE_NAME   "info"

const  uint32_t MIN_SETTINGS_VERSION = 1;
const  uint32_t MAX_SETTINGS_VERSION = 2;
inline uint32_t gSettingsVersion = 0;

struct EasyDownloadInfo;

class Account
{
    public:
        Account();
        ~Account();
        Account( const Account& a );
        Account& operator=( const Account& a );

    public:
        void initAccount( std::string name, std::string privateKeyStr );

        template<class Archive>
        void serialize( Archive &ar )
        {
            ar(
                    m_accountName,
                    m_privateKeyStr,
                    m_dnChannels,
                    m_currentDownloadChannelKey,
                    m_downloadFolder,
                    m_drives,
                    m_currentDriveKey,
                    m_myReplicators,
                    m_currentStreamIndex,
                    m_streamRefs,
                    m_viewerStreamRootFolder,
                    m_currentStreamRefIndex,
                    m_currentDriveKey,
                    m_lastUniqueStreamIndex );
            
            if ( gSettingsVersion >= 2 )
            {
                ar( m_streamFolder );
            }
        }

        std::string m_accountName;
        std::string m_privateKeyStr;
        std::string m_publicKeyStr;

        std::map<std::string, CachedReplicator> m_myReplicators;
        std::map<std::string, DownloadChannel> m_dnChannels;
        std::string m_currentDownloadChannelKey;
        bool m_channelsLoaded = false;
        std::string m_downloadFolder;
        std::vector<DownloadInfo> m_downloads;

        std::map<std::string, Drive> m_drives;
        std::string m_currentDriveKey;
        bool m_drivesLoaded = false;

        std::string             m_viewerStreamRootFolder;

        DriveContractModel      m_driveContractModel;

        std::vector<StreamInfo>     m_streams;
        std::optional<StreamInfo>   m_currentStreamInfo;
        std::vector<StreamInfo>     m_streamRefs; // viewer part
    
        std::string                 m_streamFolder; // output folder for 'ffmpeg' or 'OBS'
    
        std::vector<EasyDownloadInfo> m_easyDownloads;
        uint64_t                      m_lastUniqueIdOfEasyDownload = 0;

        int                     m_currentStreamIndex = -1; // unused
        int                     m_currentStreamRefIndex = -1; // unused
        uint64_t                m_lastUniqueStreamIndex = 0; // unused
    
        std::optional<sirius::crypto::KeyPair> m_keyPair;
};


#endif //ACCOUNT_H
