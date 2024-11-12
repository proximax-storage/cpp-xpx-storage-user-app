#include "Models/Model.h"
#include "Diff.h"
#include "Entities/Drive.h"
#include "LocalDriveItem.h"
#include "Engines/StorageEngine.h"
#include "utils/HexParser.h"
#ifndef WA_APP
#include "drive/ViewerSession.h"
#endif
#include "Entities/Settings.h"
#include "Entities/Settings.h"

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
    return m_settings->saveSettings();
}

QString Model::getDownloadFolder()
{
    return m_settings->downloadFolder();
}

QString Model::getAccountName() {
    return m_settings->accountConfig().m_accountName;
}

QString Model::getBootstrapReplicator() {
    return m_settings->m_replicatorBootstrap;
}

void Model::setCurrentAccountIndex(int index) {
    m_settings->setCurrentAccountIndex(index);
}

void Model::setBootstrapReplicator(const QString& address) {
    m_settings->m_replicatorBootstrap = address;
}

QString Model::getUdpPort() {
    return m_settings->m_udpPort;
}

QString Model::getGatewayIp() {
    const auto gatewayEndpoint = m_settings->m_restBootstrap.split(":");
    return gatewayEndpoint.size() == 2 ? gatewayEndpoint[0] : "";
}

QString Model::getGatewayPort() {
    const auto gatewayEndpoint = m_settings->m_restBootstrap.split(":");
    return gatewayEndpoint.size() == 2 ? gatewayEndpoint[1] : "";
}

double Model::getFeeMultiplier() {
    return m_settings->m_feeMultiplier;
}

Settings* Model::getSettings() {
    return m_settings;
}

const sirius::crypto::KeyPair& Model::getKeyPair() {
    return m_settings->accountConfig().m_keyPair.value();
}

#ifndef WA_APP
void Model::onDownloadCompleted(lt::torrent_handle handle) {
    m_settings->onDownloadCompleted(handle,*this);
}
#endif

void Model::addDownloadChannel(const DownloadChannel &channel) {
    m_settings->accountConfig().m_dnChannels.insert({ channel.getKey(), channel });
}

std::map<QString, DownloadChannel>&  Model::getDownloadChannels()
{
    return m_settings->accountConfig().m_dnChannels;
}

void Model::setCurrentDownloadChannelKey(const QString& channelKey)
{
    m_settings->accountConfig().m_currentDownloadChannelKey = channelKey;
}

QString Model::currentDownloadChannelKey()
{
    return m_settings->accountConfig().m_currentDownloadChannelKey;
}

std::vector<DownloadInfo>& Model::downloads()
{
    return m_settings->accountConfig().m_downloads;
}

void Model::markChannelsForDelete(const QString& driveId, bool state) {
    auto drive = findDrive(driveId);
    if (drive) {
        auto& channels = m_settings->accountConfig().m_dnChannels;
        for (auto& [key, channel] : channels) {
            if (channel.getDriveKey().compare(drive->getKey(), Qt::CaseInsensitive) == 0) {
                channel.setDeleting(state);
            }
        }
    }
}

bool Model::isDownloadChannelsLoaded() {
    return m_settings->accountConfig().m_channelsLoaded;
}

void Model::setDownloadChannelsLoaded(bool state) {
    m_settings->accountConfig().m_channelsLoaded = state;
}

std::map<QString, CachedReplicator> Model::getMyReplicators() const {
    return m_settings->accountConfig().m_myReplicators;
}

void Model::addMyReplicator(const CachedReplicator& replicator) {
    m_settings->accountConfig().m_myReplicators.insert_or_assign(replicator.getPrivateKey(), replicator);
}

void Model::removeMyReplicator(const QString& replicatorKey) {
    m_settings->accountConfig().m_myReplicators.erase(replicatorKey);
}

void Model::setCurrentDriveKey( const QString& driveKey )
{
    const auto driveKyeUpperCase = driveKey.toUpper();
    if ( ! MAP_CONTAINS(m_settings->accountConfig().m_drives, driveKyeUpperCase) )
    //if ( !m_settings->accountConfig().m_drives.contains(driveKyeUpperCase) )
    {
        qWarning() << "Model::setCurrentDriveKey: invalid drive key: " << driveKyeUpperCase;
    }
    else
    {
        m_settings->accountConfig().m_currentDriveKey = driveKyeUpperCase;
    }
}

