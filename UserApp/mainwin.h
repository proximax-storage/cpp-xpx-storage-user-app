#pragma once

#include <QMainWindow>
#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QListWidget>
#include <QCloseEvent>
#include <QComboBox>
#include <QProcess>
#include "Worker.h"
#include "OnChainClient.h"
#include "types.h"
#include "drive/FlatDrive.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWin; }
QT_END_NAMESPACE

class FsTreeTableModel;
class DownloadChannel;
class Drive;
class DownloadsTableModel;
class ReplicatorTreeModel;
class DriveTreeModel;
class DiffTableModel;
class ModifyProgressPanel;
class Model;
class Settings;
class ContractDeploymentData;

struct StreamInfo;

namespace sirius { namespace drive
{
    class FsTree;
    struct DriveKey;
}}

class MainWin : public QMainWindow
{
    Q_OBJECT
    static MainWin* m_instance;

public:
    MainWin(QWidget *parent = nullptr);
    ~MainWin();

    static MainWin* instance() { return m_instance; }
    
    void init();

    void addChannel( const std::string&              channelName,
                     const std::string&              channelKey,
                     const std::string&              driveKey,
                     const std::vector<std::string>& allowedPublicKeys );

    void removeDrive( Drive* drive );
    
    void onDriveStateChanged( Drive& );

signals:
    void refreshMyReplicatorsTable();
    void addResolver(const QUuid& id, const std::function<void(QVariant)>& resolver);
    void removeResolver(const QUuid& id);
    void runProcess(const QUuid& id, const std::function<QVariant()>& task);
    void updateUploadedDataAmount(const uint64_t receivedSize);
    void modificationFinishedByReplicators();
    void driveStateChangedSignal(const std::string& driveKey, int state, bool itIsNewState );

private:
    void drivesInitialized();
    bool requestPrivateKey();
    void cancelModification();

    void setupIcons();
    void setupDownloadsTab();
    void setupDownloadsTable();
    void onDownloadBtn();

    void setupChannelFsTable();
    void selectChannelFsItem( int index );

    void setupDriveFsTable();
    void selectDriveFsItem( int index );

    void onChannelCreationConfirmed( const std::string& alias, const std::string& channelKey, const std::string& driveKey );
    void onChannelCreationFailed( const std::string& channelKey, const std::string& errorText );
    void onCurrentChannelChanged( int index );
    void onDriveLocalDirectoryChanged(const QString& path);
    void onDriveCreationConfirmed( const std::string& driveKey );
    void onDriveCreationFailed( const std::string& driveKey, const std::string& errorText );
    void onDriveCloseConfirmed( const std::array<uint8_t, 32>& driveKey );
    void onDriveCloseFailed( const std::array<uint8_t, 32>& driveKey, const QString& errorText );
    void onCurrentDriveChanged( int index );
    void onApplyChanges();
    void onRefresh();
    void onDownloadChannelCloseConfirmed(const std::array<uint8_t, 32>& channelId);
    void onDownloadChannelCloseFailed(const std::array<uint8_t, 32>& channelId, const QString& errorText);
    void onDownloadPaymentConfirmed(const std::array<uint8_t, 32>& channelId);
    void onDownloadPaymentFailed(const std::array<uint8_t, 32> &channelId, const QString& errorText);
    void onStoragePaymentConfirmed(const std::array<uint8_t, 32>& driveKey);
    void onStoragePaymentFailed(const std::array<uint8_t, 32> &driveKey, const QString& errorText);
    void onDataModificationTransactionConfirmed(const std::array<uint8_t, 32>& driveKey, const std::array<uint8_t, 32>& modificationId);
    void onDataModificationTransactionFailed(const std::array<uint8_t, 32>& driveKey, const std::array<uint8_t, 32>& modificationId);
    void onDataModificationApprovalTransactionConfirmed(const std::array<uint8_t, 32>& driveId, const std::string& fileStructureCdi);
    void onDataModificationApprovalTransactionFailed(const std::array<uint8_t, 32>& driveId, const std::string& fileStructureCdi, uint8_t);
    void onCancelModificationTransactionConfirmed(const std::array<uint8_t, 32>& driveId, const QString& modificationId);
    void onCancelModificationTransactionFailed(const std::array<uint8_t, 32>& driveId, const QString& modificationId);
    void onReplicatorOnBoardingTransactionConfirmed(const QString& replicatorPublicKey);
    void onReplicatorOnBoardingTransactionFailed(const QString& replicatorPublicKey, const QString& replicatorPrivateKey);
    void onReplicatorOffBoardingTransactionConfirmed(const QString& replicatorPublicKey);
    void onReplicatorOffBoardingTransactionFailed(const QString& replicatorPublicKey);
    void onStartStreamingBtn();
    void loadBalance();
    void setupDrivesTab();
    void setupMyReplicatorTab();
    void setupNotifications();
    void showNotification(const QString& message, const QString& error = {});
    void addNotification(const QString& message);
    void onFsTreeReceived( const std::string& driveKey, const std::array<uint8_t,32>& fsTreeHash, const sirius::drive::FsTree& );
    void lockMainButtons(bool state);
    void closeEvent(QCloseEvent* event) override;
    void addLocalModificationsWatcher();
    bool isCurrentDownloadChannel(DownloadChannel* channel);
    bool isCurrentDrive(Drive* drive);
    void updateDiffView();
    void updateDriveView();
    void updateDownloadChannelData(DownloadChannel* channel);
    void getMosaicIdByName(const QString& accountPublicKey, const QString& mosaicName, std::function<void(uint64_t id)> callback);

