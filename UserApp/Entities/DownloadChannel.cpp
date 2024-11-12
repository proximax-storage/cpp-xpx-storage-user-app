#include "DownloadChannel.h"
#include "utils/HexParser.h"
#include <qdebug.h>

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

QString DownloadChannel::getName() const {
    return m_name;
}

void DownloadChannel::setName(const QString& name) {
    m_name = name;
}

QString DownloadChannel::getKey() const {
    return m_key;
}

void DownloadChannel::setKey(const QString& key) {
    m_key = key;
}

QString DownloadChannel::getDriveKey() const {
    return m_driveKey;
}

void DownloadChannel::setDriveKey(const QString& key) {
    m_driveKey = key;
}

std::vector<QString> DownloadChannel::getAllowedPublicKeys() const {
    return m_allowedPublicKeys;
}

void DownloadChannel::setAllowedPublicKeys(const std::vector<QString>& keys) {
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

std::vector<QString> DownloadChannel::getLastOpenedPath() const {
    return m_lastOpenedPath;
}

void DownloadChannel::setLastOpenedPath(const std::vector<QString>& path) {
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

void DownloadChannel::setReplicators(const std::vector<QString>& replicators) {
    qDebug() << "setReplicators: " << this << " size: " << replicators.size();
    m_shardReplicators.clear();
    for( const auto& key : replicators )
    {
        std::array<uint8_t, 32> replicatorKey{ 0 };
        sirius::utils::ParseHexStringIntoContainer( key.toStdString().c_str(), 64, replicatorKey );
        m_shardReplicators.emplace_back(replicatorKey);
    }
}

void DownloadChannel::setForEasyDownload(bool state)
{
    m_isForEasyDownload = state;
}

bool DownloadChannel::isForEasyDownload() const
{
    return m_isForEasyDownload;
}