QString Model::currentDriveKey()
{
    return m_settings->accountConfig().m_currentDriveKey;
}

bool Model::isDriveWithNameExists(const QString& driveName) const
{
    if (driveName.isEmpty()) {
        return false;
    }

    auto it = std::find_if( m_settings->accountConfig().m_drives.begin(), m_settings->accountConfig().m_drives.end(), [&driveName] (const auto& iterator) {
        return iterator.second.getName().compare(driveName, Qt::CaseInsensitive) == 0;
    });

    return !(it == m_settings->accountConfig().m_drives.end());
}

bool Model::isChannelWithNameExists(const QString& channelName) const
{
    if (channelName.isEmpty()) {
        return false;
    }

    auto it = std::find_if( m_settings->accountConfig().m_dnChannels.begin(), m_settings->accountConfig().m_dnChannels.end(), [&channelName] (const auto& iterator) {
        return iterator.second.getName().compare(channelName, Qt::CaseInsensitive) == 0;
    });

    return !(it == m_settings->accountConfig().m_dnChannels.end());
}

bool Model::isReplicatorWithNameExists(const QString& replicatorName) const
{
    if (replicatorName.isEmpty()) {
        return false;
    }

    auto it = std::find_if( m_settings->accountConfig().m_myReplicators.begin(), m_settings->accountConfig().m_myReplicators.end(), [&replicatorName] (const auto& iterator) {
        return iterator.second.getName().compare(replicatorName, Qt::CaseInsensitive) == 0;
    });

    return !(it == m_settings->accountConfig().m_myReplicators.end());
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

QString Model::getClientPublicKey() {
    return m_settings->accountConfig().m_publicKeyStr;
}

QString Model::getClientPrivateKey() {
    return m_settings->accountConfig().m_privateKeyStr;
}

bool Model::isDrivesLoaded() {
    return m_settings->accountConfig().m_drivesLoaded;
}

void Model::setDrivesLoaded(bool state) {
    m_settings->accountConfig().m_drivesLoaded = state;
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
    m_settings->accountConfig().m_drives.insert({ drive.getKey(), drive });
}

void Model::onMyOwnChannelsLoaded(const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& channelsPages )
{
    std::vector<xpx_chain_sdk::DownloadChannel> remoteChannels;
    for (const auto& page : channelsPages) {
        std::copy(page.data.channels.begin(), page.data.channels.end(), std::back_inserter(remoteChannels));
    }

    auto& localChannels = m_settings->accountConfig().m_dnChannels;
    std::map<QString, DownloadChannel>  validChannels;
    for (auto &remoteChannel : remoteChannels) {
        QString hash = remoteChannel.data.id.c_str();
        auto it = std::find_if(localChannels.begin(), localChannels.end(), [&hash](const auto &channel) {
            return channel.first.compare(hash, Qt::CaseInsensitive) == 0;
        });

        // add new channel
        if (it == localChannels.end()) {
            DownloadChannel channel;
            channel.setName(remoteChannel.data.id.c_str());
            channel.setKey(remoteChannel.data.id.c_str());
            channel.setAllowedPublicKeys(convertToQStringVector(remoteChannel.data.listOfPublicKeys));
            channel.setDriveKey(remoteChannel.data.drive.c_str());
            channel.setReplicators(convertToQStringVector(remoteChannel.data.shardReplicators));
            channel.setCreating(false);
            channel.setDeleting(false);
            auto creationTime = std::chrono::steady_clock::now();
            channel.setCreatingTimePoint(creationTime);
            validChannels.insert({ channel.getKey(), channel });
        } else {
            // update valid channel
            it->second.setCreating(false);
            it->second.setDeleting(false);
            it->second.setReplicators(convertToQStringVector(remoteChannel.data.shardReplicators));
            validChannels.insert({ it->first, it->second });
        }
    }

    // remove closed channels from saved
    m_settings->accountConfig().m_dnChannels = validChannels;
    m_settings->saveSettings();
}

void Model::onSponsoredChannelsLoaded(const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& remoteChannelsPages) {
    std::vector<xpx_chain_sdk::DownloadChannel> remoteChannels;
    for (const auto& page : remoteChannelsPages) {
        std::copy(page.data.channels.begin(), page.data.channels.end(), std::back_inserter(remoteChannels));
    }

    auto& localChannels = m_settings->accountConfig().m_dnChannels;
    for (auto &remoteChannel : remoteChannels) {
        QString hash = remoteChannel.data.id.c_str();
        auto it = std::find_if(localChannels.begin(), localChannels.end(), [&hash](const auto &channel) {
            return channel.first.compare(hash, Qt::CaseInsensitive) == 0;
        });

        // add new channel
        if (it == localChannels.end()) {
            DownloadChannel channel;
            channel.setName(remoteChannel.data.id.c_str());
            channel.setKey(remoteChannel.data.id.c_str());
            channel.setAllowedPublicKeys(convertToQStringVector(remoteChannel.data.listOfPublicKeys));
            channel.setDriveKey(remoteChannel.data.drive.c_str());
            channel.setReplicators(convertToQStringVector(remoteChannel.data.shardReplicators));
            channel.setCreating(false);
            channel.setDeleting(false);
            auto creationTime = std::chrono::steady_clock::now();
            channel.setCreatingTimePoint(creationTime);
            m_settings->accountConfig().m_dnChannels.insert({ channel.getKey(), channel });
        }
    }

    m_settings->saveSettings();
}

void Model::onDrivesLoaded( const std::vector<xpx_chain_sdk::drives_page::DrivesPage>& remoteDrivesPages )
{
    std::vector<xpx_chain_sdk::Drive> remoteDrives;
    for (const auto& page : remoteDrivesPages) {
        std::copy(page.data.drives.begin(), page.data.drives.end(), std::back_inserter(remoteDrives));
    }

    qDebug() << "Model::onDrivesLoaded: " << remoteDrives.size();
    auto& drives = m_settings->accountConfig().m_drives;

    for( auto& remoteDrive : remoteDrives )
    {
//        if (drives.contains(remoteDrive.data.multisig)) {
        if ( MAP_CONTAINS(drives, remoteDrive.data.multisig.c_str())) {

            const auto driveKeyUpperCase = QString::fromStdString(remoteDrive.data.multisig).toUpper();
            Drive& drive = drives[driveKeyUpperCase];
            drive.setReplicatorsCount(remoteDrive.data.replicatorCount);

            if ( !remoteDrive.data.replicators.empty() )
            {
                drive.setReplicators(convertToQStringVector(remoteDrive.data.replicators));
                gStorageEngine->addReplicators( drive.getReplicators() );
            }

            drive.updateDriveState(creating);
            drive.updateDriveState(no_modifications);
        } else {
            Drive newDrive;
            newDrive.setKey(remoteDrive.data.multisig.c_str());
            newDrive.setName(remoteDrive.data.multisig.c_str());
            newDrive.setLocalFolder(QString::fromUtf8(homeFolder().c_str()) + "/" + newDrive.getKey());
            newDrive.setSize(remoteDrive.data.size);
            newDrive.setReplicatorsCount(remoteDrive.data.replicatorCount);
            addDrive(newDrive);

            Drive& drive = drives[newDrive.getKey()];
            drive.setReplicatorsCount(remoteDrive.data.replicatorCount);

            if ( !remoteDrive.data.replicators.empty() )
            {
                drive.setReplicators(convertToQStringVector(remoteDrive.data.replicators));
                gStorageEngine->addReplicators( drive.getReplicators() );
            }

            drive.updateDriveState(creating);
            drive.updateDriveState(no_modifications);
        }

        Drive& currentDrive = drives[QString::fromStdString(remoteDrive.data.multisig).toUpper()];
        if (remoteDrive.data.activeDataModifications.empty())
        {
            continue;
        }
        else if (remoteDrive.data.activeDataModifications[remoteDrive.data.activeDataModifications.size() - 1].dataModification.isStream)
        {
            // For stream modification
            auto lastModificationIndex = remoteDrive.data.activeDataModifications.size();
            auto lastModificationId = remoteDrive.data.activeDataModifications[lastModificationIndex - 1].dataModification.id;

            currentDrive.setModificationHash(Model::hexStringToHash( lastModificationId.c_str() ));
            currentDrive.setIsStreaming();
            currentDrive.updateDriveState(registering);
            currentDrive.updateDriveState(uploading);
        }
        else
        {
            // For modifications
            auto lastModificationIndex = remoteDrive.data.activeDataModifications.size();
            auto lastModificationId = remoteDrive.data.activeDataModifications[lastModificationIndex - 1].dataModification.id;
            currentDrive.setModificationHash(Model::hexStringToHash( lastModificationId.c_str() ));

            // TODO: Approach needs to be improved
            const QString pathToDriveData = stdStringToQStringUtf8(getSettingsFolder()) + "/" + currentDrive.getKey().toUpper() + CLIENT_SANDBOX_FOLDER;
            bool isDirExists = QDir(QDir::toNativeSeparators(pathToDriveData)).exists();
            if (isDirExists) {
                std::map<QString, QFileInfo> filesData;
                QFileInfoList torrentsList;
                QDirIterator it(pathToDriveData, QDirIterator::Subdirectories);

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
                        filesData.insert_or_assign(file.fileName(), file);
                    }
                }

                for (const auto& t : torrentsList) {
                    //if (filesData.contains(t.fileName())) {
                    if ( MAP_CONTAINS(filesData, t.fileName())) {
                        emit addTorrentFileToStorageSession(t.fileName(),
                                                            pathToDriveData,
                                                            rawHashFromHex(currentDrive.getKey()),
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
            return d.second.getKey().compare(driveInfo.data.multisig.c_str(), Qt::CaseInsensitive) == 0;
        });

        if ( it == remoteDrives.end() )
        {
            d.second.updateDriveState(unconfirmed);
        }
    }

    m_settings->saveSettings();
}

std::map<QString, Drive>& Model::getDrives()
{
    return m_settings->accountConfig().m_drives;
}

Drive* Model::currentDrive()
{
//    if (m_settings->accountConfig().m_drives.contains(m_settings->accountConfig().m_currentDriveKey))
    if ( MAP_CONTAINS( m_settings->accountConfig().m_drives, m_settings->accountConfig().m_currentDriveKey) )
    {
        return &m_settings->accountConfig().m_drives[m_settings->accountConfig().m_currentDriveKey];
    }

    return nullptr;
}

Drive* Model::findDrive( const QString& driveKey )
{
    const auto driveKeyUpperCase = driveKey.toUpper();
    auto& drives = m_settings->accountConfig().m_drives;
    //if ( drives.contains(driveKeyUpperCase) ) {
    if ( MAP_CONTAINS(drives,driveKeyUpperCase) ) {
        return &drives[driveKeyUpperCase];
    }

    return nullptr;
}

Drive* Model::findDriveByNameOrPublicKey( const QString& value )
{
    auto& drives = m_settings->accountConfig().m_drives;
    for (auto& drive : drives) {
        if (drive.second.getName() == value || drive.second.getKey().compare(value, Qt::CaseInsensitive) == 0) {
            return &drive.second;
        }
    }

    return nullptr;
}

CachedReplicator Model::findReplicatorByNameOrPublicKey( const QString& value )
{
    for (const auto& replicator : m_settings->accountConfig().m_myReplicators) {
        if (replicator.second.getName() == value || replicator.second.getPublicKey().compare(value, Qt::CaseInsensitive) == 0) {
            return replicator.second;
        }
    }

    return {};
}

Drive* Model::findDriveByModificationId( const std::array<uint8_t, 32>& modificationId )
{
    auto& drives = m_settings->accountConfig().m_drives;
    auto it = std::find_if( drives.begin(), drives.end(), [&modificationId] (const auto& iterator) {
        return iterator.second.getModificationHash() == modificationId;
    });

    return it == drives.end() ? nullptr : &it->second;
}

void Model::removeDrive( const QString& driveKey )
{
    const auto driveKeyUpperCase = driveKey.toUpper();
    auto& drives = m_settings->accountConfig().m_drives;
    drives.erase(driveKeyUpperCase);

    m_settings->saveSettings();
}

void Model::removeChannelsByDriveKey(const QString &driveKey) {
    auto& channels = m_settings->accountConfig().m_dnChannels;
    auto begin = channels.begin();
    while(begin != channels.end()) {
        if (begin->second.getDriveKey().compare(driveKey, Qt::CaseInsensitive) == 0) {
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
    //if (m_settings->accountConfig().m_dnChannels.contains(m_settings->accountConfig().m_currentDownloadChannelKey))
    if ( MAP_CONTAINS( m_settings->accountConfig().m_dnChannels, m_settings->accountConfig().m_currentDownloadChannelKey ))
    {
        return &m_settings->accountConfig().m_dnChannels[m_settings->accountConfig().m_currentDownloadChannelKey];
    }

    return nullptr;
}

DownloadChannel* Model::findChannel( const QString& channelKey )
{
    auto& channels = m_settings->accountConfig().m_dnChannels;
    auto it = std::find_if( channels.begin(), channels.end(), [channelKey] (const auto& channelInfo)
    {
        return channelKey.compare(channelInfo.first, Qt::CaseInsensitive) == 0;
    });

    return it == channels.end() ? nullptr : &it->second;
}

CachedReplicator Model::findReplicatorByPublicKey(const QString& replicatorPublicKey) const {
    auto it = std::find_if( m_settings->accountConfig().m_myReplicators.begin(), m_settings->accountConfig().m_myReplicators.end(), [replicatorPublicKey] (const auto& r)
    {
        return replicatorPublicKey.compare(r.second.getPublicKey(), Qt::CaseInsensitive) == 0;
    });

    return it == m_settings->accountConfig().m_myReplicators.end() ? CachedReplicator() : it->second;
}

void Model::updateReplicatorAlias(const QString& replicatorPublicKey, const QString& alias) const {
    auto it = std::find_if( m_settings->accountConfig().m_myReplicators.begin(), m_settings->accountConfig().m_myReplicators.end(), [replicatorPublicKey] (const auto& r)
    {
        return replicatorPublicKey.compare(r.second.getPublicKey(), Qt::CaseInsensitive) == 0;
    });

    if (it != m_settings->accountConfig().m_myReplicators.end()) {
        it->second.setName(alias);
    }
}

void Model::removeChannel( const QString& channelKey )
{
    const auto channelKeyUpperCase = channelKey.toUpper();
    auto& channels = m_settings->accountConfig().m_dnChannels;
    channels.erase(channelKeyUpperCase);

    m_settings->saveSettings();
}

void Model::applyForChannels(const QString &driveKey, std::function<void(DownloadChannel &)> callback) {
    for( auto& channel : m_settings->accountConfig().m_dnChannels )
    {
        if (channel.second.getDriveKey().compare(driveKey, Qt::CaseInsensitive) == 0)
        {
            callback(channel.second);
        }
    }
}

void Model::applyFsTreeForChannels( const QString& driveKey, const sirius::drive::FsTree& fsTree, const std::array<uint8_t, 32>& fsTreeHash )
{
    for( auto& channel : m_settings->accountConfig().m_dnChannels )
    {
        if (channel.second.getDriveKey().compare(driveKey, Qt::CaseInsensitive) == 0)
        {
            channel.second.setFsTree(fsTree);
            channel.second.setFsTreeHash(fsTreeHash);
            channel.second.setDownloadingFsTree(false);

            if ( m_viewingFsTreeHandler )
            {
                (*m_viewingFsTreeHandler)( true, channel.second.getKey(), channel.second.getDriveKey() );
            }
            
            for( auto it = m_channelFsTreeHandler.begin(); it != m_channelFsTreeHandler.end(); )
            {
                if ( (*it)( true, channel.second.getKey(), channel.second.getDriveKey() ) )
                {
                    it = m_channelFsTreeHandler.erase(it);
                }
                else
                {
                    it++;
                }
            }

        }
    }
}

void Model::initStorageEngine()
{
    gStorageEngine = std::make_shared<StorageEngine>(this);
}

void Model::startStorageEngine()
{
    gStorageEngine->start();
}

void Model::endStorageEngine()
{
    gStorageEngine.reset();
}

uint64_t Model::lastModificationSize() const
{
    return gStorageEngine->m_lastModifySize;
}

sirius::drive::lt_handle Model::downloadFile(const QString&                  channelIdStr,
                                             const std::array<uint8_t, 32>&  fileHash,
                                             std::filesystem::path           outputFolder )
{
    std::array<uint8_t,32> channelId{ 0 };
    sirius::utils::ParseHexStringIntoContainer( channelIdStr.toStdString().c_str(), 64, channelId );

    return gStorageEngine->downloadFile( channelId, fileHash, outputFolder );
}

void Model::calcDiff( Drive& drive )
{
    std::array<uint8_t,32> driveKey{};
    sirius::utils::ParseHexStringIntoContainer( drive.getKey().toStdString().c_str(), 64, driveKey );

    qDebug() << "Model::calcDiff: localDriveFolder: " << drive.getLocalFolder();
    if ( ! isFolderExists(drive.getLocalFolder()) )
    {
        drive.setLocalFolderExists(false);
        drive.setActionsList({});
    }
    else
    {
        drive.setLocalFolderExists(true);

        auto localDrive = std::make_shared<LocalDriveItem>();
        Diff::calcLocalDriveInfoR( *localDrive, qStringToStdStringUTF8(drive.getLocalFolder()), true, &driveKey );
        sirius::drive::ActionList actionList;
        Diff diff( *localDrive, qStringToStdStringUTF8(drive.getLocalFolder()), drive.getFsTree(), actionList);
        drive.getFsTree().dbgPrint();

        drive.setActionsList(actionList);
        drive.setLocalDriveItem(std::move(localDrive));
    }
}

std::array<uint8_t,32> Model::hexStringToHash( const QString& str )
{
    std::array<uint8_t,32> hash{};
    sirius::utils::ParseHexStringIntoContainer( str.toStdString().c_str(), 64, hash );
    return hash;
}

void Model::removeTorrentSync( sirius::drive::InfoHash infoHash )
{
    gStorageEngine->removeTorrentSync( infoHash );
}

const std::vector<StreamInfo>& Model::streamerAnnouncements() const
{
    return m_settings->accountConfig().m_streams;
}

std::vector<StreamInfo>& Model::getStreams()
{
    return m_settings->accountConfig().m_streams;
}

Drive* Model::currentStreamingDrive() const
{
    for( auto& [key,drive] : m_settings->accountConfig().m_drives )
    {
        if ( drive.isStreaming() )
        {
            return &drive;
        }
    }
    return nullptr;
}

void Model::addStreamRef( const StreamInfo& streamInfo )
{
    auto& streams = m_settings->accountConfig().m_streamRefs;
    streams.push_back( streamInfo );
    std::sort( streams.begin(), streams.end(), [] (const auto& s1, const auto& s2) ->bool {
        return s1.m_secsSinceEpoch > s2.m_secsSinceEpoch;
    });
    
    m_settings->saveSettings();
}

void Model::deleteStreamRef( int index )
{
    auto& streams = m_settings->accountConfig().m_streamRefs;
    streams.erase( streams.begin() + index );
    m_settings->saveSettings();
}

const std::vector<StreamInfo>& Model::streamRefs() const
{
    return m_settings->accountConfig().m_streamRefs;
}

void Model::setCurrentStreamInfo( std::optional<StreamInfo> info )
{
    m_settings->accountConfig().m_currentStreamInfo = info;
}

std::optional<StreamInfo> Model::currentStreamInfo() const
{
    return m_settings->accountConfig().m_currentStreamInfo;
}

const StreamInfo*  Model::getStreamRef( int index ) const
{
    if ( index >= 0 && index < m_settings->accountConfig().m_streamRefs.size() )
    {
        return &m_settings->accountConfig().m_streamRefs[index];
    }
    return nullptr;
}

StreamInfo*  Model::getStreamRef( int index )
{
    if ( index >= 0 && index < m_settings->accountConfig().m_streamRefs.size() )
    {
        return &m_settings->accountConfig().m_streamRefs[index];
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
#ifdef WA_APP
//TODO_WA
#else
    gStorageEngine->m_session->setModificationStatusResponseHandler( handler );
#endif
}

DriveContractModel& Model::driveContractModel() {
    return m_settings->accountConfig().m_driveContractModel;
};

void Model::requestModificationStatus(  const QString&     replicatorKey,
                                        const QString&     driveKey,
                                        const QString&     modificationHash,
                                        std::optional<boost::asio::ip::udp::endpoint> replicatorEndpoint )
{
#ifdef WA_APP
//TODO_WA
#else

//    std::error_code ec;
//    if ( fs::exists( "/Users/alex/Proj/cpp-xpx-storage-user-app", ec ) )
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
#endif
}

const std::vector<EasyDownloadInfo>& Model::easyDownloads() const
{
    return m_settings->accountConfig().m_easyDownloads;
}

std::vector<EasyDownloadInfo>& Model::easyDownloads()
{
    return m_settings->accountConfig().m_easyDownloads;
}

uint64_t& Model::lastUniqueIdOfEasyDownload()
{
    return m_settings->accountConfig().m_lastUniqueIdOfEasyDownload;
}
