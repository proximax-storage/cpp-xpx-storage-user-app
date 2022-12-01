#include "Model.h"
#include "Diff.h"
#include "LocalDriveItem.h"
#include "Settings.h"
#include "StorageEngine.h"
#include "utils/HexParser.h"
#include "OnChainClient.h"

#include <boost/algorithm/string.hpp>

#if ! __MINGW32__
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
#endif

bool Model::isZeroHash( const std::array<uint8_t,32>& hash )
{
    static const std::array<uint8_t,32> zeroHash = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 };
    return hash == zeroHash;
}

fs::path Model::homeFolder()
{
#if __MINGW32__
    //todo: not tested
    return getenv("HOMEPATH");
#else
    const char *homedir;

    if ( const char* homedir = getenv("HOME"); homedir != nullptr )
    {
        return homedir;
    }

    return getpwuid(getuid())->pw_dir;
#endif
}

bool Model::loadSettings()
{
    return gSettings.load("");
}

void Model::saveSettings()
{
    return gSettings.save();
}

fs::path Model::downloadFolder()
{
    return gSettings.downloadFolder();
}

std::vector<ChannelInfo>& Model::dnChannels()
{
    return gSettings.config().m_dnChannels;
}

void Model::setCurrentDnChannelIndex( int index )
{
    gSettings.config().m_currentDnChannelIndex = index;
}

int Model::currentDnChannelIndex()
{
    return gSettings.config().m_currentDnChannelIndex;
}

std::vector<DownloadInfo>& Model::downloads()
{
    return gSettings.config().m_downloads;
}

void Model::addDrive( const std::string& driveHash,
                      const std::string& driveName,
                      const std::string& localFolder )
{
    gSettings.config().m_drives.emplace_back( DriveInfo{ driveHash, driveName, localFolder, 0, 0, false } );
}

void Model::setCurrentDriveIndex( int index )
{
    if ( index < 0 || index >= gSettings.config().m_drives.size() )
    {
        qDebug() << LOG_SOURCE << "setCurrentDriveIndex: invalid index: " << index;
    }
    else
    {
        gSettings.config().m_currentDriveIndex = index;
    }
}

int Model::currentDriveIndex()
{
    return gSettings.config().m_currentDriveIndex;
}

void Model::onMyOwnChannelsLoaded(const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& channelsPages )
{
    std::vector<xpx_chain_sdk::DownloadChannel> remoteChannels;
    for (const auto& page : channelsPages) {
        std::copy(page.data.channels.begin(), page.data.channels.end(), std::back_inserter(remoteChannels));
    }

    auto& localChannels = gSettings.config().m_dnChannels;
    std::vector<ChannelInfo> validChannels;
    for (auto &remoteChannel : remoteChannels) {
        std::string hash = remoteChannel.data.id;
        auto it = std::find_if(localChannels.begin(), localChannels.end(), [&hash](const auto &channelInfo) {
            return boost::iequals(channelInfo.m_hash, hash);
        });

        // add new channel
        if (it == localChannels.end()) {
            ChannelInfo channelInfo;
            channelInfo.m_name = remoteChannel.data.id;
            channelInfo.m_hash = remoteChannel.data.id;
            channelInfo.m_allowedPublicKeys = remoteChannel.data.listOfPublicKeys;
            channelInfo.m_driveHash = remoteChannel.data.drive;
            channelInfo.m_isCreating = false;
            channelInfo.m_isDeleting = false;
            auto creationTime = std::chrono::steady_clock::now(); //todo
            channelInfo.m_creationTimepoint = creationTime;
            validChannels.push_back(channelInfo);
        } else {
            // update valid channel
            validChannels.push_back(*it);
        }
    }

    // remove closed channels from saved
    gSettings.config().m_dnChannels = validChannels;
    gSettings.save();
}

