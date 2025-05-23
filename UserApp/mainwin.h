#pragma once

#include <QMainWindow>
#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QListWidget>
#include <QCloseEvent>
#include <QComboBox>
#include <QProcess>
#include <QTableWidget>
#include "Entities/DataInfo.h"
#include "types.h"
#include "CustomLogsRedirector.h"
#include "Dialogs/ModalDialog.h"
#include "drive/FlatDrive.h"
#include "Entities/Drive.h"

#include <boost/beast/core/string_type.hpp>

class OnChainClient;
class Worker;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWin; }
QT_END_NAMESPACE

class FsTreeTableModel;
class DownloadChannel;
class Drive;
class DownloadsTableModel;
class EasyDownloadTableModel;
class ReplicatorTreeModel;
class DriveTreeModel;
class DiffTableModel;
class ModifyProgressPanel;
class StreamingView;
class StreamingPanel;
class Model;
class Settings;
class ContractDeploymentData;
class WizardAddStreamAnnounceDialog;
class AddDownloadChannelDialog;

struct StreamInfo;
struct EasyDownloadInfo;

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
    
    void displayError(  const QString& text, const QString& informativeText = "" );

    void addChannel( const QString&              channelName,
                     const QString&              channelKey,
                     const QString&              driveKey,
                     const std::vector<QString>& allowedPublicKeys,
                     bool                        isForEasyDownload );

    void removeDrive( Drive* drive );
    
    void onDriveStateChanged( Drive& );
    
    void updateDriveState( Drive* drive, DriveState state );

signals:
    void refreshMyReplicatorsTable();
    void addResolver(const QUuid& id, const std::function<void(QVariant)>& resolver);
    void removeResolver(const QUuid& id);
    void runProcess(const QUuid& id, const std::function<QVariant()>& task);
    void updateUploadedDataAmount(uint64_t receivedSize, uint64_t replicatorNumber);
    void modificationFinishedByReplicators();
    void driveStateChangedSignal(const QString& driveKey, int state, bool itIsNewState );
    void updateStreamingStatus( const QString& );


private:
    void initGeometry();
    void drivesInitialized();
    bool requestPrivateKey();
    void cancelModification();
    void doUpdateBalancePeriodically();

    void setupIcons();
    void setupDownloadsTab();
    void setupDownloadsTable();
    void onDownloadBtn();

    void setupEasyDownloads();

    void setupChannelFsTable();
    void selectChannelFsItem( int index );

    void setupDriveFsTable();
    void selectDriveFsItem( int index );

    void onChannelCreationConfirmed( const QString& alias, const QString& channelKey, const QString& driveKey );
    void onChannelCreationFailed( const QString& channelKey, const QString& errorText );
    void onCurrentChannelChanged( int index );
    void onDriveLocalDirectoryChanged(const QString& path);
    void onDriveCreationConfirmed( const QString& driveKey );
    void onDriveCreationFailed( const QString& driveKey, const QString& errorText );
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
    void onDataModificationTransactionFailed(const std::array<uint8_t, 32>& driveKey, const std::array<uint8_t, 32>& modificationId, const QString& status);
    void onDataModificationApprovalTransactionConfirmed(const std::array<uint8_t, 32>& driveId, const QString& fileStructureCdi);
    void onDataModificationApprovalTransactionFailed(const std::array<uint8_t, 32>& driveId, const QString& fileStructureCdi, uint8_t);
    void onCancelModificationTransactionConfirmed(const std::array<uint8_t, 32>& driveId, const QString& modificationId);
    void onCancelModificationTransactionFailed(const std::array<uint8_t, 32>& driveId, const QString& modificationId, const QString& error);
    void onReplicatorOnBoardingTransactionConfirmed(const QString& replicatorPublicKey, bool isExists);
    void onReplicatorOnBoardingTransactionFailed(const QString& replicatorPublicKey, const QString& replicatorPrivateKey, const QString& error);
    void onReplicatorOffBoardingTransactionConfirmed(const QString& replicatorPublicKey);
    void onReplicatorOffBoardingTransactionFailed(const QString& replicatorPublicKey, const QString& error);
    void startStreamingProcess( const StreamInfo& streamInfo );
    void startStreamingProcess( const Drive& driveInfo );
    void onStartStreamingBtnFfmpeg();
    void loadBalance();
    void setupDrivesTab();
    void setupMyReplicatorTab();
    void setupNotifications();
    void showNotification(const QString& message, const QString& error = {});
    void addNotification(const QString& message);
    void onFsTreeReceived( const QString& driveKey, const std::array<uint8_t,32>& fsTreeHash, const sirius::drive::FsTree& );
    void lockMainButtons(bool state);
    void closeEvent(QCloseEvent* event) override;
    void addLocalModificationsWatcher();
    bool isCurrentDownloadChannel(DownloadChannel* channel);
    bool isCurrentDrive(Drive* drive);
    void updateDiffView();
    void updateDriveView();
    void updateDownloadChannelData(DownloadChannel* channel);
    void getMosaicIdByName(const QString& accountPublicKey, const QString& mosaicName, std::function<void(uint64_t id)> callback);
    void deleteEasyDnlLocalFiles(QModelIndexList rows);

    void onDeployContract();
    void onRunContract();

    void onDeployContractTransactionConfirmed(std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId);
    void onDeployContractTransactionFailed(std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId);
    void onDeployContractApprovalTransactionConfirmed(std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId);
    void onDeployContractApprovalTransactionFailed(std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId);
    QString selectedDriveKeyInTable() const;
    Drive*  selectedDriveInTable() const;
    QString selectedDriveKeyInWizardTable(QTableWidget* table) const;
    Drive*  selectedDriveInWizardTable(QTableWidget* table) const;
    Drive*  currentStreamingDrive() const;

    std::vector<DataInfo> readDataInfoList();
