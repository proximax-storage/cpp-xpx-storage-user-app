#pragma once

#include <QMainWindow>
#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QListWidget>
#include <QCloseEvent>
#include <QComboBox>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWin; }
QT_END_NAMESPACE

class FsTreeTableModel;
class DownloadChannel;
class Drive;
class DownloadsTableModel;
class DriveTreeModel;
class DiffTableModel;
class OnChainClient;
class ModifyProgressPanel;
class Model;
class Settings;

namespace sirius { namespace drive
{
    class FsTree;
}}

class MainWin : public QMainWindow
{
    Q_OBJECT

public:
    MainWin(QWidget *parent = nullptr);
    ~MainWin();

    void init();

    void addChannel( const std::string&              channelName,
                     const std::string&              channelKey,
                     const std::string&              driveKey,
                     const std::vector<std::string>& allowedPublicKeys );

signals:
    void drivesInitialized();

private:
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
    void loadBalance();
    void setupDrivesTab();
    void setupNotifications();
    void showNotification(const QString& message, const QString& error = {});
    void addNotification(const QString& message);
    void onFsTreeReceived( const std::string& driveKey, const std::array<uint8_t,32>& fsTreeHash, const sirius::drive::FsTree& );
    void lockMainButtons(bool state);
    void closeEvent(QCloseEvent* event) override;
    void addLocalModificationsWatcher();
    bool isCurrentDrive(Drive* drive);

private slots:
    void checkDriveForUpdates(Drive* drive, const std::function<void(bool)>& callback);
    void checkDriveForUpdates(DownloadChannel* channel, const std::function<void(bool)>& callback);
    void updateReplicatorsForChannel(const std::string& channelId, const std::function<void()>& callback);
    void onInternalError(const QString& errorText);
    void setCurrentDriveOnUi(const std::string& driveKey);
    void onDriveStateChanged(const std::string& driveKey, int state);
    void updateEntityNameOnUi(QComboBox* box, const std::string& name, const std::string& key);
    void updateDownloadChannelStatusOnUi(const DownloadChannel& channel);
    void updateDriveStatusOnUi(const Drive& drive);
    void addEntityToUi(QComboBox* box, const std::string& name, const std::string& key);
    void removeEntityFromUi(QComboBox* box, const std::string& key);
    void lockChannel(const std::string& channelId);
    void unlockChannel(const std::string& channelId);
    void lockDrive();
    void unlockDrive();
    void onDownloadFsTreeDirect(const std::string& driveId, const std::string& fileStructureCdi);
    void downloadFsTreeByChannel(const std::string& channelId, const std::string& fileStructureCdi);

public:
    // if private key is not set it will be 'true'
    bool          m_mustExit = false;

private:
    Ui::MainWin*            ui;

    QTimer*                 m_downloadUpdateTimer;

    FsTreeTableModel*       m_channelFsTreeTableModel;
    DownloadsTableModel*    m_downloadsTableModel;
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
};