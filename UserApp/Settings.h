#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#include <QObject>
#include <QRect>
#include <QDebug>

#include "crypto/KeyPair.h"
#include "drive/Utils.h"
#include "drive/Session.h"
#include "libtorrent/torrent_handle.hpp"

#include "Drive.h"
#include "DownloadChannel.h"
#include "DownloadInfo.h"
#include "Utils.h"
#include "Account.h"


class Settings : public QObject
{
    Q_OBJECT

    public:
        std::string     m_restBootstrap;
        std::string     m_replicatorBootstrap;
        std::string     m_udpPort  = "6846";
        double          m_feeMultiplier = 150000;
        QRect           m_windowGeometry;
        bool            m_isDriveStructureAsTree;

    public:
        explicit Settings(QObject* parent = nullptr);
        ~Settings();
        Settings( const Settings& s );
        Settings& operator=( const Settings& s );

    public:
        void initForTests();
        bool load( const std::string& pwd );
        void save();
        bool loaded() const;
        std::vector<std::string> accountList();
        Account& config();
        fs::path downloadFolder();
        void setCurrentAccountIndex( int currentAccountIndex );
        void onDownloadCompleted( lt::torrent_handle handle );
        void removeFromDownloads( int index );

    signals:
        void downloadError(const QString& message);

    private:
        friend class PrivKeyDialog;
        friend class SettingsDialog;

        std::vector<Account>    m_accounts;
        int                     m_currentAccountIndex = -1;
        bool                    m_loaded = false;
        fs::path                m_configPath;
};
