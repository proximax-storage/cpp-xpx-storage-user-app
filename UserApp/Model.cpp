#include "Model.h"
#include "Diff.h"
#include "LocalDriveItem.h"
#include "Settings.h"
#include "StorageEngine.h"
#include "utils/HexParser.h"

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

void Model::onChannelLoaded( const std::string& channelKey,
                             const std::string& driveKey,
                             const std::vector<std::string>& listOfPublicKeys )
{
    auto& channels = gSettings.config().m_dnChannels;

    auto it = std::find_if( channels.begin(), channels.end(), [&channelKey] (const auto& channelInfo)
    {
        return channelInfo.m_hash == channelKey;
    });

    if ( it == channels.end() )
    {
        auto creationTime = std::chrono::steady_clock::now(); //todo

        gSettings.config().m_dnChannels.emplace_back( ChannelInfo{ channelKey, channelKey, driveKey, listOfPublicKeys, false, false, creationTime } );
    }
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
        qDebug() << "setCurrentDriveIndex: invalid index: " << index;
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

void Model::onDrivesLoaded( const QStringList& driveList )
{
    auto& drives = gSettings.config().m_drives;

    for( auto& driveHash : driveList )
    {
        std::string hash = driveHash.toStdString();

        auto it = std::find_if( drives.begin(), drives.end(), [&hash] (const auto& driveInfo)
        {
            return driveInfo.m_driveKey == hash;
        });

        if ( it == drives.end() )
        {
            Model::addDrive( hash, hash, Model::homeFolder() / hash );
        }
    }
}

std::vector<DriveInfo>& Model::drives()
{
    return gSettings.config().m_drives;
}

DriveInfo* Model::currentDriveInfoPtr()
{
    qDebug() << "currentDriveIndex: " << gSettings.config().m_currentDriveIndex;

    if ( gSettings.config().m_currentDriveIndex >= 0 && gSettings.config().m_currentDriveIndex < gSettings.config().m_drives.size() )
    {
        return & gSettings.config().m_drives[ gSettings.config().m_currentDriveIndex ];
    }
    return nullptr;
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
        return channelKey==channelInfo.m_hash;
    });

    if ( it == channels.end() )
    {
        return nullptr;
    }

    return &(*it);
}

void Model::startStorageEngine()
{
    gStorageEngine = std::make_shared<StorageEngine>();
    gStorageEngine->start();
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

void Model::onFsTreeForDriveReceived( const std::string&           driveHash,
                                      const std::array<uint8_t,32> fsTreeHash,
                                      const sirius::drive::FsTree& fsTree )
{
    auto& drives = Model::drives();

    auto it = std::find_if( drives.begin(), drives.end(), [driveHash] (const auto& driveInfo)
    {
        return driveHash==driveInfo.m_driveKey;
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
        qDebug() << "currentDriveInfoPtr() == nullptr";
        return;
    }

    std::array<uint8_t,32> driveKey;
    sirius::utils::ParseHexStringIntoContainer( drive->m_driveKey.c_str(), 64, driveKey );


    auto localDrive = std::make_shared<LocalDriveItem>();
    Diff::calcLocalDriveInfoR( *localDrive, drive->m_localDriveFolder, true, &driveKey );

    Diff diff( *localDrive, drive->m_localDriveFolder, drive->m_fsTree, driveKey, drive->m_actionList );

    drive->m_localDrive = std::move(localDrive);
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