void Model::onSponsoredChannelsLoaded(const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& remoteChannelsPages) {
    std::vector<xpx_chain_sdk::DownloadChannel> remoteChannels;
    for (const auto& page : remoteChannelsPages) {
        std::copy(page.data.channels.begin(), page.data.channels.end(), std::back_inserter(remoteChannels));
    }

    auto& localChannels = gSettings.config().m_dnChannels;
    for (auto &remoteChannel : remoteChannels) {
        std::string hash = remoteChannel.data.id;
        auto it = std::find_if(localChannels.begin(), localChannels.end(), [&hash](const auto &channelInfo) {
            return boost::iequals(channelInfo.m_hash, hash);
        });

        // add new channel
        if (it == localChannels.end()) {
            ChannelInfo channelInfo;
            channelInfo.m_name = remoteChannel.data.id;
            channelInfo.m_hash = remoteChannel.data.id;
            channelInfo.m_allowedPublicKeys = remoteChannel.data.listOfPublicKeys;
            channelInfo.m_driveHash = remoteChannel.data.drive;
            channelInfo.m_isCreating = false;
            channelInfo.m_isDeleting = false;
            auto creationTime = std::chrono::steady_clock::now(); //todo
            channelInfo.m_creationTimepoint = creationTime;
            gSettings.config().m_dnChannels.push_back(channelInfo);
        }
    }

    gSettings.save();
}

void Model::onDrivesLoaded( const std::vector<xpx_chain_sdk::drives_page::DrivesPage>& remoteDrivesPages )
{
    qDebug() << LOG_SOURCE << "onDrivesLoaded: ";

    std::vector<xpx_chain_sdk::Drive> remoteDrives;
    for (const auto& page : remoteDrivesPages) {
        std::copy(page.data.drives.begin(), page.data.drives.end(), std::back_inserter(remoteDrives));
    }

    qDebug() << LOG_SOURCE << "onDrivesLoaded: " << remoteDrives.size();

    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto& drives = gSettings.config().m_drives;

    for( auto& remoteDrive : remoteDrives )
    {
        std::string hash = remoteDrive.data.multisig;
        auto it = std::find_if( drives.begin(), drives.end(), [&hash] (const auto& driveInfo)
        {
            return boost::iequals( driveInfo.m_driveKey, hash );
        });

        if ( it == drives.end() )
        {
            DriveInfo driveInfo;
            driveInfo.m_driveKey         = remoteDrive.data.multisig;
            driveInfo.m_name             = remoteDrive.data.multisig;
            driveInfo.m_localDriveFolder = Model::homeFolder() / hash;
            driveInfo.m_maxDriveSize     = remoteDrive.data.size;
            driveInfo.m_replicatorNumber = remoteDrive.data.replicatorCount;
            driveInfo.m_isCreating = false;
            driveInfo.m_isDeleting = false;
            drives.push_back(driveInfo);
            it = drives.end()-1;
        }

        it->m_isConfirmed = true;
        it->m_replicatorNumber = remoteDrive.data.replicatorCount;

        if ( ! remoteDrive.data.activeDataModifications.empty() )
        {
            qWarning () << LOG_SOURCE << "drive modification status: is_registring: " << it->m_name;
            auto& lastModification = remoteDrive.data.activeDataModifications.back();
            it->m_currentModificationHash = Model::hexStringToHash( lastModification.dataModification.id );
            //TODO
            it->m_modificationStatus = is_registring;
        }
    }

    std::erase_if( drives, [](const DriveInfo& drive) { return ! drive.m_isConfirmed; } );

    gSettings.save();
}

std::vector<DriveInfo>& Model::drives()
{
    return gSettings.config().m_drives;
}

DriveInfo* Model::currentDriveInfoPtr()
{
    qDebug() << LOG_SOURCE << "currentDriveIndex: " << gSettings.config().m_currentDriveIndex;

    if ( gSettings.config().m_currentDriveIndex >= 0 && gSettings.config().m_currentDriveIndex < gSettings.config().m_drives.size() )
    {
        return & gSettings.config().m_drives[ gSettings.config().m_currentDriveIndex ];
    }
    return nullptr;
}

DriveInfo* Model::findDrive( const std::string& driveKey )
{
    auto& drives = gSettings.config().m_drives;
    auto it = std::find_if( drives.begin(), drives.end(), [&driveKey] (const DriveInfo& info) {
        return boost::iequals( info.m_driveKey, driveKey );
    });

    return it == drives.end() ? nullptr : &(*it);
}

