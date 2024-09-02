#ifndef DOWNLOADCHANNEL_H
#define DOWNLOADCHANNEL_H

#include <string>
#include <chrono>
#include <array>
#include <vector>

#include <cereal/types/chrono.hpp>

#include "Engines/StorageEngine.h"

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
        std::string getName() const;
        void setName(const std::string& name);
        std::string getKey() const;
        void setKey(const std::string& key);
        std::string getDriveKey() const;
        void setDriveKey(const std::string& key);
        std::vector<std::string> getAllowedPublicKeys() const;
        void setAllowedPublicKeys(const std::vector<std::string>& keys);
        bool isCreating() const;
        void setCreating(bool state);
        bool isDeleting() const;
        void setDeleting(bool state);
        timepoint getCreatingTimePoint() const;
        void setCreatingTimePoint(timepoint time);
        std::vector<std::string> getLastOpenedPath() const;
        void setLastOpenedPath(const std::vector<std::string>& path);
        std::array<uint8_t, 32> getFsTreeHash() const;
        void setFsTreeHash(const std::array<uint8_t, 32>& hash);
        sirius::drive::FsTree getFsTree() const;
        void setFsTree(const sirius::drive::FsTree& fsTree);
        bool isDownloadingFsTree() const;
        void setDownloadingFsTree(bool state);
        sirius::drive::ReplicatorList getReplicators() const;
        void setReplicators(const std::vector<std::string>& replicators);
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
        std::string m_name;
        std::string m_key;
        std::string m_driveKey;
        std::vector<std::string> m_allowedPublicKeys;
        bool m_isCreating;
        bool m_isDeleting;
        bool m_isForEasyDownload;
        timepoint m_creationTimepoint;
        timepoint m_tobeDeletedTimepoint;
        std::vector<std::string> m_lastOpenedPath;
        std::array<uint8_t, 32> m_fsTreeHash;
        sirius::drive::FsTree m_fsTree;
        bool m_downloadingFsTree;
        sirius::drive::ReplicatorList m_shardReplicators;
};


#endif //DOWNLOADCHANNEL_H
