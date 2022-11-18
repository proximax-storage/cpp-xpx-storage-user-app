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

#include "Model.h"
#include "Utils.h"

std::filesystem::path settingsFolder();

inline std::filesystem::path fsTreesFolder() { return settingsFolder() / "FsTrees";}

inline QDebug& operator<<(QDebug& out, const std::string& str) { out << QString::fromStdString(str); return out; }


class Settings
{
public:

    std::string     m_restBootstrap;
    std::string     m_replicatorBootstrap;
    std::string     m_udpPort  = "6846";

    struct Account
    {
        std::string                 m_accountName;

        std::string                 m_privateKeyStr;
        std::string                 m_publicKeyStr;

        std::vector<ChannelInfo>    m_dnChannels;
        int                         m_currentDnChannelIndex = -1;

        std::string                 m_downloadFolder;
        std::vector<DownloadInfo>   m_downloads;

        std::vector<DriveInfo>      m_drives;
        int                         m_currentDriveIndex = -1;

        Account();
        Account( const Account& a );
        Account& operator=( const Account& a );


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
                m_currentDriveIndex );
        }

        std::optional<sirius::crypto::KeyPair>  m_keyPair;

        void initAccount( std::string name, std::string privateKeyStr )
        {
            m_accountName   = name;
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

    void initForTests();

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
        qDebug() << LOG_SOURCE << "setCurrentAccountIndex: " << currentAccountIndex;
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