public:
    void startEasyDownload( const DataInfo& dataInfo, bool isInterrupted );
    void continueEasyDownload( uint64_t downloadId, const QString& channelKey );
    void restartInterruptedEasyDownloads();
private:
    sirius::drive::FsTree m_easyDownloadFsTree;
    
private:

    bool checkObsConfiguration();
    void initStreaming();
    void initWizardStreaming();
    void initWizardArchiveStreaming();

    void connectToStreamingTransactions();

    std::optional<StreamInfo> selectedStreamInfo(); // could return nullptr
    std::optional<StreamInfo> wizardSelectedStreamInfo(QTableWidget* table); // could return nullptr

    void updateStreamerTable( const Drive& drive );
    void wizardUpdateStreamAnnouncementTable();
    void wizardUpdateArchiveTable();

    std::vector<StreamInfo> readStreamInfoList( const Drive& );
    std::vector<StreamInfo> wizardReadStreamInfoList();

    void onFsTreeReceivedForStreamAnnotations( const Drive& drive );
    void updateViewerCBox();

    void startViewingStream();
    void startViewingStream2( DownloadChannel& channel );
    bool startViewingApprovedStream( DownloadChannel& channel );
    void downloadApprovedChunk( QString dnChannelKey, QString destFolder, int chunkIndex );
    void cancelViewingStream();

    void waitStream();
    void onStreamStatusResponse( const sirius::drive::DriveKey& driveKey,
                                 const sirius::Key&             streamerKey,
                                 bool                           isStreaming,
                                 const std::array<uint8_t,32>&  streamId );

    void updateViewerProgressPanel( int tabIndex );

    void startFfmpegStreamingProcess();

    void cancelStreaming();
    void finishStreaming();
    void updateStreamerProgressPanel( int tabIndex );

    void initWorker();

    void callbackResolver(const QUuid& id, const QVariant& data);

    ContractDeploymentData* contractDeploymentData();

//    void validateContractDrive();

    template<class Type>
    void typeResolver(const QVariant& data, const std::function<void(Type, QString)>& callback) {
        if (data.canConvert<Type>()) {
            callback(data.value<Type>(), "");
        } else if (data.canConvert<QString>()) {
            callback(Type(), data.value<QString>());
        } else {
            callback(Type(), "unknown error");
        }
    }
    
    void dbg();

    void updateDriveWidgets(const QString& driveKey, int state, bool itIsNewState );

private slots:
    void checkDriveForUpdates(Drive* drive, const std::function<void(bool)>& callback);
    void checkDriveForUpdates(DownloadChannel* channel, const std::function<void(bool)>& callback);
    void updateReplicatorsForChannel(const QString& channelId, const std::function<void()>& callback);
    void onErrorsHandler(int errorType, const QString& errorText);
    void setDownloadChannelOnUi(const QString& channelId);
    void setCurrentDriveOnUi(const QString& driveKey);
    void showSettingsDialog();
    void updateEntityNameOnUi(QComboBox* box, const QString& name, const QString& key);
    void updateDownloadChannelStatusOnUi(const DownloadChannel& channel);
    void updateDriveStatusOnUi(const Drive& drive);
    void addEntityToUi(QComboBox* box, const QString& name, const QString& key);
    void removeEntityFromUi(QComboBox* box, const QString& key);
    void lockChannel(const QString& channelId);
    void unlockChannel(const QString& channelId);
    void lockDrive();
    void unlockDrive();
    void networkDataHandler(const QString networkName);
    void onCloseChannel();
    void onCloseDrive();
    void onDownloadFsTreeDirect(const QString& driveId, const QString& fileStructureCdi);
    void downloadFsTreeByChannel(const QString& channelId, const QString& fileStructureCdi);
    void onAddResolver(const QUuid& id, const std::function<void(QVariant)>& resolver);
    void onRemoveResolver(const QUuid& id);
    void calculateDiffAsync(const std::function<void(int, QString)>& callback);
    void dataModificationsStatusHandler(const sirius::drive::ReplicatorKey &replicatorKey,
                                        const sirius::Hash256 &modificationHash,
                                        const sirius::drive::ModifyTrafficInfo &msg,
                                        boost::string_view currentTask,
                                        bool isModificationQueued,
                                        bool isModificationFinished,
                                        const QString &error);
    //void on_m_wizardAddStreamAnnouncementBtn_clicked();

#ifndef WA_APP
    //TODO_WA
    void onRowsRemovedAnnouncements();
    void onRowsInsertedAnnouncements();
    void onRowsRemovedArchive();
    void onRowsInsertedArchive();
    void hideWizardUi();
#endif

    void on_m_streamingTabView_currentChanged(int index);
    void invokePasteLinkDialog();
    void cleanEasyDownloadsTable();

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
    EasyDownloadTableModel* m_easyDownloadTableModel;
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
    StreamingView*          m_streamingView = nullptr;
    StreamingPanel*         m_streamingPanel = nullptr;
    bool                    m_startStreamingNow = false;

    Worker*                 mpWorker;
    QThread*                mpThread;
    std::map<QUuid, std::function<void(QVariant)>> mResolvers;

    std::shared_ptr<CustomLogsRedirector>     mpCustomCoutStream;
    std::shared_ptr<CustomLogsRedirector>     mpCustomCerrorStream;
    std::shared_ptr<CustomLogsRedirector>     mpCustomClogStream;

    // For notification (when drive is created)
    //WizardAddStreamAnnounceDialog*            m_wizardAddStreamAnnounceDialog = nullptr;
    
    ModalDialog*                              m_modalDialog = nullptr;
};
