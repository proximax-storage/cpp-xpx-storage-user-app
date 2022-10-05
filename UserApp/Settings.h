#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#include <QDebug>

#include "crypto/KeyPair.h"
#include "drive/Utils.h"
#include "drive/Session.h"
#include "libtorrent/torrent_handle.hpp"

namespace fs = std::filesystem;

inline bool STANDALONE_TEST = true;

inline std::recursive_mutex gSettingsMutex;

std::filesystem::path settingsFolder();

inline QDebug& operator<<(QDebug& out, const std::string& str) { out << QString::fromStdString(str); return out; }


class Settings
{
public:

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

    struct DriveInfo
    {
        std::string m_driveHash;
        std::string m_name;

        std::optional<lt::torrent_handle>   m_tmpRequestingFsTreeTorrent = {};
        std::array<uint8_t,32>              m_tmpRequestingFsTreeHash    = {};

        template<class Archive>
        void serialize( Archive &ar )
        {
            ar( m_driveHash, m_name );
        }
    };

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

    struct Account
    {
        std::string                 m_restBootstrap       = "google.com:7001"; //TODO!!!
        std::string                 m_replicatorBootstrap = "192.168.2.101:5001"; //TODO!!!
        std::string                 m_udpPort             = "6846";

        std::string                 m_privateKeyStr;
        std::string                 m_publicKeyStr;

        std::vector<ChannelInfo>    m_dnChannels;
        int                         m_currentDnChannelIndex = -1;

        std::string                 m_downloadFolder = "~/Downloads";
        std::vector<DownloadInfo>   m_downloads;

        std::vector<DriveInfo>      m_drives;
        int                         m_currentDriveIndex = -1;

        Account() {}

        Account& operator=( const Account& a )
        {
            m_restBootstrap         = a.m_restBootstrap;
            m_replicatorBootstrap   = a.m_replicatorBootstrap;
            m_udpPort               = a.m_udpPort;
            m_privateKeyStr         = a.m_privateKeyStr;
            m_dnChannels            = a.m_dnChannels;
            m_currentDnChannelIndex = a.m_currentDnChannelIndex;
            m_downloadFolder        = a.m_downloadFolder;
            m_downloads             = a.m_downloads;
            m_drives                = a.m_drives;
            m_currentDriveIndex     = a.m_currentDriveIndex;

            updateKeyPair( m_privateKeyStr );
            return *this;
        }

        Account( const Account& a )
        {
            *this = a;
        }

        template<class Archive>
        void serialize( Archive &ar )
        {
            ar( m_restBootstrap,
                m_replicatorBootstrap,
                m_udpPort,
                m_privateKeyStr,
                m_dnChannels,
                m_currentDnChannelIndex,
                m_downloadFolder );
        }

        std::optional<sirius::crypto::KeyPair>  m_keyPair;

        void updateKeyPair( std::string privateKeyStr )
        {
            m_privateKeyStr = privateKeyStr;

            if ( m_privateKeyStr.size() == 64 )
            {
                m_keyPair =
                    sirius::crypto::KeyPair::FromPrivate(
                        sirius::crypto::PrivateKey::FromString( m_privateKeyStr ));

                m_publicKeyStr = sirius::drive::toString( m_keyPair->publicKey().array() );
            }
        }

    };

public:

    Settings() {}

    bool load( const std::string& pwd );
    void save();

    bool loaded() const { return m_loaded; }

    std::vector<std::string> accountList();

    Account& config()
    {
        assert( m_currentAccountIndex >= 0 && m_currentAccountIndex < m_accounts.size() );
        return m_accounts[m_currentAccountIndex];
    }

    fs::path downloadFolder() { return config().m_downloadFolder; }

    ChannelInfo& currentChannelInfo()
    {
        assert( config().m_currentDnChannelIndex >= 0 && config().m_currentDnChannelIndex < config().m_dnChannels.size() );
        return config().m_dnChannels[config().m_currentDnChannelIndex];
    }

    ChannelInfo* currentChannelInfoPtr()
    {
        if ( config().m_currentDnChannelIndex < 0 && config().m_currentDnChannelIndex >= config().m_dnChannels.size() )
        {
            return nullptr;
        }
        return &config().m_dnChannels[config().m_currentDnChannelIndex];
    }

    void setCurrentAccountIndex( int currentAccountIndex )
    {
        qDebug() << "setCurrentAccountIndex: " << currentAccountIndex;
        m_currentAccountIndex = currentAccountIndex;
        assert( m_currentAccountIndex >= 0 && m_currentAccountIndex < m_accounts.size() );
    }

    void onDownloadCompleted( lt::torrent_handle handle );

    void removeFromDownloads( int index );

private:
    friend class PrivKeyDialog;
    friend class SettingsDialog;

    Settings& operator=( const Settings& s ) = default;

    uint32_t                m_settingsVersion = 1;
    std::vector<Account>    m_accounts;
    int                     m_currentAccountIndex = -1;
    bool                    m_loaded = false;
};

inline Settings gSettings;