    void onDeployContract();
    void onRunContract();

    void onDeployContractTransactionConfirmed(std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId);
    void onDeployContractTransactionFailed(std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId);
    void onDeployContractApprovalTransactionConfirmed(std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId);
    void onDeployContractApprovalTransactionFailed(std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId);

    QString currentStreamingDriveKey() const;
    Drive* currentStreamingDrive() const;
    void initStreaming();
    void updateStreamerTable( Drive& );
    void readStreamingAnnotations( const Drive& );
    void onFsTreeReceivedForStreamAnnotations( const Drive& drive );
    void updateViewerCBox();

    void startViewingStream();
    void updateCreateChannelStatusForVieweing( const DownloadChannel& channel );
    void startViewingStream2();
    void cancelViewingStream();

    void onStreamStatusResponse( const sirius::drive::DriveKey& driveKey,
                                 bool                           isStreaming,
                                 const std::array<uint8_t,32>&  streamId );

    void updateViewerProgressPanel( int tabIndex );

    void startFfmpegStreamingProcess();

    void cancelOrFinishStreaming();
    void updateStreamerProgressPanel( int tabIndex );

    void initWorker();

    void callbackResolver(const QUuid& id, const QVariant& data);

    ContractDeploymentData* contractDeploymentData();

//    void validateContractDrive();

    template<class Type>
    void typeResolver(const QVariant& data, const std::function<void(Type, std::string)>& callback) {
        if (data.canConvert<Type>()) {
            callback(data.value<Type>(), "");
        } else if (data.canConvert<std::string>()) {
            callback(Type(), data.value<std::string>());
        } else {
            callback(Type(), "unknown error");
        }
    }
    
    void dbg();

    void updateDriveWidgets(const std::string& driveKey, int state, bool itIsNewState );

private slots:
    void checkDriveForUpdates(Drive* drive, const std::function<void(bool)>& callback);
    void checkDriveForUpdates(DownloadChannel* channel, const std::function<void(bool)>& callback);
    void updateReplicatorsForChannel(const std::string& channelId, const std::function<void()>& callback);
    void onErrorsHandler(int errorType, const QString& errorText);
    void setDownloadChannelOnUi(const std::string& channelId);
    void setCurrentDriveOnUi(const std::string& driveKey);
    void showSettingsDialog();
    void updateEntityNameOnUi(QComboBox* box, const std::string& name, const std::string& key);
    void updateDownloadChannelStatusOnUi(const DownloadChannel& channel);
    void updateDriveStatusOnUi(const Drive& drive);
    void addEntityToUi(QComboBox* box, const std::string& name, const std::string& key);
    void removeEntityFromUi(QComboBox* box, const std::string& key);
    void lockChannel(const std::string& channelId);
    void unlockChannel(const std::string& channelId);
    void lockDrive();
    void unlockDrive();
    void networkDataHandler(const QString networkName);
    void onDownloadFsTreeDirect(const std::string& driveId, const std::string& fileStructureCdi);
    void downloadFsTreeByChannel(const std::string& channelId, const std::string& fileStructureCdi);
    void onAddResolver(const QUuid& id, const std::function<void(QVariant)>& resolver);
    void onRemoveResolver(const QUuid& id);
    void calculateDiffAsync(const std::function<void(int, std::string)>& callback);
    void dataModificationsStatusHandler(const sirius::drive::ReplicatorKey &replicatorKey,
                                        const sirius::Hash256 &modificationHash,
                                        const sirius::drive::ModifyTrafficInfo &msg,
                                        lt::string_view currentTask,
                                        bool isModificationQueued,
                                        bool isModificationFinished,
                                        const std::string &error);

public:
    // if private key is not set it will be 'true'
    bool          m_mustExit = false;

private:
    Ui::MainWin*            ui;

    QTimer*                 m_downloadUpdateTimer;
    QTimer*                 m_modificationStatusTimer;

    FsTreeTableModel*       m_channelFsTreeTableModel;
    DownloadsTableModel*    m_downloadsTableModel;
    ReplicatorTreeModel*    m_myReplicatorsModel;
    Settings*               m_settings;
    Model*                  m_model;

    FsTreeTableModel*       m_driveTableModel;
    DriveTreeModel*         m_driveTreeModel;
    DiffTableModel*         m_diffTableModel;
    DriveTreeModel*         m_diffTreeModel;

    OnChainClient*          m_onChainClient;
    QFileSystemWatcher*     m_modificationsWatcher;

    ModifyProgressPanel*    m_modifyProgressPanel;
    QListWidget*            m_notificationsWidget;
    uint64_t                m_XPX_MOSAIC_ID;
    
    ModifyProgressPanel*    m_startViewingProgressPanel;
    ModifyProgressPanel*    m_streamingProgressPanel;
    QProcess*               m_ffmpegStreamingProcess;

    Worker*                 mpWorker;
    QThread*                mpThread;
    std::map<QUuid, std::function<void(QVariant)>> mResolvers;
};
