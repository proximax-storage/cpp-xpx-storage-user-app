#include "Model.h"
#include "Diff.h"
#include "Drive.h"
#include "LocalDriveItem.h"
#include "StorageEngine.h"
#include "utils/HexParser.h"
#include "drive/ViewerSession.h"
#include "Settings.h"

#include <QFileInfoList>
#include <QDirIterator>

#include <boost/algorithm/string.hpp>

#if !(__MINGW32__) && !(_MSC_VER)
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
#elif _MSC_VER
    #include <io.h>
    #include <sys/types.h>
    #include <windows.h>
#endif

Model::Model(Settings* settings, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_loadedDrivesCount(0)
    , m_outdatedDriveNumber(0)
{
}

bool Model::isZeroHash( const std::array<uint8_t,32>& hash )
{
    static const std::array<uint8_t,32> zeroHash = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 };
    return hash == zeroHash;
}

fs::path Model::homeFolder()
{
#if __MINGW32__ || _MSC_VER
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

double Model::getFeeMultiplier() {
    return m_settings->m_feeMultiplier;
}

const sirius::crypto::KeyPair& Model::getKeyPair() {
    return m_settings->config().m_keyPair.value();
}

void Model::onDownloadCompleted(lt::torrent_handle handle) {
    m_settings->onDownloadCompleted(handle);
}

void Model::addDownloadChannel(const DownloadChannel &channel) {
    m_settings->config().m_dnChannels.insert({ channel.getKey(), channel });
}

std::map<std::string, DownloadChannel>&  Model::getDownloadChannels()
{
    return m_settings->config().m_dnChannels;
}

void Model::setCurrentDownloadChannelKey(const std::string& channelKey)
{
    m_settings->config().m_currentDownloadChannelKey = channelKey;
}

std::string Model::currentDownloadChannelKey()
{
    return m_settings->config().m_currentDownloadChannelKey;
}

std::vector<DownloadInfo>& Model::downloads()
{
    return m_settings->config().m_downloads;
}

void Model::markChannelsForDelete(const std::string& driveId, bool state) {
    auto drive = findDrive(driveId);
    if (drive) {
        auto& channels = m_settings->config().m_dnChannels;
        for (auto& [key, channel] : channels) {
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

std::map<std::string, CachedReplicator> Model::getMyReplicators() const {
    return m_settings->config().m_myReplicators;
}

void Model::addMyReplicator(const CachedReplicator& replicator) {
    m_settings->config().m_myReplicators.insert_or_assign(replicator.getPrivateKey(), replicator);
}

void Model::removeMyReplicator(const std::string& replicatorKey) {
    m_settings->config().m_myReplicators.erase(replicatorKey);
}

void Model::setCurrentDriveKey( const std::string& driveKey )
{
    const auto driveKyeUpperCase = QString::fromStdString(driveKey).toUpper().toStdString();
    if ( !m_settings->config().m_drives.contains(driveKyeUpperCase) )
    {
        qWarning() << "Model::setCurrentDriveKey: invalid drive key: " << driveKyeUpperCase;
    }
    else
    {
        m_settings->config().m_currentDriveKey = driveKyeUpperCase;
    }
}

std::string Model::currentDriveKey()
{
    return m_settings->config().m_currentDriveKey;
}

bool Model::isDriveWithNameExists(const QString& driveName) const
{
    if (driveName.isEmpty()) {
        return false;
    }

    auto it = std::find_if( m_settings->config().m_drives.begin(), m_settings->config().m_drives.end(), [&driveName] (const auto& iterator) {
        return boost::iequals(iterator.second.getName(), driveName.toStdString());
    });

    return !(it == m_settings->config().m_drives.end());
}

bool Model::isChannelWithNameExists(const QString& channelName) const
{
    if (channelName.isEmpty()) {
        return false;
    }

    auto it = std::find_if( m_settings->config().m_dnChannels.begin(), m_settings->config().m_dnChannels.end(), [&channelName] (const auto& iterator) {
        return boost::iequals(iterator.second.getName(), channelName.toStdString());
    });

    return !(it == m_settings->config().m_dnChannels.end());
}

bool Model::isReplicatorWithNameExists(const QString& replicatorName) const
{
    if (replicatorName.isEmpty()) {
        return false;
    }

    auto it = std::find_if( m_settings->config().m_myReplicators.begin(), m_settings->config().m_myReplicators.end(), [&replicatorName] (const auto& iterator) {
        return boost::iequals(iterator.second.getName(), replicatorName.toStdString());
    });

    return !(it == m_settings->config().m_myReplicators.end());
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

void Model::setLoadedDrivesCount(uint64_t count) {
    m_loadedDrivesCount = count;
}

uint64_t Model::getLoadedDrivesCount() const {
    return m_loadedDrivesCount;
}

void Model::setOutdatedDriveNumber(uint64_t count) {
    m_outdatedDriveNumber = count;
}

uint64_t Model::getOutdatedDriveNumber() const {
    return m_outdatedDriveNumber;
}

void Model::addDrive(const Drive& drive) {
    m_settings->config().m_drives.insert({ drive.getKey(), drive });
}

void Model::onMyOwnChannelsLoaded(const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& channelsPages )
{
    std::vector<xpx_chain_sdk::DownloadChannel> remoteChannels;
    for (const auto& page : channelsPages) {
        std::copy(page.data.channels.begin(), page.data.channels.end(), std::back_inserter(remoteChannels));
    }

    auto& localChannels = m_settings->config().m_dnChannels;
    std::map<std::string, DownloadChannel>  validChannels;
    for (auto &remoteChannel : remoteChannels) {
        std::string hash = remoteChannel.data.id;
        auto it = std::find_if(localChannels.begin(), localChannels.end(), [&hash](const auto &channel) {
            return boost::iequals(channel.first, hash);
        });

        // add new channel
        if (it == localChannels.end()) {
            DownloadChannel channel;
            channel.setName(remoteChannel.data.id);
            channel.setKey(remoteChannel.data.id);
            channel.setAllowedPublicKeys(remoteChannel.data.listOfPublicKeys);
            channel.setDriveKey(remoteChannel.data.drive);
            channel.setReplicators(remoteChannel.data.shardReplicators);
            channel.setCreating(false);
            channel.setDeleting(false);
            auto creationTime = std::chrono::steady_clock::now();
            channel.setCreatingTimePoint(creationTime);
            validChannels.insert({ channel.getKey(), channel });
        } else {
            // update valid channel
            it->second.setCreating(false);
            it->second.setDeleting(false);
            validChannels.insert({ it->first, it->second });
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
            return boost::iequals(channel.first, hash);
        });

        // add new channel
        if (it == localChannels.end()) {
            DownloadChannel channel;
            channel.setName(remoteChannel.data.id);
            channel.setKey(remoteChannel.data.id);
            channel.setAllowedPublicKeys(remoteChannel.data.listOfPublicKeys);
            channel.setDriveKey(remoteChannel.data.drive);
            channel.setReplicators(remoteChannel.data.shardReplicators);
            channel.setCreating(false);
            channel.setDeleting(false);
            auto creationTime = std::chrono::steady_clock::now();
            channel.setCreatingTimePoint(creationTime);
            m_settings->config().m_dnChannels.insert({ channel.getKey(), channel });
        }
    }

    m_settings->save();
}

void Model::onDrivesLoaded( const std::vector<xpx_chain_sdk::drives_page::DrivesPage>& remoteDrivesPages )
{
    std::vector<xpx_chain_sdk::Drive> remoteDrives;
    for (const auto& page : remoteDrivesPages) {
        std::copy(page.data.drives.begin(), page.data.drives.end(), std::back_inserter(remoteDrives));
    }

    qDebug() << "Model::onDrivesLoaded: " << remoteDrives.size();
    auto& drives = m_settings->config().m_drives;

    for( auto& remoteDrive : remoteDrives )
    {
        if (drives.contains(remoteDrive.data.multisig)) {
            const auto driveKeyUpperCase = QString::fromStdString(remoteDrive.data.multisig).toUpper().toStdString();
            Drive& drive = drives[driveKeyUpperCase];
            drive.setReplicatorsCount(remoteDrive.data.replicatorCount);

            if ( !remoteDrive.data.replicators.empty() )
            {
                drive.setReplicators(remoteDrive.data.replicators);
                gStorageEngine->addReplicators( drive.getReplicators() );
            }

            drive.updateDriveState(creating);
            drive.updateDriveState(no_modifications);
        } else {
            Drive newDrive;
            newDrive.setKey(remoteDrive.data.multisig);
            newDrive.setName(remoteDrive.data.multisig);
            newDrive.setLocalFolder(homeFolder().string() + "/" + newDrive.getKey());
            newDrive.setSize(remoteDrive.data.size);
            newDrive.setReplicatorsCount(remoteDrive.data.replicatorCount);
            addDrive(newDrive);

            Drive& drive = drives[newDrive.getKey()];
            drive.setReplicatorsCount(remoteDrive.data.replicatorCount);

            if ( !remoteDrive.data.replicators.empty() )
            {
                drive.setReplicators(remoteDrive.data.replicators);
                gStorageEngine->addReplicators( drive.getReplicators() );
            }

            drive.updateDriveState(creating);
            drive.updateDriveState(no_modifications);
        }

        Drive& currentDrive = drives[QString::fromStdString(remoteDrive.data.multisig).toUpper().toStdString()];
        if ( ! remoteDrive.data.activeDataModifications.empty() &&
             ! remoteDrive.data.activeDataModifications[remoteDrive.data.activeDataModifications.size() - 1].dataModification.isStream) {

            auto lastModificationIndex = remoteDrive.data.activeDataModifications.size();
            auto lastModificationId = remoteDrive.data.activeDataModifications[lastModificationIndex - 1].dataModification.id;
            currentDrive.setModificationHash(Model::hexStringToHash( lastModificationId ));

            const std::string pathToDriveData = getSettingsFolder().string() + "/" + QString(currentDrive.getKey().c_str()).toUpper().toStdString() + "/modify_drive_data";
            bool isDirExists = QDir(pathToDriveData.c_str()).exists();
            if (isDirExists) {
                std::map<QString, QFileInfo> filesData;
                QFileInfoList torrentsList;
                QDirIterator it(pathToDriveData.c_str(), QDirIterator::Subdirectories);

                QRegularExpression nameTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9.torrent]{72})")));
                while (it.hasNext()) {
                    QString currentFileName = it.next();
                    QFileInfo file(currentFileName);

                    if (file.isDir()) {
                        continue;
                    }

                    if (nameTemplate.match(file.fileName()).hasMatch()) {
                        torrentsList.append(file);
                    } else {
                        filesData.insert_or_assign(file.fileName() + ".torrent", file);
                    }
                }

                for (const auto& t : torrentsList) {
                    if (filesData.contains(t.fileName())) {
                        emit addTorrentFileToStorageSession(t.fileName().toStdString(),
                                                            pathToDriveData,
                                                            rawHashFromHex(currentDrive.getKey().c_str()),
                                                            currentDrive.getModificationHash());
                    }
                }
            }

            currentDrive.updateDriveState(registering);
            currentDrive.updateDriveState(uploading);
        }
    }

    for (auto& d : drives)
    {
        auto it = std::find_if( remoteDrives.begin(), remoteDrives.end(), [&d] (const auto& driveInfo)
        {
            return boost::iequals( driveInfo.data.multisig, d.second.getKey() );
        });

        if ( it == remoteDrives.end() )
        {
            d.second.updateDriveState(unconfirmed);
        }
    }

    m_settings->save();
}

std::map<std::string, Drive>& Model::getDrives()
{
    return m_settings->config().m_drives;
}

Drive* Model::currentDrive()
{
    if (m_settings->config().m_drives.contains(m_settings->config().m_currentDriveKey))
    {
        return &m_settings->config().m_drives[m_settings->config().m_currentDriveKey];
    }

    return nullptr;
}

Drive* Model::findDrive( const std::string& driveKey )
{
    const auto driveKeyUpperCase = QString::fromStdString(driveKey).toUpper().toStdString();
    auto& drives = m_settings->config().m_drives;
    if (drives.contains(driveKeyUpperCase)) {
        return &drives[driveKeyUpperCase];
    }

    return nullptr;
}

Drive* Model::findDriveByNameOrPublicKey( const std::string& value )
{
    auto& drives = m_settings->config().m_drives;
    for (auto& drive : drives) {
        if (drive.second.getName() == value || boost::iequals(drive.second.getKey(), value)) {
            return &drive.second;
        }
    }

    return nullptr;
}

CachedReplicator Model::findReplicatorByNameOrPublicKey( const std::string& value )
{
    for (const auto& replicator : m_settings->config().m_myReplicators) {
        if (replicator.second.getName() == value || boost::iequals(replicator.second.getPublicKey(), value)) {
            return replicator.second;
        }
    }

    return {};
}

Drive* Model::findDriveByModificationId( const std::array<uint8_t, 32>& modificationId )
{
    auto& drives = m_settings->config().m_drives;
    auto it = std::find_if( drives.begin(), drives.end(), [&modificationId] (const auto& iterator) {
        return iterator.second.getModificationHash() == modificationId;
    });

    return it == drives.end() ? nullptr : &it->second;
}

void Model::removeDrive( const std::string& driveKey )
{
    const auto driveKeyUpperCase = QString::fromStdString(driveKey).toUpper().toStdString();
    auto& drives = m_settings->config().m_drives;
    drives.erase(driveKeyUpperCase);

    m_settings->save();
}

void Model::removeChannelsByDriveKey(const std::string &driveKey) {
    auto& channels = m_settings->config().m_dnChannels;
    auto begin = channels.begin();
    while(begin != channels.end()) {
        if (boost::iequals( begin->second.getDriveKey(), driveKey )) {
            begin = channels.erase(begin);
        } else {
            ++begin;
        }
    }
}

void Model::removeFromDownloads( int rowIndex )
{
    m_settings->removeFromDownloads( rowIndex );
}

DownloadChannel* Model::currentDownloadChannel()
{
    if (m_settings->config().m_dnChannels.contains(m_settings->config().m_currentDownloadChannelKey))
    {
        return &m_settings->config().m_dnChannels[m_settings->config().m_currentDownloadChannelKey];
    }

    return nullptr;
}

DownloadChannel* Model::findChannel( const std::string& channelKey )
{
    auto& channels = m_settings->config().m_dnChannels;
    auto it = std::find_if( channels.begin(), channels.end(), [channelKey] (const auto& channelInfo)
    {
        return boost::iequals( channelKey, channelInfo.first );
    });

    return it == channels.end() ? nullptr : &it->second;
}

CachedReplicator Model::findReplicatorByPublicKey(const std::string& replicatorPublicKey) const {
    auto it = std::find_if( m_settings->config().m_myReplicators.begin(), m_settings->config().m_myReplicators.end(), [replicatorPublicKey] (const auto& r)
    {
        return boost::iequals( replicatorPublicKey, r.second.getPublicKey() );
    });

    return it == m_settings->config().m_myReplicators.end() ? CachedReplicator() : it->second;
}

void Model::updateReplicatorAlias(const std::string& replicatorPublicKey, const std::string& alias) const {
    auto it = std::find_if( m_settings->config().m_myReplicators.begin(), m_settings->config().m_myReplicators.end(), [replicatorPublicKey] (const auto& r)
    {
        return boost::iequals( replicatorPublicKey, r.second.getPublicKey() );
    });

    if (it != m_settings->config().m_myReplicators.end()) {
        it->second.setName(alias);
    }
}

void Model::removeChannel( const std::string& channelKey )
{
    const auto channelKeyUpperCase = QString::fromStdString(channelKey).toUpper().toStdString();
    auto& channels = m_settings->config().m_dnChannels;
    channels.erase(channelKeyUpperCase);

    m_settings->save();
}

void Model::applyForChannels(const std::string &driveKey, std::function<void(DownloadChannel &)> callback) {
    for( auto& channel : m_settings->config().m_dnChannels )
    {
        if ( boost::iequals( channel.second.getDriveKey(), driveKey ) )
        {
            callback(channel.second);
        }
    }
}

void Model::applyFsTreeForChannels( const std::string& driveKey, const sirius::drive::FsTree& fsTree, const std::array<uint8_t, 32>& fsTreeHash )
{
    for( auto& channel : m_settings->config().m_dnChannels )
    {
        if ( boost::iequals( channel.second.getDriveKey(), driveKey ) )
        {
            channel.second.setFsTree(fsTree);
            channel.second.setFsTreeHash(fsTreeHash);
            channel.second.setDownloadingFsTree(false);
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

sirius::drive::lt_handle Model::downloadFile(const std::string &channelIdStr,
                                             const std::array<uint8_t, 32> &fileHash)
{
    std::array<uint8_t,32> channelId{ 0 };
    sirius::utils::ParseHexStringIntoContainer( channelIdStr.c_str(), 64, channelId );

    return gStorageEngine->downloadFile( channelId, fileHash );
}

void Model::calcDiff( Drive& drive )
{
    std::array<uint8_t,32> driveKey{};
    sirius::utils::ParseHexStringIntoContainer( drive.getKey().c_str(), 64, driveKey );

    qDebug() << LOG_SOURCE << "calcDiff: localDriveFolder: " << drive.getLocalFolder();
    if ( ! isFolderExists(drive.getLocalFolder()) )
    {
        drive.setLocalFolderExists(false);
        drive.setActionsList({});
    }
    else
    {
        drive.setLocalFolderExists(true);

        auto localDrive = std::make_shared<LocalDriveItem>();
        Diff::calcLocalDriveInfoR( *localDrive, drive.getLocalFolder(), true, &driveKey );
        sirius::drive::ActionList actionList;
        Diff diff( *localDrive, drive.getLocalFolder(), drive.getFsTree(), actionList);

        drive.setActionsList(actionList);
        drive.setLocalDriveItem(std::move(localDrive));
    }
}

std::array<uint8_t,32> Model::hexStringToHash( const std::string& str )
{
    std::array<uint8_t,32> hash{};
    sirius::utils::ParseHexStringIntoContainer( str.c_str(), 64, hash );
    return hash;
}

void Model::removeTorrentSync( sirius::drive::InfoHash infoHash )
{
    gStorageEngine->removeTorrentSync( infoHash );
}

const std::vector<StreamInfo>& Model::streamerAnnouncements() const
{
    return m_settings->config().m_streams;
}

std::vector<StreamInfo>& Model::streamerAnnouncements()
{
    return m_settings->config().m_streams;
}

std::string& Model::streamFolder()
{
    return m_settings->config().m_streamFolder;
}

void Model::addStreamRef( const StreamInfo& streamInfo )
{
    auto& streams = m_settings->config().m_streamRefs;
    streams.push_back( streamInfo );
    std::sort( streams.begin(), streams.end(), [] (const auto& s1, const auto& s2) ->bool {
        return s1.m_secsSinceEpoch > s2.m_secsSinceEpoch;
    });
    
    m_settings->save();
}

void Model::deleteStreamRef( int index )
{
    auto& streams = m_settings->config().m_streamRefs;
    streams.erase( streams.begin() + index );
    m_settings->save();
}

const std::vector<StreamInfo>& Model::streamRefs() const
{
    return m_settings->config().m_streamRefs;
}

void Model::setCurrentStreamInfo( std::optional<StreamInfo> info )
{
    m_settings->config().m_currentStreamInfo = info;
}

std::optional<StreamInfo> Model::currentStreamInfo() const
{
    return m_settings->config().m_currentStreamInfo;
}

const StreamInfo*  Model::getStreamRef( int index ) const
{
    if ( index >= 0 && index < m_settings->config().m_streamRefs.size() )
    {
        return &m_settings->config().m_streamRefs[index];
    }
    return nullptr;
}

StreamInfo*  Model::getStreamRef( int index )
{
    if ( index >= 0 && index < m_settings->config().m_streamRefs.size() )
    {
        return &m_settings->config().m_streamRefs[index];
    }
    return nullptr;
}

void Model::requestStreamStatus( const StreamInfo& streamInfo, StreamStatusResponseHandler streamStatusResponseHandler )
{
    if ( const auto* driveInfo = findDrive( streamInfo.m_driveKey ); driveInfo != nullptr )
    {
        gStorageEngine->requestStreamStatus( hexStringToHash(streamInfo.m_driveKey), driveInfo->replicatorList(), streamStatusResponseHandler );
    }
}

void Model::setModificationStatusResponseHandler( ModificationStatusResponseHandler handler )
{
    gStorageEngine->m_session->setModificationStatusResponseHandler( handler );
}

DriveContractModel& Model::driveContractModel() {
    return m_settings->config().m_driveContractModel;
};

void Model::requestModificationStatus(  const std::string&     replicatorKey,
                                        const std::string&     driveKey,
                                        const std::string&     modificationHash,
                                        std::optional<boost::asio::ip::tcp::endpoint> replicatorEndpoint )
{
//    if ( Model::homeFolder() == "/Users/alex" )
//    {
//        boost::asio::ip::tcp::endpoint endpoint{ boost::asio::ip::make_address("192.168.2.101"), 5001 };
//
//        gStorageEngine->m_session->sendModificationStatusRequestToReplicator( hexStringToHash(replicatorKey),
//                                                                              hexStringToHash(driveKey),
//                                                                              hexStringToHash(modificationHash),
//                                                                              endpoint );
//    }
//    else
//    {
        gStorageEngine->m_session->sendModificationStatusRequestToReplicator( hexStringToHash(replicatorKey),
                                                                              hexStringToHash(driveKey),
                                                                              hexStringToHash(modificationHash),
                                                                             replicatorEndpoint );
//    }
}

void Model::stestInitChannels()
{
    m_settings->config().m_dnChannels.clear();

    DownloadChannel channel1;
    channel1.setName("my_channel");
    channel1.setKey("0101010100000000000000000000000000000000000000000000000000000000");
    channel1.setDriveKey("0100000000050607080900010203040506070809000102030405060708090001");
    m_settings->config().m_dnChannels.insert({ channel1.getKey(), channel1 });

    DownloadChannel channel2;
    channel2.setName("my_channel2");
    channel2.setKey("0202020200000000000000000000000000000000000000000000000000000000");
    channel2.setDriveKey("0200000000050607080900010203040506070809000102030405060708090001");
    m_settings->config().m_dnChannels.insert({ channel2.getKey(), channel2 });
    m_settings->config().m_currentDownloadChannelKey = channel1.getKey();
}

void Model::stestInitDrives()
{
    m_settings->config().m_drives.clear();
    std::array<uint8_t,32> driveKey{1,0,0,0,0,5,6,7,8,9, 0,1,2,3,4,5,6,7,8,9, 0,1,2,3,4,5,6,7,8,9, 0,1};

    Drive d1;
    d1.setKey( sirius::drive::toString(driveKey));
    d1.setName("drive1");
    d1.setLocalFolder("/Users/alex/000-drive1");
    m_settings->config().m_drives.insert({ d1.getKey(), d1 });

    std::array<uint8_t,32> driveKey2{2,0,0,0,0,5,6,7,8,9, 0,1,2,3,4,5,6,7,8,9, 0,1,2,3,4,5,6,7,8,9, 0,1};

    Drive d2;
    d2.setKey(sirius::drive::toString(driveKey2));
    d2.setName("drive2");
    d2.setLocalFolder("/Users/alex/000-drive2");
    m_settings->config().m_drives.insert({ d2.getKey(), d2 });
    m_settings->config().m_currentDriveKey = d1.getKey();
}
