#ifndef DRIVE_H
#define DRIVE_H

#include <QObject>
#include "drive/FsTree.h"
#include "drive/ActionList.h"
#include "LocalDriveItem.h"

#ifdef WA_APP
#   include <boost/asio.hpp>
//#   include "types.h"
    // namespace sirius { namespace drive {
    // class FsTree;
    // class ActionList;
    // }}
#endif

using  endpoint_list  = std::vector<boost::asio::ip::udp::endpoint>;

enum DriveState
{
    creating,
    unconfirmed,
    deleting,
    deleted,
    no_modifications,
    registering,
    approved,
    uploading,
    failed,
    canceling,
    canceled,
    contract_deploying,
    contract_deployed
};

class DriveModificationEvent;

class Drive
{
    public:
        explicit Drive();
        Drive(const Drive& drive) = default;
        ~Drive() = default;

    public:
        Drive& operator=(const Drive&);

    public:
        QString getKey() const;
        void setKey(const QString& key);

        QString getName() const;
        void setName(const QString& name);

        QString getLocalFolder() const;
        void setLocalFolder(const QString& localFolder);

        bool isLocalFolderExists() const;
        void setLocalFolderExists(bool state);

        uint64_t getSize() const;
        void setSize(uint64_t size);

        uint32_t getReplicatorsCount() const;
        void setReplicatorsCount(uint32_t count);

        std::vector<QString> getLastOpenedPath() const;
        void setLastOpenedPath(const std::vector<QString>& path);

        std::array<uint8_t, 32> getRootHash() const;
        void setRootHash(const std::array<uint8_t, 32>& rootHash);

        sirius::drive::FsTree getFsTree() const;
        void setFsTree(const sirius::drive::FsTree& fsTree);

        bool isDownloadingFsTree() const;
        void setDownloadingFsTree(bool state);

        std::shared_ptr<LocalDriveItem> getLocalDriveItem() const;
        void setLocalDriveItem(std::shared_ptr<LocalDriveItem> item);

        sirius::drive::ActionList getActionsList() const;
        void setActionsList(const sirius::drive::ActionList& actions);

        std::array<uint8_t, 32> getModificationHash() const;
        void setModificationHash(const std::array<uint8_t, 32>& modificationHash );

        DriveState getState() const;

        sirius::drive::ReplicatorList getReplicators() const;
        void setReplicators(const std::vector<QString>& replicators);
    
        const sirius::drive::ReplicatorList& replicatorList() const { return m_replicatorList; }
    
        endpoint_list getEndpointReplicatorList() const;

        bool isStreaming() const { return m_streamStatus != ss_no_stream; }
        void setCreatingStreamAnnouncement() { m_streamStatus = ss_creating_announce; }
        void setIsStreaming() { m_streamStatus = ss_streaming; }
        void resetStreamingStatus() { m_streamStatus = ss_no_stream; }
        auto getStreamingStatus() const { return m_streamStatus; }

        void updateDriveState(DriveState state);

        template<class Archive>
        void serialize( Archive &ar )
        {
            ar(m_driveKey,
               m_name,
               m_localDriveFolder,
               m_size,
               m_replicatorNumber,
               m_replicatorList,
               m_rootHash );
        }

    private:
        QString m_driveKey;
        QString m_name;
        QString m_localDriveFolder;
        bool m_localDriveFolderExists;
        uint64_t m_size;
        uint32_t m_replicatorNumber;
        sirius::drive::ReplicatorList m_replicatorList;
        std::vector<QString> m_lastOpenedPath;

        std::array<uint8_t, 32> m_rootHash;
        sirius::drive::FsTree m_fsTree;
        bool m_downloadingFsTree;

        // diff
        std::shared_ptr<LocalDriveItem> m_localDrive;
        sirius::drive::ActionList m_actionList;
        std::array<uint8_t, 32> m_currentModificationHash;
        DriveState m_driveState;

public:
        enum StreamStatus { ss_no_stream, ss_creating_announce, ss_streaming };
private:
        StreamStatus m_streamStatus = ss_no_stream;
};


#endif //DRIVE_H
