#include "DownloadInfo.h"

DownloadInfo::DownloadInfo()
{}

DownloadInfo::~DownloadInfo()
{}

std::array<uint8_t,32> DownloadInfo::getHash() const {
    return m_hash;
}

void DownloadInfo::setHash(const std::array<uint8_t,32>& hash) {
    m_hash = hash;
}

std::string DownloadInfo::getDownloadChannelKey() const {
    return m_channelKey;
}

void DownloadInfo::setDownloadChannelKey(const std::string& key) {
    m_channelKey = key;
}

std::string DownloadInfo::getFileName() const {
    return m_fileName;
}

void DownloadInfo::setFileName(const std::string& name) {
    m_fileName = name;
}

std::string DownloadInfo::getSaveFolder() const {
    return m_saveFolder;
}

void DownloadInfo::setSaveFolder(const std::string& folder) {
    m_saveFolder = folder;
}

std::string DownloadInfo::getDownloadFolder() const {
    return m_downloadFolder;
}

void DownloadInfo::setDownloadFolder(const std::string& folder) {
    m_downloadFolder = folder;
}

int DownloadInfo::getProgress() const {
    return m_progress;
}

void DownloadInfo::setProgress(int progress) {
    m_progress = progress;
}

sirius::drive::lt_handle DownloadInfo::getHandle() const {
    return m_ltHandle;
}

void DownloadInfo::setHandle(const sirius::drive::lt_handle& handle) {
    m_ltHandle = handle;
}

bool DownloadInfo::isCompleted() const {
    return m_isCompleted;
}

void DownloadInfo::setCompleted(bool state) {
    m_isCompleted = state;
}

bool DownloadInfo::isChannelOutdated() const {
    return m_channelIsOutdated;
}
void DownloadInfo::setChannelOutdated(bool state) {
    m_channelIsOutdated = state;
}
