#ifndef DRIVE_H
#define DRIVE_H

#include <QObject>
#include <QStateMachine>
#include "drive/FsTree.h"
#include "LocalDriveItem.h"
#include "drive/ActionList.h"

enum DriveState
{
    creating,
    unconfirmed,
    deleting,
    deleted,
    deleteIsFailed,
    no_modifications,
    registering,
    approved,
    uploading,
    failed,
    canceling,
    canceled
};

class DriveModificationEvent;

class Drive : public QObject
{
    Q_OBJECT

    public:
        explicit Drive(QObject* parent = nullptr);
        Drive(const Drive& drive);
        ~Drive() override;

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

        std::optional<std::array<uint8_t, 32>> getRootHash() const;
        void setRootHash(const std::array<uint8_t, 32>& rootHash);

        sirius::drive::FsTree getFsTree() const;
        void setFsTree(const sirius::drive::FsTree& fsTree);

        bool isDownloadingFsTree() const;
        void setDownloadingFsTree(bool state);

        bool isWaitingFsTree() const;
        void setWaitingFsTree(bool state);

        std::shared_ptr<LocalDriveItem> getLocalDriveItem() const;
        void setLocalDriveItem(std::shared_ptr<LocalDriveItem> item);

        sirius::drive::ActionList getActionsList() const;
        void setActionsList(const sirius::drive::ActionList& actions);

        std::optional<std::array<uint8_t, 32>> getModificationHash() const;
        void setModificationHash(const std::array<uint8_t, 32>& modificationHash);

        DriveState getState() const;

        void updateState(DriveState state);

        template<class Archive>
        void serialize( Archive &ar )
        {
            ar(m_driveKey,
               m_name,
               m_localDriveFolder,
               m_size,
               m_replicatorNumber );
        }

    signals:
        void initialized(const std::string& driveKey);
        void stateChanged(const std::string& driveKey, int state);

    private:
        void initStateMachine(DriveState initialState);

    private:
        std::string m_driveKey;
        std::string m_name;
        std::string m_localDriveFolder;
        bool m_localDriveFolderExists = false;
        uint64_t m_size = 0;
        uint32_t m_replicatorNumber = 0;
        std::vector<std::string> m_lastOpenedPath;

        std::optional<std::array<uint8_t, 32>> m_rootHash;
        sirius::drive::FsTree m_fsTree;
        bool m_downloadingFsTree = false;
        bool m_calculateDiffIsWaitingFsTree = false;

        // diff
        std::shared_ptr<LocalDriveItem> m_localDrive;
        sirius::drive::ActionList m_actionList;
        std::optional<std::array<uint8_t, 32>> m_currentModificationHash;
        DriveState m_driveState;
        QStateMachine *mp_stateMachine;
        std::vector<DriveModificationEvent*> m_modificationEvents;
        bool m_isInitialized = false;
};


#endif //DRIVE_H
