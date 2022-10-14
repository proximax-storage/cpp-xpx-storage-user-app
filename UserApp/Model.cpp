#include "Model.h"
#include "Settings.h"
#include "StorageEngine.h"
#include "utils/HexParser.h"

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

std::vector<DriveInfo>& Model::drives()
{
    return gSettings.config().m_drives;
}

DriveInfo* Model::currentDriveInfoPtr()
{
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
        return driveHash==driveInfo.m_driveHash;
    });

    if ( it == drives.end() )
    {
        return;
    }

    it->m_fsTree = fsTree;
    //todo

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
