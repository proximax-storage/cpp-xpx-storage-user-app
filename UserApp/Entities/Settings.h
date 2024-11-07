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

#ifndef WA_APP
#include "drive/Session.h"
#include "libtorrent/torrent_handle.hpp"
#endif

#include "Drive.h"
#include "DownloadChannel.h"
#include "DownloadInfo.h"
#include "Utils.h"
#include "Account.h"

class Model;

class Settings : public QObject
{
    Q_OBJECT

    public:
        std::string     m_restBootstrap;
        std::string     m_replicatorBootstrap;
        std::string     m_udpPort  = "6846";
        double          m_feeMultiplier = 150000;
        QRect           m_windowGeometry;
        bool            m_isDriveStructureAsTree = false;

        // pre-defined settings
        const QString SIRIUS_MAINNET = "Sirius Mainnet";
        const QString SIRIUS_TESTNET = "Sirius Testnet 2";
        std::vector<std::tuple<QString, QString, QString>> MAINNET_API_NODES{   { "arcturus.xpxsirius.io", "3000", "" },
                                                                                { "aldebaran.xpxsirius.io", "3000", "" },
                                                                                { "betelgeuse.xpxsirius.io", "3000", "" },
                                                                                { "bigcalvin.xpxsirius.io", "3000", "" },
                                                                                { "delphinus.xpxsirius.io", "3000", "" },
                                                                                { "lyrasithara.xpxsirius.io", "3000", "" } };

        std::vector<std::tuple<QString, QString, QString>> MAINNET_REPLICATORS {   { "replicator-1.dfms.io","7904", "" },
                                                                                   { "replicator-2.dfms.io","7904", "" },
                                                                                   { "replicator-3.dfms.io","7904", "" },
                                                                                   { "replicator-4.dfms.io","7904", "" },
                                                                                   { "replicator-5.dfms.io","7904", "" },
                                                                                   { "replicator-6.dfms.io","7904", "" },
                                                                                   { "replicator-7.dfms.io","7904", "" },
                                                                                   { "replicator-8.dfms.io","7904", "" },
                                                                                   { "replicator-9.dfms.io","7904", "" },
                                                                                   { "replicator-10.dfms.io","7904", "" } };

        std::vector<std::tuple<QString, QString, QString>> TESTNET_API_NODES{ { "api-1.testnet2.xpxsirius.io", "3000", "" },
                                                                              { "api-2.testnet2.xpxsirius.io", "3000", "" } };

        std::vector<std::tuple<QString, QString, QString>> TESTNET_REPLICATORS{   { "genesis-p2p-2.testnet2.xpxsirius.io", "7904", "" },
                                                                                  { "genesis-p2p-3.testnet2.xpxsirius.io", "7904", "" },
                                                                                  { "apsouth1-p2p-2.testnet2.xpxsirius.io", "7904", "" },
                                                                                  { "apsouth1-p2p-3.testnet2.xpxsirius.io", "7904" , ""} };

    public:
        explicit Settings(QObject* parent = nullptr);
        ~Settings();
        Settings( const Settings& s );
        Settings& operator=( const Settings& s );

    public:
        void initForTests();
        bool load( const std::string& pwd );
        void saveSettings();
        bool loaded() const;
        std::vector<std::string> accountList();
        Account& accountConfig();
        fs::path downloadFolder();
        void setCurrentAccountIndex( int currentAccountIndex );
#ifndef WA_APP
        void onDownloadCompleted( lt::torrent_handle handle, Model& model );
#endif
        void removeFromDownloads( int index );
        void resolveBootstrapEndpoints();

    signals:
        void downloadError(const QString& message);

    private:
        static void resolveEndpointsList(std::vector<std::tuple<QString, QString, QString>>& endpoints);

    private:
        friend class PrivKeyDialog;
        friend class SettingsDialog;

        std::vector<Account>    m_accounts;
        int                     m_currentAccountIndex = -1;
        bool                    m_loaded = false;
        fs::path                m_configPath;
    
    public:
        // It is used for proper removing downloaded torrents
        //
        struct DownloadTorrentItem
        {
            //std::array<uint8_t,32>   m_hash; // key
            std::filesystem::path    m_path; // last saved (or moved) path
            sirius::drive::lt_handle m_ltHandle;
            int                      m_useCounter = 0;
        };
        std::map<std::array<uint8_t,32>,DownloadTorrentItem> m_downloadTorrentMap;
    
    sirius::drive::lt_handle addDownloadTorrent(  Model&                          model,
                                                  const std::string&              channelIdStr,
                                                  const std::array<uint8_t, 32>&  fileHash,
                                                  std::filesystem::path           outputFolder );
    
        void onTorrentDownloaded( Model&                          model,
                                  const std::array<uint8_t, 32>&  fileHash,
                                  std::filesystem::path           destinationFilename );
    
        void onDownloadCanceled( Model&                          model,
                                 const std::array<uint8_t, 32>&  fileHash );
};
