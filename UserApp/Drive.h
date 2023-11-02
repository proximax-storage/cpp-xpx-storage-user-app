#ifndef DRIVE_H
#define DRIVE_H

#include <QObject>
#include "drive/FsTree.h"
#include "LocalDriveItem.h"
#include "drive/ActionList.h"

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
        std::string getKey() const;
        void setKey(const std::string& key);

        std::string getName() const;
        void setName(const std::string& name);

        std::string getLocalFolder() const;
        void setLocalFolder(const std::string& localFolder);

        bool isLocalFolderExists() const;
        void setLocalFolderExists(bool state);

        uint64_t getSize() const;
        void setSize(uint64_t size);

        uint32_t getReplicatorsCount() const;
        void setReplicatorsCount(uint32_t count);

        std::vector<std::string> getLastOpenedPath() const;
        void setLastOpenedPath(const std::vector<std::string>& path);

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
        void setModificationHash(const std::array<uint8_t, 32>& modificationHash, bool isStreaming = false );

        DriveState getState() const;

        sirius::drive::ReplicatorList getReplicators() const;
        void setReplicators(const std::vector<std::string>& replicators);
    
        const sirius::drive::ReplicatorList& replicatorList() const { return m_replicatorList; }
    
        endpoint_list getEndpointReplicatorList() const;

        bool isStreaming() const { return m_isStreaming; }
        void setIsStreaming( bool isStreaming ) { m_isStreaming = isStreaming; }

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
        std::string m_driveKey;
        std::string m_name;
        std::string m_localDriveFolder;
        bool m_localDriveFolderExists;
        uint64_t m_size;
        uint32_t m_replicatorNumber;
        sirius::drive::ReplicatorList m_replicatorList;
        std::vector<std::string> m_lastOpenedPath;

        std::array<uint8_t, 32> m_rootHash;
        sirius::drive::FsTree m_fsTree;
        bool m_downloadingFsTree;

        // diff
        std::shared_ptr<LocalDriveItem> m_localDrive;
        sirius::drive::ActionList m_actionList;
        std::array<uint8_t, 32> m_currentModificationHash;
        bool m_isStreaming = false;
        DriveState m_driveState;
};


#endif //DRIVE_H
