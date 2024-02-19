#include "DownloadChannel.h"
#include "utils/HexParser.h"

DownloadChannel::DownloadChannel()
    : m_name("")
    , m_key("")
    , m_driveKey("")
    , m_allowedPublicKeys({})
    , m_isCreating(false)
    , m_isDeleting(false)
    , m_creationTimepoint()
    , m_tobeDeletedTimepoint()
    , m_lastOpenedPath({})
    , m_fsTreeHash({ 0 })
    , m_fsTree({})
    , m_downloadingFsTree(false)
    , m_shardReplicators({})
{}

DownloadChannel::~DownloadChannel()
{}

std::string DownloadChannel::getName() const {
    return m_name;
}

void DownloadChannel::setName(const std::string& name) {
    m_name = name;
}

std::string DownloadChannel::getKey() const {
    return m_key;
}

void DownloadChannel::setKey(const std::string& key) {
    m_key = key;
}

std::string DownloadChannel::getDriveKey() const {
    return m_driveKey;
}

void DownloadChannel::setDriveKey(const std::string& key) {
    m_driveKey = key;
}

std::vector<std::string> DownloadChannel::getAllowedPublicKeys() const {
    return m_allowedPublicKeys;
}

void DownloadChannel::setAllowedPublicKeys(const std::vector<std::string>& keys) {
    m_allowedPublicKeys = keys;
}

bool DownloadChannel::isCreating() const {
    return m_isCreating;
}

void DownloadChannel::setCreating(bool state) {
    m_isCreating = state;
}

bool DownloadChannel::isDeleting() const {
    return m_isDeleting;
}

void DownloadChannel::setDeleting(bool state) {
    m_isDeleting = state;
}

timepoint DownloadChannel::getCreatingTimePoint() const {
    return m_creationTimepoint;
}

void DownloadChannel::setCreatingTimePoint(timepoint time) {
    m_creationTimepoint = time;
}

std::vector<std::string> DownloadChannel::getLastOpenedPath() const {
    return m_lastOpenedPath;
}

void DownloadChannel::setLastOpenedPath(const std::vector<std::string>& path) {
    m_lastOpenedPath = path;
}

std::array<uint8_t, 32> DownloadChannel::getFsTreeHash() const {
    return m_fsTreeHash;
}

void DownloadChannel::setFsTreeHash(const std::array<uint8_t, 32>& hash) {
    m_fsTreeHash = hash;
}

sirius::drive::FsTree DownloadChannel::getFsTree() const {
    return m_fsTree;
}

void DownloadChannel::setFsTree(const sirius::drive::FsTree& fsTree) {
    m_fsTree = fsTree;
}

bool DownloadChannel::isDownloadingFsTree() const {
    return m_downloadingFsTree;
}

void DownloadChannel::setDownloadingFsTree(bool state) {
    m_downloadingFsTree = state;
}

sirius::drive::ReplicatorList DownloadChannel::getReplicators() const {
    return m_shardReplicators;
}

void DownloadChannel::setReplicators(const std::vector<std::string>& replicators) {
    m_shardReplicators.clear();
    for( const auto& key : replicators )
    {
        std::array<uint8_t, 32> replicatorKey{ 0 };
        sirius::utils::ParseHexStringIntoContainer( key.c_str(), 64, replicatorKey );
        m_shardReplicators.emplace_back(replicatorKey);
    }
}