DriveInfo* Model::findDriveByModificationId( const std::array<uint8_t, 32>& modificationId )
{
    auto& drives = gSettings.config().m_drives;
    auto it = std::find_if( drives.begin(), drives.end(), [&modificationId] (const DriveInfo& info) {
        return *info.m_currentModificationHash == modificationId;
    });

    return it == drives.end() ? nullptr : &(*it);
}


void Model::removeDrive( const std::string& driveKey )
{
    auto& drives = gSettings.config().m_drives;

    auto it = std::find_if( drives.begin(), drives.end(), [driveKey] (const auto& driveInfo)
    {
        return boost::iequals( driveKey, driveInfo.m_driveKey );
    });

    if ( it == drives.end() )
    {
        return;
    }

    drives.erase(it);

    if ( gSettings.config().m_currentDriveIndex >= drives.size() )
    {
        gSettings.config().m_currentDriveIndex = drives.size()-1;
    }

    gSettings.save();
}

void Model::removeChannelByDriveKey(const std::string &driveKey) {
    auto& channels = gSettings.config().m_dnChannels;
    auto begin = channels.begin();
    while(begin != channels.end()) {
        if (boost::iequals( begin->m_driveHash, driveKey )) {
            begin = channels.erase(begin);
        } else {
            ++begin;
        }
    }
}

void Model::applyForDrive( const std::string& driveKey, std::function<void(DriveInfo&)> func )
{
    auto& drives = gSettings.config().m_drives;

    auto it = std::find_if( drives.begin(), drives.end(), [driveKey] (const auto& driveInfo)
    {
        return boost::iequals( driveKey, driveInfo.m_driveKey );
    });

    if ( it != drives.end() )
    {
        func( *it );
    }
}

void Model::removeFromDownloads( int rowIndex )
{
    gSettings.removeFromDownloads(rowIndex);
}

ChannelInfo* Model::currentChannelInfoPtr()
{
    return gSettings.currentChannelInfoPtr();
}

ChannelInfo* Model::findChannel( const std::string& channelKey )
{
    auto& channels = gSettings.config().m_dnChannels;
    auto it = std::find_if( channels.begin(), channels.end(), [channelKey] (const auto& channelInfo)
    {
        return boost::iequals( channelKey, channelInfo.m_hash );
    });

    return it == channels.end() ? nullptr : &(*it);
}

void Model::removeChannel( const std::string& channelKey )
{
    auto& channels = gSettings.config().m_dnChannels;
    auto it = std::find_if( channels.begin(), channels.end(), [channelKey] (const auto& channelInfo)
    {
        return boost::iequals( channelKey, channelInfo.m_hash );
    });

    if ( it == channels.end() )
    {
        return;
    }

    channels.erase(it);

    if ( gSettings.config().m_currentDnChannelIndex >= channels.size() )
    {
        gSettings.config().m_currentDnChannelIndex = (int)channels.size() - 1;
    }

    gSettings.save();
}

void Model::applyForChannels( const std::string& driveKey, std::function<void(ChannelInfo&)> func )
{
    for( auto& channelInfo : gSettings.config().m_dnChannels )
    {
        if ( boost::iequals( channelInfo.m_driveHash, driveKey ) )
        {
            func( channelInfo );
        }
    }
}

void Model::startStorageEngine( std::function<void()> addressAlreadyInUseHandler )
{
    gStorageEngine = std::make_shared<StorageEngine>();
    gStorageEngine->start( addressAlreadyInUseHandler );
}

void Model::endStorageEngine()
{
    gStorageEngine.reset();
}

void Model::downloadFsTree( const std::string&             driveHash,
                            const std::string&             dnChannelId,
                            const std::array<uint8_t,32>&  fsTreeHash,
                            FsTreeHandler                  onFsTreeReceived )
{
    gStorageEngine->downloadFsTree( driveHash,
                                    dnChannelId,
                                    fsTreeHash,
                                    onFsTreeReceived );
}

