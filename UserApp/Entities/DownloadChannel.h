#ifndef DOWNLOADCHANNEL_H
#define DOWNLOADCHANNEL_H

#include <string>
#include <chrono>
#include <array>
#include <vector>

#include <cereal/types/chrono.hpp>

#include "Engines/StorageEngine.h"
#include "Utils.h"

#ifndef WA_APP
#   include "drive/Session.h"
#endif

#include "drive/FsTree.h"

using timepoint = std::chrono::time_point<std::chrono::steady_clock>;

class DownloadChannel {
    public:
        DownloadChannel();
        ~DownloadChannel();

    public:
        QString getName() const;
        void setName(const QString& name);
        QString getKey() const;
        void setKey(const QString& key);
        QString getDriveKey() const;
        void setDriveKey(const QString& key);
        std::vector<QString> getAllowedPublicKeys() const;
        void setAllowedPublicKeys(const std::vector<QString>& keys);
        bool isCreating() const;
        void setCreating(bool state);
        bool isDeleting() const;
        void setDeleting(bool state);
        timepoint getCreatingTimePoint() const;
        void setCreatingTimePoint(timepoint time);
        std::vector<QString> getLastOpenedPath() const;
        void setLastOpenedPath(const std::vector<QString>& path);
        std::array<uint8_t, 32> getFsTreeHash() const;
        void setFsTreeHash(const std::array<uint8_t, 32>& hash);
        sirius::drive::FsTree getFsTree() const;
        void setFsTree(const sirius::drive::FsTree& fsTree);
        bool isDownloadingFsTree() const;
        void setDownloadingFsTree(bool state);
        sirius::drive::ReplicatorList getReplicators() const;
        void setReplicators(const std::vector<QString>& replicators);
        void setForEasyDownload(bool state);
        bool isForEasyDownload() const;

        template<class Archive>
        void serialize( Archive &ar )
        {
            ar( m_key,
                m_name,
                m_driveKey,
                m_allowedPublicKeys,
                m_isCreating,
                m_isDeleting,
                m_creationTimepoint,
                m_tobeDeletedTimepoint,
                m_fsTreeHash,
                m_isForEasyDownload );
        }

    private:
        QString m_name;
        QString m_key;
        QString m_driveKey;
        std::vector<QString> m_allowedPublicKeys;
        bool m_isCreating;
        bool m_isDeleting;
        bool m_isForEasyDownload;
        timepoint m_creationTimepoint;
        timepoint m_tobeDeletedTimepoint;
        std::vector<QString> m_lastOpenedPath;
        std::array<uint8_t, 32> m_fsTreeHash;
        sirius::drive::FsTree m_fsTree;
        bool m_downloadingFsTree;
        sirius::drive::ReplicatorList m_shardReplicators;
};


#endif //DOWNLOADCHANNEL_H
