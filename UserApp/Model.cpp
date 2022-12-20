#include "Model.h"
#include "Diff.h"
#include "Drive.h"
#include "LocalDriveItem.h"
#include "StorageEngine.h"
#include "utils/HexParser.h"
#include "Settings.h"

#include <boost/algorithm/string.hpp>

#if ! __MINGW32__
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
#endif

Model::Model(Settings* settings, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
{
}

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
    return m_settings->load("");
}

void Model::saveSettings()
{
    return m_settings->save();
}

fs::path Model::getDownloadFolder()
{
    return m_settings->downloadFolder();
}

std::string Model::getAccountName() {
    return m_settings->config().m_accountName;
}

std::string Model::getBootstrapReplicator() {
    return m_settings->m_replicatorBootstrap;
}

void Model::setCurrentAccountIndex(int index) {
    m_settings->setCurrentAccountIndex(index);
}

void Model::setBootstrapReplicator(const std::string& address) {
    m_settings->m_replicatorBootstrap = address;
}

std::string Model::getUdpPort() {
    return m_settings->m_udpPort;
}

std::string Model::getGatewayIp() {
    const auto gatewayEndpoint = QString(m_settings->m_restBootstrap.c_str()).split(":");
    return gatewayEndpoint.size() == 2 ? gatewayEndpoint[0].toStdString() : "";
}

std::string Model::getGatewayPort() {
    const auto gatewayEndpoint = QString(m_settings->m_restBootstrap.c_str()).split(":");
    return gatewayEndpoint.size() == 2 ? gatewayEndpoint[1].toStdString() : "";
}

sirius::crypto::KeyPair& Model::getKeyPair() {
    return m_settings->config().m_keyPair.value();
}

void Model::onDownloadCompleted(lt::torrent_handle handle) {
    m_settings->onDownloadCompleted(handle);
}

void Model::addDownloadChannel(const DownloadChannel &channel) {
    m_settings->config().m_dnChannels.push_back(channel);
}

std::vector<DownloadChannel> Model::getDownloadChannels()
{
    return m_settings->config().m_dnChannels;
}

void Model::setCurrentDownloadChannelIndex(int index )
{
    m_settings->config().m_currentDnChannelIndex = index;
}

int Model::currentDownloadChannelIndex()
{
    return m_settings->config().m_currentDnChannelIndex;
}

std::vector<DownloadInfo>& Model::downloads()
{
    return m_settings->config().m_downloads;
}

void Model::markChannelsForDelete(const std::string& driveId, bool state) {
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
    auto drive = findDrive(driveId);
    if (drive) {
        auto& channels = m_settings->config().m_dnChannels;
        for (auto& channel : channels) {
            if (boost::iequals( channel.getDriveKey(), drive->getKey() )) {
                channel.setDeleting(state);
            }
        }
    }
}

bool Model::isDownloadChannelsLoaded() {
    return m_settings->config().m_channelsLoaded;
}

void Model::setDownloadChannelsLoaded(bool state) {
    m_settings->config().m_channelsLoaded = state;
}

void Model::setCurrentDriveIndex( int index )
{
    qDebug() << LOG_SOURCE << "setCurrentDriveIndex: " << index;
    if ( index < 0 || index >= m_settings->config().m_drives.size() )
    {
        qDebug() << LOG_SOURCE << "setCurrentDriveIndex: invalid index: " << index;
    }
    else
    {
        m_settings->config().m_currentDriveIndex = index;
    }
}

int Model::currentDriveIndex()
{
    return m_settings->config().m_currentDriveIndex;
}

QRect Model::getWindowGeometry() const {
    return m_settings->m_windowGeometry;
}

void Model::setWindowGeometry(const QRect &geometry) {
    m_settings->m_windowGeometry = geometry;
}

void Model::initForTests() {
    m_settings->initForTests();
}

std::string Model::getClientPublicKey() {
    return m_settings->config().m_publicKeyStr;
}

std::string Model::getClientPrivateKey() {
    return m_settings->config().m_privateKeyStr;
}

bool Model::isDrivesLoaded() {
    return m_settings->config().m_drivesLoaded;
}

void Model::setDrivesLoaded(bool state) {
    m_settings->config().m_drivesLoaded = state;
}

void Model::addDrive(const Drive& drive) {
    m_settings->config().m_drives.push_back(drive);

    const auto index = (int)m_settings->config().m_drives.size() - 1;
    onDriveStateChanged(m_settings->config().m_drives[index]);
}