sirius::drive::lt_handle Model::downloadFile( const std::string&            channelIdStr,
                                              const std::array<uint8_t,32>& fileHash )
{
    std::array<uint8_t,32> channelId;
    sirius::utils::ParseHexStringIntoContainer( channelIdStr.c_str(), 64, channelId );

    return gStorageEngine->downloadFile( channelId, fileHash );
}

//TODO: remove it
void Model::onFsTreeForDriveReceived( const std::string&           driveHash,
                                      const std::array<uint8_t,32> fsTreeHash,
                                      const sirius::drive::FsTree& fsTree )
{
    auto& drives = Model::drives();

    auto it = std::find_if( drives.begin(), drives.end(), [driveHash] (const auto& driveInfo)
    {
        return boost::iequals( driveHash, driveInfo.m_driveKey );
    });

    if ( it == drives.end() )
    {
        return;
    }

    it->m_rootHash  = fsTreeHash;
    it->m_fsTree    = fsTree;
}

void Model::calcDiff()
{
    auto* drive = currentDriveInfoPtr();

    if ( drive == nullptr )
    {
        qDebug() << LOG_SOURCE << "currentDriveInfoPtr() == nullptr";
        return;
    }

    std::array<uint8_t,32> driveKey;
    sirius::utils::ParseHexStringIntoContainer( drive->m_driveKey.c_str(), 64, driveKey );

    qDebug() << LOG_SOURCE << "calcDiff: localDriveFolder: " << drive->m_localDriveFolder;
    auto localDrive = std::make_shared<LocalDriveItem>();
    Diff::calcLocalDriveInfoR( *localDrive, drive->m_localDriveFolder, true, &driveKey );

    Diff diff( *localDrive, drive->m_localDriveFolder, drive->m_fsTree, driveKey, drive->m_actionList );

    drive->m_localDrive = std::move(localDrive);
}

std::array<uint8_t,32> Model::hexStringToHash( const std::string& str )
{
    std::array<uint8_t,32> hash;
    sirius::utils::ParseHexStringIntoContainer( str.c_str(), 64, hash );
    return hash;
}


void Model::stestInitChannels()
{
    gSettings.config().m_dnChannels.clear();
    gSettings.config().m_dnChannels.push_back( ChannelInfo{
                                                "my_channel",
                                                "0101010100000000000000000000000000000000000000000000000000000000",
                                                "0100000000050607080900010203040506070809000102030405060708090001"
                                                 } );
    gSettings.config().m_dnChannels.push_back( ChannelInfo{
                                                "my_channel2",
                                                "0202020200000000000000000000000000000000000000000000000000000000",
                                                "0200000000050607080900010203040506070809000102030405060708090001"
                                                 } );
    gSettings.config().m_currentDnChannelIndex = 0;
}

void Model::stestInitDrives()
{
    gSettings.config().m_drives.clear();
    std::array<uint8_t,32> driveKey{1,0,0,0,0,5,6,7,8,9, 0,1,2,3,4,5,6,7,8,9, 0,1,2,3,4,5,6,7,8,9, 0,1};
    gSettings.config().m_drives.push_back( DriveInfo{ sirius::drive::toString(driveKey), "drive1", "/Users/alex/000-drive1" });

    std::array<uint8_t,32> driveKey2{2,0,0,0,0,5,6,7,8,9, 0,1,2,3,4,5,6,7,8,9, 0,1,2,3,4,5,6,7,8,9, 0,1};
    gSettings.config().m_drives.push_back( DriveInfo{ sirius::drive::toString(driveKey2), "drive2", "/Users/alex/000-drive2" });

    gSettings.config().m_currentDriveIndex = 0;

    //todo!!!
//    LocalDriveItem root;
//    Model::scanFolderR( gSettings.config().m_drives[0].m_localDriveFolder, root );
//    LocalDriveItem root2;
//    Model::scanFolderR( gSettings.config().m_drives[0].m_localDriveFolder, root2 );
//    calcDiff( gSettings.config().m_drives[0].m_localDriveFolder, "", root, root2 );
}
