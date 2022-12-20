#include "Drive.h"
#include "Model.h"
#include "DriveModificationTransition.h"
#include "DriveModificationEvent.h"

#include <boost/algorithm/string.hpp>


Drive::Drive(QObject* parent)
    : QObject(parent)
    , m_localDrive(std::make_shared<LocalDriveItem>())
{
    initStateMachine(creating);
}

Drive::~Drive()
{}

Drive::Drive(const Drive &drive) {
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
    m_calculateDiffIsWaitingFsTree = drive.m_calculateDiffIsWaitingFsTree;

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
    initStateMachine(m_driveState);
}

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
    m_calculateDiffIsWaitingFsTree = drive.m_calculateDiffIsWaitingFsTree;

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
    initStateMachine(m_driveState);
    return *this;
}

std::string Drive::getKey() const {
    return m_driveKey;
}

void Drive::setKey(const std::string &key) {
    m_driveKey = key;
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

std::optional<std::array<uint8_t, 32>> Drive::getRootHash() const {
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

bool Drive::isWaitingFsTree() const {
    return m_calculateDiffIsWaitingFsTree;
}

void Drive::setWaitingFsTree(bool state) {
    m_calculateDiffIsWaitingFsTree = state;
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

std::optional<std::array<uint8_t, 32>> Drive::getModificationHash() const {
    return m_currentModificationHash;
}

void Drive::setModificationHash(const std::array<uint8_t, 32>& modificationHash) {
    m_currentModificationHash = modificationHash;
}

DriveState Drive::getState() const {
    return m_driveState;
}

void Drive::updateState(DriveState state) {
    if (!m_isInitialized) {
        m_modificationEvents.push_back(new DriveModificationEvent(state));
    } else {
        mp_stateMachine->postEvent(new DriveModificationEvent(state));
    }
}

void Drive::initStateMachine(DriveState initialState) {
    mp_stateMachine = new QStateMachine(this);
    auto creatingState = new QState();
    auto unConfirmedState = new QState();
    auto deletingState = new QState();
    auto deletedState = new QState();
    auto deleteIsFailedState = new QState();
    auto noModificationsState = new QState();
    auto registeringState = new QState();
    auto uploadingState = new QState();
    auto approvedState = new QState();
    auto cancelingState = new QState();
    auto canceledState = new QState();
    auto failedState = new QState();

    auto creatingToNoModificationsTransition = new DriveModificationTransition(no_modifications);
    creatingToNoModificationsTransition->setTargetState(noModificationsState);
    creatingState->addTransition(creatingToNoModificationsTransition);

    auto creatingToUnConfirmedTransition = new DriveModificationTransition(unconfirmed);
    creatingToUnConfirmedTransition->setTargetState(unConfirmedState);
    creatingState->addTransition(creatingToUnConfirmedTransition);

    auto noModificationsToRegisteringTransition = new DriveModificationTransition(registering);
    noModificationsToRegisteringTransition->setTargetState(registeringState);
    noModificationsState->addTransition(noModificationsToRegisteringTransition);

    auto noModificationsToDeletingTransition = new DriveModificationTransition(deleting);
    noModificationsToDeletingTransition->setTargetState(deletingState);
    noModificationsState->addTransition(noModificationsToDeletingTransition);

    auto deletingToDeletedTransition = new DriveModificationTransition(deleted);
    deletingToDeletedTransition->setTargetState(deletedState);
    deletingState->addTransition(deletingToDeletedTransition);

    auto deletingToDeleteIsFailedTransition = new DriveModificationTransition(deleteIsFailed);
    deletingToDeleteIsFailedTransition->setTargetState(deleteIsFailedState);
    deletingState->addTransition(deletingToDeleteIsFailedTransition);

    auto deleteIsFailedToNoModificationsTransition = new DriveModificationTransition(no_modifications);
    deleteIsFailedToNoModificationsTransition->setTargetState(noModificationsState);
    deleteIsFailedState->addTransition(deleteIsFailedToNoModificationsTransition);

    auto registeringToCancelingTransition = new DriveModificationTransition(canceling);
    registeringToCancelingTransition->setTargetState(cancelingState);
    registeringState->addTransition(registeringToCancelingTransition);

    auto registeringToFailedTransition = new DriveModificationTransition(failed);
    registeringToFailedTransition->setTargetState(failedState);
    registeringState->addTransition(registeringToFailedTransition);

    auto registeringToUploadingTransition = new DriveModificationTransition(uploading);
    registeringToUploadingTransition->setTargetState(uploadingState);
    registeringState->addTransition(registeringToUploadingTransition);

    auto uploadingToCancelingTransition = new DriveModificationTransition(canceling);
    uploadingToCancelingTransition->setTargetState(cancelingState);
    uploadingState->addTransition(uploadingToCancelingTransition);

    auto uploadingToApprovedTransition = new DriveModificationTransition(approved);
    uploadingToApprovedTransition->setTargetState(approvedState);
    uploadingState->addTransition(uploadingToApprovedTransition);

    auto uploadingToFailedTransition = new DriveModificationTransition(failed);
    uploadingToFailedTransition->setTargetState(failedState);
    uploadingState->addTransition(uploadingToFailedTransition);

    auto approvedToNoModificationsTransition = new DriveModificationTransition(no_modifications);
    approvedToNoModificationsTransition->setTargetState(noModificationsState);
    approvedState->addTransition(approvedToNoModificationsTransition);

    auto cancelingToApprovedTransition = new DriveModificationTransition(approved);
    cancelingToApprovedTransition->setTargetState(approvedState);
    cancelingState->addTransition(cancelingToApprovedTransition);

    auto cancelingToFailedTransition = new DriveModificationTransition(failed);
    cancelingToFailedTransition->setTargetState(failedState);
    cancelingState->addTransition(cancelingToFailedTransition);

    auto cancelingToCanceledTransition = new DriveModificationTransition(canceled);
    cancelingToCanceledTransition->setTargetState(canceledState);
    cancelingState->addTransition(cancelingToCanceledTransition);

    auto failedToNoModificationsTransition = new DriveModificationTransition(no_modifications);
    failedToNoModificationsTransition->setTargetState(noModificationsState);
    failedState->addTransition(failedToNoModificationsTransition);

    auto canceledToNoModificationsTransition = new DriveModificationTransition(no_modifications);
    canceledToNoModificationsTransition->setTargetState(noModificationsState);
    canceledState->addTransition(canceledToNoModificationsTransition);

    mp_stateMachine->addState(creatingState);
    mp_stateMachine->addState(unConfirmedState);
    mp_stateMachine->addState(deletingState);
    mp_stateMachine->addState(deletedState);
    mp_stateMachine->addState(deleteIsFailedState);
    mp_stateMachine->addState(noModificationsState);
    mp_stateMachine->addState(registeringState);
    mp_stateMachine->addState(uploadingState);
    mp_stateMachine->addState(approvedState);
    mp_stateMachine->addState(cancelingState);
    mp_stateMachine->addState(canceledState);
    mp_stateMachine->addState(failedState);

    switch (initialState) {
        case DriveState::no_modifications:
        {
            mp_stateMachine->setInitialState(noModificationsState);
            break;
        }
        case creating:
        {
            mp_stateMachine->setInitialState(creatingState);
            break;
        }
        case unconfirmed:
        {
            mp_stateMachine->setInitialState(unConfirmedState);
            break;
        }
        case deleting:
        {
            mp_stateMachine->setInitialState(deletingState);
            break;
        }
        case deleted:
        {
            mp_stateMachine->setInitialState(deletedState);
            break;
        }
        case deleteIsFailed:
        {
            mp_stateMachine->setInitialState(deleteIsFailedState);
            break;
        }
        case registering:
        {
            mp_stateMachine->setInitialState(registeringState);
            break;
        }
        case approved:
        {
            mp_stateMachine->setInitialState(approvedState);
            break;
        }
        case uploading:
        {
            mp_stateMachine->setInitialState(uploadingState);
            break;
        }
        case failed:
        {
            mp_stateMachine->setInitialState(failedState);
            break;
        }
        case canceling:
        {
            mp_stateMachine->setInitialState(cancelingState);
            break;
        }
        case canceled:
        {
            mp_stateMachine->setInitialState(canceledState);
            break;
        }
    }

    connect(mp_stateMachine, &QStateMachine::started, this, [this]() {
        m_isInitialized = true;
        emit initialized(m_driveKey);

        for (auto event : m_modificationEvents) {
            mp_stateMachine->postEvent(event);
        }

        m_modificationEvents.clear();
    });

    mp_stateMachine->start();

    QObject::connect(creatingState, &QState::entered, this, [this]() {
        m_driveState = creating;
        qDebug () << "m_driveState = creating;";
        emit stateChanged(m_driveKey, m_driveState);
    });

    QObject::connect(unConfirmedState, &QState::entered, this, [this]() {
        m_driveState = unconfirmed;
        qDebug () << "m_driveState = unconfirmed;";
        emit stateChanged(m_driveKey, m_driveState);
    });

    QObject::connect(deletingState, &QState::entered, this, [this]() {
        m_driveState = deleting;
        qDebug () << "m_driveState = deleting;";
        emit stateChanged(m_driveKey, m_driveState);
    });

    QObject::connect(deletedState, &QState::entered, this, [this]() {
        m_driveState = deleted;
        qDebug () << "m_driveState = deleted;";
        emit stateChanged(m_driveKey, m_driveState);
    });

    QObject::connect(deleteIsFailedState, &QState::entered, this, [this]() {
        m_driveState = deleteIsFailed;
        qDebug () << "m_driveState = deleteIsFailed;";
        emit stateChanged(m_driveKey, m_driveState);
    });

    QObject::connect(noModificationsState, &QState::entered, this, [this]() {
        m_driveState = no_modifications;
        qDebug () << "m_driveState = no_modifications;";
        emit stateChanged(m_driveKey, m_driveState);
    });

    QObject::connect(registeringState, &QState::entered, this, [this]() {
        m_driveState = registering;
        qDebug () << "m_driveState = is_registering;";
        emit stateChanged(m_driveKey, m_driveState);
    });

    QObject::connect(uploadingState, &QState::entered, this, [this]() {
        m_driveState = uploading;
        qDebug () << "m_driveState = uploadingState;";
        emit stateChanged(m_driveKey, m_driveState);
    });

    QObject::connect(approvedState, &QState::entered, this, [this]() {
        m_driveState = approved;
        qDebug () << "m_driveState = approvedState;";
        emit stateChanged(m_driveKey, m_driveState);
    });

    QObject::connect(cancelingState, &QState::entered, this, [this]() {
        m_driveState = canceling;
        qDebug () << "m_driveState = is_canceling;";
        emit stateChanged(m_driveKey, m_driveState);
    });

    QObject::connect(canceledState, &QState::entered, this, [this]() {
        m_driveState = canceled;
        qDebug () << "m_driveState = is_canceled;";
        emit stateChanged(m_driveKey, m_driveState);
    });

    QObject::connect(failedState, &QState::entered, this, [this]() {
        m_driveState = failed;
        qDebug () << "m_driveState = is_failed;";
        emit stateChanged(m_driveKey, m_driveState);
    });
}