void Model::onMyOwnChannelsLoaded(const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& channelsPages )
{
    std::vector<xpx_chain_sdk::DownloadChannel> remoteChannels;
    for (const auto& page : channelsPages) {
        std::copy(page.data.channels.begin(), page.data.channels.end(), std::back_inserter(remoteChannels));
    }

    auto& localChannels = m_settings->config().m_dnChannels;
    std::vector<DownloadChannel> validChannels;
    for (auto &remoteChannel : remoteChannels) {
        std::string hash = remoteChannel.data.id;
        auto it = std::find_if(localChannels.begin(), localChannels.end(), [&hash](const auto &channel) {
            return boost::iequals(channel.getKey(), hash);
        });

        // add new channel
        if (it == localChannels.end()) {
            DownloadChannel channel;
            channel.setName(remoteChannel.data.id);
            channel.setKey(remoteChannel.data.id);
            channel.setAllowedPublicKeys(remoteChannel.data.listOfPublicKeys);
            channel.setDriveKey(remoteChannel.data.drive);
            channel.setCreating(false);
            channel.setDeleting(false);
            auto creationTime = std::chrono::steady_clock::now(); //todo
            channel.setCreatingTimePoint(creationTime);
            validChannels.push_back(channel);
        } else {
            // update valid channel
            validChannels.push_back(*it);
        }
    }

    // remove closed channels from saved
    m_settings->config().m_dnChannels = validChannels;
    m_settings->save();
}

void Model::onSponsoredChannelsLoaded(const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& remoteChannelsPages) {
    std::vector<xpx_chain_sdk::DownloadChannel> remoteChannels;
    for (const auto& page : remoteChannelsPages) {
        std::copy(page.data.channels.begin(), page.data.channels.end(), std::back_inserter(remoteChannels));
    }

    auto& localChannels = m_settings->config().m_dnChannels;
    for (auto &remoteChannel : remoteChannels) {
        std::string hash = remoteChannel.data.id;
        auto it = std::find_if(localChannels.begin(), localChannels.end(), [&hash](const auto &channel) {
            return boost::iequals(channel.getKey(), hash);
        });

        // add new channel
        if (it == localChannels.end()) {
            DownloadChannel channel;
            channel.setName(remoteChannel.data.id);
            channel.setKey(remoteChannel.data.id);
            channel.setAllowedPublicKeys(remoteChannel.data.listOfPublicKeys);
            channel.setDriveKey(remoteChannel.data.drive);
            channel.setCreating(false);
            channel.setDeleting(false);
            auto creationTime = std::chrono::steady_clock::now(); //todo
            channel.setCreatingTimePoint(creationTime);
            m_settings->config().m_dnChannels.push_back(channel);
        }
    }

    m_settings->save();
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

    auto& drives = m_settings->config().m_drives;

    for( auto& remoteDrive : remoteDrives )
    {
        std::string hash = remoteDrive.data.multisig;
        auto it = std::find_if( drives.begin(), drives.end(), [&hash] (const auto& driveInfo)
        {
            return boost::iequals( driveInfo.getKey(), hash );
        });

        if ( it == drives.end() )
        {
            Drive drive;
            drive.setKey(remoteDrive.data.multisig);
            drive.setName(remoteDrive.data.multisig);
            drive.setLocalFolder("");
            drive.setSize(remoteDrive.data.size);
            drive.setReplicatorsCount(remoteDrive.data.replicatorCount);
            drives.push_back(drive);
            it = drives.end() - 1;
        }

        it->updateState(no_modifications);
        it->setReplicatorsCount(remoteDrive.data.replicatorCount);

        if ( ! remoteDrive.data.activeDataModifications.empty() )
        {
            qWarning () << LOG_SOURCE << "drive modification status: is_registring: " << it->getName();
            auto& lastModification = remoteDrive.data.activeDataModifications.back();
            it->setModificationHash(Model::hexStringToHash( lastModification.dataModification.id ));
            //TODO load torrent and other data to session to continue, check cancel modifications also
            it->updateState(uploading);
        }
    }

    for (auto& d : drives)
    {
        auto it = std::find_if( remoteDrives.begin(), remoteDrives.end(), [&d] (const auto& driveInfo)
        {
            return boost::iequals( driveInfo.data.multisig, d.getKey() );
        });

        if ( it == remoteDrives.end() )
        {
            d.updateState(unconfirmed);
        }

        onDriveStateChanged(d);
    }

    m_settings->save();
}

std::vector<Drive> Model::getDrives()
{
    return m_settings->config().m_drives;
}

