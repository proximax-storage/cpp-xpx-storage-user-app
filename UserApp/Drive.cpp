#include "Drive.h"
#include "mainwin.h"
#include "utils/HexParser.h"
#include "Utils.h"


Drive::Drive()
    : m_driveKey("")
    , m_name("")
    , m_localDriveFolder("")
    , m_localDriveFolderExists(false)
    , m_size(0)
    , m_replicatorNumber(0)
    , m_replicatorList({})
    , m_lastOpenedPath({})
    , m_rootHash({ 0 })
    , m_fsTree({})
    , m_downloadingFsTree(false)
    , m_localDrive(std::make_shared<LocalDriveItem>())
    , m_actionList({})
    , m_currentModificationHash({ 0 })
    , m_driveState(creating)
{
    MainWin::instance()->onDriveStateChanged( *this );
}

//Drive::Drive(const Drive &drive) {
//    m_driveKey = drive.m_driveKey;
//    m_name = drive.m_name;
//    m_localDriveFolder = drive.m_localDriveFolder;
//    m_localDriveFolderExists = drive.m_localDriveFolderExists;
//    m_size = drive.m_size;
//    m_replicatorNumber = drive.m_replicatorNumber;
//    m_lastOpenedPath = drive.m_lastOpenedPath;
//    m_rootHash = drive.m_rootHash;
//    m_fsTree = drive.m_fsTree;
//    m_downloadingFsTree = drive.m_downloadingFsTree;
//
//    m_localDrive = std::make_shared<LocalDriveItem>();
//    m_localDrive->m_isFolder = drive.m_localDrive->m_isFolder;
//    m_localDrive->m_name = drive.m_localDrive->m_name;
//    m_localDrive->m_size = drive.m_localDrive->m_size;
//    m_localDrive->m_fileHash = drive.m_localDrive->m_fileHash;
//    m_localDrive->m_childs = drive.m_localDrive->m_childs;
//    m_localDrive->m_modifyTime = drive.m_localDrive->m_modifyTime;
//    m_localDrive->m_ldiStatus = drive.m_localDrive->m_ldiStatus;
//
//    m_actionList = drive.m_actionList;
//    m_currentModificationHash = drive.m_currentModificationHash;
//    m_driveState = drive.m_driveState;
//}

Drive &Drive::operator=(const Drive& drive) {
    if (this == &drive) {
        return *this;
    }

    m_driveKey = drive.m_driveKey;
    m_name = drive.m_name;
    m_localDriveFolder = drive.m_localDriveFolder;
    m_localDriveFolderExists = drive.m_localDriveFolderExists;
    m_size = drive.m_size;
    m_replicatorNumber = drive.m_replicatorNumber;
    m_lastOpenedPath = drive.m_lastOpenedPath;
    m_rootHash = drive.m_rootHash;
    m_fsTree = drive.m_fsTree;
    m_downloadingFsTree = drive.m_downloadingFsTree;

    m_localDrive = std::make_shared<LocalDriveItem>();
    m_localDrive->m_isFolder = drive.m_localDrive->m_isFolder;
    m_localDrive->m_name = drive.m_localDrive->m_name;
    m_localDrive->m_size = drive.m_localDrive->m_size;
    m_localDrive->m_fileHash = drive.m_localDrive->m_fileHash;
    m_localDrive->m_childs = drive.m_localDrive->m_childs;
    m_localDrive->m_modifyTime = drive.m_localDrive->m_modifyTime;
    m_localDrive->m_ldiStatus = drive.m_localDrive->m_ldiStatus;

    m_actionList = drive.m_actionList;
    m_currentModificationHash = drive.m_currentModificationHash;
    m_driveState = drive.m_driveState;
    return *this;
}

std::string Drive::getKey() const {
    return m_driveKey;
}

void Drive::setKey(const std::string &key) {
    m_driveKey = QString::fromStdString(key).toUpper().toStdString();
}

std::string Drive::getName() const {
    return m_name;
}

void Drive::setName(const std::string &name) {
    m_name = name;
}

std::string Drive::getLocalFolder() const {
    return m_localDriveFolder;
}

void Drive::setLocalFolder(const std::string &localFolder) {
    m_localDriveFolder = localFolder;
}

bool Drive::isLocalFolderExists() const {
    return m_localDriveFolderExists;
}

void Drive::setLocalFolderExists(bool state) {
    m_localDriveFolderExists = state;
}

uint64_t Drive::getSize() const {
    return m_size;
}

void Drive::setSize(uint64_t size) {
    m_size = size;
}

uint32_t Drive::getReplicatorsCount() const {
    return m_replicatorNumber;
}

