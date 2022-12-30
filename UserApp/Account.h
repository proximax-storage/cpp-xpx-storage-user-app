#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QObject>

#include <string>
#include <vector>
#include <optional>

#include "Drive.h"
#include "DownloadInfo.h"
#include "DownloadChannel.h"
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
                    m_currentDnChannelIndex,
                    m_downloadFolder,
                    m_drives,
                    m_currentDriveKey );
        }

        std::string m_accountName;
        std::string m_privateKeyStr;
        std::string m_publicKeyStr;
        std::vector<DownloadChannel> m_dnChannels;
        int m_currentDnChannelIndex = -1;
        bool m_channelsLoaded = false;
        std::string m_downloadFolder;
        std::vector<DownloadInfo> m_downloads;
        std::map<std::string, Drive> m_drives;
        std::string m_currentDriveKey;
        bool m_drivesLoaded = false;
        std::optional<sirius::crypto::KeyPair> m_keyPair;
};


#endif //ACCOUNT_H