Drive* Model::currentDrive()
{
    if ( m_settings->config().m_currentDriveIndex >= 0 && m_settings->config().m_currentDriveIndex < m_settings->config().m_drives.size() )
    {
        return & m_settings->config().m_drives[ m_settings->config().m_currentDriveIndex ];
    }
    return nullptr;
}

Drive* Model::findDrive( const std::string& driveKey )
{
    auto& drives = m_settings->config().m_drives;
    auto it = std::find_if( drives.begin(), drives.end(), [&driveKey] (const Drive& drive) {
        return boost::iequals( drive.getKey(), driveKey );
    });

    return it == drives.end() ? nullptr : &(*it);
}

Drive* Model::findDriveByModificationId( const std::array<uint8_t, 32>& modificationId )
{
    auto& drives = m_settings->config().m_drives;
    auto it = std::find_if( drives.begin(), drives.end(), [&modificationId] (const Drive& drive) {
        return *drive.getModificationHash() == modificationId;
    });

    return it == drives.end() ? nullptr : &(*it);
}


void Model::removeDrive( const std::string& driveKey )
{
    auto& drives = m_settings->config().m_drives;

    auto it = std::find_if( drives.begin(), drives.end(), [driveKey] (const auto& drive)
    {
        return boost::iequals( driveKey, drive.getKey() );
    });

    if ( it == drives.end() )
    {
        return;
    }

    drives.erase(it);

    if ( m_settings->config().m_currentDriveIndex >= drives.size() )
    {
        m_settings->config().m_currentDriveIndex = (int)drives.size() - 1;
    }

    m_settings->save();
}

void Model::removeChannelByDriveKey(const std::string &driveKey) {
    auto& channels = m_settings->config().m_dnChannels;
    auto begin = channels.begin();
    while(begin != channels.end()) {
        if (boost::iequals( begin->getDriveKey(), driveKey )) {
            begin = channels.erase(begin);
        } else {
            ++begin;
        }
    }
}

void Model::applyForDrive( const std::string& driveKey, std::function<void(Drive&)> func )
{
    auto& drives = m_settings->config().m_drives;

    auto it = std::find_if( drives.begin(), drives.end(), [driveKey] (const auto& drive)
    {
        return boost::iequals( driveKey, drive.getKey() );
    });

    if ( it != drives.end() )
    {
        func( *it );
    }
}

void Model::removeFromDownloads( int rowIndex )
{
    m_settings->removeFromDownloads(rowIndex);
}

DownloadChannel* Model::currentDownloadChannel()
{
    return m_settings->currentChannelInfoPtr();
}

DownloadChannel* Model::findChannel( const std::string& channelKey )
{
    auto& channels = m_settings->config().m_dnChannels;
    auto it = std::find_if( channels.begin(), channels.end(), [channelKey] (const auto& channelInfo)
    {
        return boost::iequals( channelKey, channelInfo.getKey() );
    });

    return it == channels.end() ? nullptr : &(*it);
}

void Model::removeChannel( const std::string& channelKey )
{
    auto& channels = m_settings->config().m_dnChannels;
    auto it = std::find_if( channels.begin(), channels.end(), [channelKey] (const auto& channelInfo)
    {
        return boost::iequals( channelKey, channelInfo.getKey() );
    });

    if ( it == channels.end() )
    {
        return;
    }

    channels.erase(it);

    if ( m_settings->config().m_currentDnChannelIndex >= channels.size() )
    {
        m_settings->config().m_currentDnChannelIndex = (int)channels.size() - 1;
    }

    m_settings->save();
}

void Model::applyForChannels(const std::string &driveKey, std::function<void(DownloadChannel &)> callback) {
    for( auto& channel : m_settings->config().m_dnChannels )
    {
        if ( boost::iequals( channel.getDriveKey(), driveKey ) )
        {
            callback(channel);
        }
    }
}

void Model::applyFsTreeForChannels( const std::string& driveKey, const sirius::drive::FsTree& fsTree, const std::array<uint8_t, 32>& fsTreeHash )
{
    for( auto& channel : m_settings->config().m_dnChannels )
    {
        if ( boost::iequals( channel.getDriveKey(), driveKey ) )
        {
            channel.setFsTree(fsTree);
            channel.setFsTreeHash(fsTreeHash);
            channel.setWaitingFsTree(false);
        }
    }
}

void Model::startStorageEngine( std::function<void()> addressAlreadyInUseHandler )
{
    gStorageEngine = std::make_shared<StorageEngine>(this);
    gStorageEngine->start( addressAlreadyInUseHandler );
}

void Model::endStorageEngine()
{
    gStorageEngine.reset();
}

