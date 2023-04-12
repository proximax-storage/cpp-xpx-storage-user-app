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

class Account : public QObject
{
    Q_OBJECT

    public:
        Account(QObject* parent = nullptr);
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
                    m_streams,
                    m_currentStreamIndex,
                    m_streamRefs,
                    m_currentStreamRefIndex,
                    m_currentDriveKey,
                    m_lastUniqueStreamIndex );
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

        std::vector<StreamInfo> m_streams;
        int                     m_currentStreamIndex = -1;
        std::vector<StreamInfo> m_streamRefs; // viewer part
        int                     m_currentStreamRefIndex = -1;
        uint64_t                m_lastUniqueStreamIndex = 0;

        std::optional<sirius::crypto::KeyPair> m_keyPair;
};


#endif //ACCOUNT_H
