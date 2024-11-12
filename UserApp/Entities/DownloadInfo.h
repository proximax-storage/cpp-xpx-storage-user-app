#ifndef DOWNLOADINFO_H
#define DOWNLOADINFO_H

#include <array>
#include <QString>
#include "drive/ActionList.h"
#include "Common.h"
#include "Utils.h"

class DownloadInfo
{
    using DownloadNotification = std::function<void( const DownloadInfo& )>;

    std::array<uint8_t,32>   m_hash;
    QString                  m_channelKey;
    QString                  m_fileName;
    QString                  m_saveFolder;
    QString                  m_downloadFolder; // folder where torrent will be saved before renaming (by copy or move)
    bool                     m_isCompleted = false;
    bool                     m_channelIsOutdated = false;
    int                      m_progress = 0; // m_progress==1001 means completed
    sirius::drive::lt_handle m_ltHandle;
    
    uint64_t                 m_easyDownloadId = -1;

    std::optional<DownloadNotification> m_notification;
    
public:
    DownloadInfo();
    ~DownloadInfo();

public:
    std::array<uint8_t,32> getHash() const;
    void setHash(const std::array<uint8_t,32>& hash);

    QString getDownloadChannelKey() const;
    void setDownloadChannelKey(const QString& key);

    QString getFileName() const;
    void setFileName(const QString& name);

    QString getSaveFolder() const;
    void setSaveFolder(const QString& folder);

    QString getDownloadFolder() const;
    void setDownloadFolder(const QString& folder);

    int getProgress() const;
    void setProgress(int progress);

    sirius::drive::lt_handle getHandle() const;
    void setHandle(const sirius::drive::lt_handle& handle);

    bool isCompleted() const;
    void setCompleted(bool state);

    bool isChannelOutdated() const;
    void setChannelOutdated(bool state);
    
    void setNotification( const DownloadNotification& notification ) { m_notification = notification; };
    auto getNotification() { return m_notification; };
    
    bool isForViewing() const { return m_notification.has_value(); }
    
    void     setEasyDownloadId( uint64_t id ) { m_easyDownloadId = id; }
    uint64_t easyDownloadId() const { return m_easyDownloadId; }

public:
    template<class Archive>
    void serialize( Archive &ar )
    {
        ar(
                m_hash,
                m_channelKey,
                m_fileName,
                m_saveFolder,
                m_downloadFolder,
                m_isCompleted );
    }
};


#endif //DOWNLOADINFO_H