void Model::downloadFsTree( const std::string&             driveHash,
                            const std::string&             dnChannelId,
                            const std::array<uint8_t,32>&  fsTreeHash,
                            std::function<void( const std::string&           driveHash,
                                                const std::array<uint8_t,32> fsTreeHash,
                                                const sirius::drive::FsTree& fsTree )> onFsTreeReceived )
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
    auto& drives = m_settings->config().m_drives;

    auto it = std::find_if( drives.begin(), drives.end(), [driveHash] (const auto& drive)
    {
        return boost::iequals( driveHash, drive.getKey() );
    });

    if ( it == drives.end() )
    {
        return;
    }

    it->setRootHash(fsTreeHash);
    it->setFsTree(fsTree);
}

void Model::calcDiff()
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto* drive = currentDrive();
    if ( drive == nullptr )
    {
        qDebug() << LOG_SOURCE << "currentDrivePtr() == nullptr";
        return;
    }

    std::array<uint8_t,32> driveKey;
    sirius::utils::ParseHexStringIntoContainer( drive->getKey().c_str(), 64, driveKey );

    qDebug() << LOG_SOURCE << "calcDiff: localDriveFolder: " << drive->getLocalFolder();
    if ( ! isFolderExists(drive->getLocalFolder()) )
    {
        drive->setLocalFolderExists(false);
        drive->setActionsList({});
    }
    else
    {
        drive->setLocalFolderExists(true);

        auto localDrive = std::make_shared<LocalDriveItem>();
        Diff::calcLocalDriveInfoR( *localDrive, drive->getLocalFolder(), true, &driveKey );

        sirius::drive::ActionList actionList;
        Diff diff( *localDrive, drive->getLocalFolder(), drive->getFsTree(), driveKey, actionList);
        drive->setActionsList(actionList);

        drive->setLocalDriveItem(std::move(localDrive));
    }
}

std::array<uint8_t,32> Model::hexStringToHash( const std::string& str )
{
    std::array<uint8_t,32> hash;
    sirius::utils::ParseHexStringIntoContainer( str.c_str(), 64, hash );
    return hash;
}

void Model::onDriveStateChanged(const Drive& drive) {
    QObject::connect(&drive, &Drive::stateChanged, [this](auto driveKey, auto state)
    {
        emit driveStateChanged(driveKey, state);
        if (state == unconfirmed)
        {
            removeDrive(driveKey);
            removeChannelByDriveKey(driveKey);
        }
    });
}

void Model::stestInitChannels()
{
    m_settings->config().m_dnChannels.clear();

    DownloadChannel channel1;
    channel1.setName("my_channel");
    channel1.setKey("0101010100000000000000000000000000000000000000000000000000000000");
    channel1.setDriveKey("0100000000050607080900010203040506070809000102030405060708090001");
    m_settings->config().m_dnChannels.push_back( channel1 );

    DownloadChannel channel2;
    channel2.setName("my_channel2");
    channel2.setKey("0202020200000000000000000000000000000000000000000000000000000000");
    channel2.setDriveKey("0200000000050607080900010203040506070809000102030405060708090001");
    m_settings->config().m_dnChannels.push_back( channel2 );
    m_settings->config().m_currentDnChannelIndex = 0;
}

void Model::stestInitDrives()
{
    m_settings->config().m_drives.clear();
    std::array<uint8_t,32> driveKey{1,0,0,0,0,5,6,7,8,9, 0,1,2,3,4,5,6,7,8,9, 0,1,2,3,4,5,6,7,8,9, 0,1};

    Drive d1;
    d1.setKey( sirius::drive::toString(driveKey));
    d1.setName("drive1");
    d1.setLocalFolder("/Users/alex/000-drive1");
    m_settings->config().m_drives.push_back(d1);

    std::array<uint8_t,32> driveKey2{2,0,0,0,0,5,6,7,8,9, 0,1,2,3,4,5,6,7,8,9, 0,1,2,3,4,5,6,7,8,9, 0,1};

    Drive d2;
    d2.setKey(sirius::drive::toString(driveKey2));
    d2.setName("drive2");
    d2.setLocalFolder("/Users/alex/000-drive2");
    m_settings->config().m_drives.push_back(d2);

    m_settings->config().m_currentDriveIndex = 0;

    //todo!!!
//    LocalDriveItem root;
//    Model::scanFolderR( m_settings->config().m_drives[0].m_localDriveFolder, root );
//    LocalDriveItem root2;
//    Model::scanFolderR( m_settings->config().m_drives[0].m_localDriveFolder, root2 );
//    calcDiff( m_settings->config().m_drives[0].m_localDriveFolder, "", root, root2 );
}