void Drive::setReplicatorsCount(uint32_t count) {
    m_replicatorNumber = count;
}

std::vector<std::string> Drive::getLastOpenedPath() const {
    return m_lastOpenedPath;
}
void Drive::setLastOpenedPath(const std::vector<std::string>& path) {
    m_lastOpenedPath = path;
}

std::array<uint8_t, 32> Drive::getRootHash() const {
    return m_rootHash;
}

void Drive::setRootHash(const std::array<uint8_t, 32>& rootHash) {
    m_rootHash = rootHash;
}

sirius::drive::FsTree Drive::getFsTree() const {
    return m_fsTree;
}

void Drive::setFsTree(const sirius::drive::FsTree& fsTree) {
    m_fsTree = fsTree;
}

bool Drive::isDownloadingFsTree() const {
    return m_downloadingFsTree;
}

void Drive::setDownloadingFsTree(bool state) {
    m_downloadingFsTree = state;
}

std::shared_ptr<LocalDriveItem> Drive::getLocalDriveItem() const {
    return m_localDrive;
}

void Drive::setLocalDriveItem(std::shared_ptr<LocalDriveItem> item) {
    m_localDrive = item;
}

sirius::drive::ActionList Drive::getActionsList() const {
    return m_actionList;
}

void Drive::setActionsList(const sirius::drive::ActionList& actions) {
    m_actionList = actions;
}

std::array<uint8_t, 32> Drive::getModificationHash() const {
    return m_currentModificationHash;
}

void Drive::setModificationHash(const std::array<uint8_t, 32>& modificationHash, bool isStreaming) {
    m_currentModificationHash = modificationHash;
    m_isStreaming = isStreaming;
}

DriveState Drive::getState() const {
    return m_driveState;
}

sirius::drive::ReplicatorList Drive::getReplicators() const {
    return m_replicatorList;
}

void Drive::setReplicators(const std::vector<std::string>& replicators) {
    m_replicatorList.clear();
    for( const auto& key : replicators )
    {
        std::array<uint8_t, 32> replicatorKey{};
        sirius::utils::ParseHexStringIntoContainer( key.c_str(), 64, replicatorKey );
        m_replicatorList.emplace_back(replicatorKey);
    }
}

void Drive::updateDriveState(DriveState newState)
{
//    if ( m_driveState == newState )
//    {
//        return;
//    }

    switch( m_driveState )
    {
        case creating:
        {
            if ( newState == creating )
            {
                //m_driveState = unconfirmed;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            if ( newState == unconfirmed )
            {
                m_driveState = unconfirmed;
                MainWin::instance()->onDriveStateChanged( *this );
                // emit -> MainWin::deleteDrive
            }
            if ( newState == no_modifications )
            {
                m_driveState = no_modifications;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            break;
        }
        case unconfirmed:
        {
            qWarning() << "unconfirmed: Ignore newState" << (int)newState;
            break;
        }
        case deleting:
        {
            if ( newState == deleted )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
                // emit -> MainWin::deleteDrive
            }
            if ( newState == no_modifications )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            break;
        }
        case deleted:
        {
            qWarning() << "deleted: Ignore newState" << (int)newState;
            break;
        }
        case no_modifications:
        {
            if ( newState == deleting )
            {
                //m_driveState = no_modifications;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            if ( newState == deleting )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            if ( newState == registering )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            break;
        }
        case registering:
        {
            if ( newState == failed )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            if ( newState == uploading )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            break;
        }
        case approved:
        {
            if ( newState == no_modifications )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            break;
        }
        case uploading:
        {
            if ( newState == canceling )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            if ( newState == failed )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            if ( newState == approved )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            break;
        }
        case failed:
        {
            if ( newState == no_modifications )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            break;
        }
        case canceling:
        {
            if ( newState == canceled )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            if ( newState == failed )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            break;
        }
        case canceled:
        {
            if ( newState == no_modifications )
            {
                m_driveState = newState;
                MainWin::instance()->onDriveStateChanged( *this );
            }
            break;
        }
        case contract_deploying:
        {
            break;
        }
        case contract_deployed:
        {
            break;
        }
        default:
        {
            break;
        }
    }
}

endpoint_list Drive::getEndpointReplicatorList() const
{
    endpoint_list endPointList;
    for( auto& replicatorKey : m_replicatorList )
    {
        auto endpoint = gStorageEngine->getEndpoint( replicatorKey );
        if ( endpoint )
        {
            LOG( "addEndpoint: " << *endpoint );
            endPointList.push_back( *endpoint );
        }
    }
    return endPointList;
}
