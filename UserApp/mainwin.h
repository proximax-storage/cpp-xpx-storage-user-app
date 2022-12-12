#pragma once

#include <QMainWindow>
#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QListWidget>
#include <QCloseEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWin; }
QT_END_NAMESPACE

class FsTreeTableModel;
class ChannelInfo;
class DriveInfo;
class DownloadsTableModel;
class DriveTreeModel;
class DiffTableModel;
class OnChainClient;
class ModifyProgressPanel;

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

    void selectDownloadRow( int );

    void addChannel( const std::string&              channelName,
                     const std::string&              channelKey,
                     const std::string&              driveKey,
                     const std::vector<std::string>& allowedPublicKeys );

private:
    bool requestPrivateKey();

    void updateModificationStatus();
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
//    void onChannelDeleted( const std::string& channelKey );
    void onCurrentChannelChanged( int index );
    void onDriveLocalDirectoryChanged(const QString& path);
    void onDriveCreationConfirmed( const std::string& alias, const std::string& driveKey );
    void onDriveCreationFailed( const std::string& alias, const std::string& driveKey, const std::string& errorText );
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
    void onDataModificationApprovalTransactionFailed(const std::array<uint8_t, 32>& driveId);
    void onCancelModificationTransactionConfirmed(const std::array<uint8_t, 32>& driveId, const QString& modificationId);
    void onCancelModificationTransactionFailed(const std::array<uint8_t, 32>& driveId, const QString& modificationId);
    void loadBalance();

    void setDownloadPath( );
    void setupDrivesTab();
    void setupNotifications();
    void showNotification(const QString& message, const QString& error = {});
    void addNotification(const QString& message);

    void downloadLatestFsTree( const std::string& driveKey );
    void onFsTreeReceived( const std::string& driveKey, const std::array<uint8_t,32>& fsTreeHash, const sirius::drive::FsTree& );
    void continueCalcDiff( DriveInfo& drive );

    void startCalcDiff();

    void lockMainButtons(bool state);
    void closeEvent(QCloseEvent* event) override;
    void addLocalModificationsWatcher();

private slots:
    void updateChannelsCBox();
    void updateDrivesCBox();
    void lockChannel(const std::string& channelId);
    void unlockChannel(const std::string& channelId);
    void lockDrive(const std::string& driveId);
    void unlockDrive(const std::string& driveId);
    void markChannelsForDelete(const std::string& driveId, bool state);

public:
    // if private key is not set it will be 'true'
    bool          m_mustExit = false;

private:
    Ui::MainWin*            ui;

    QTimer*                 m_downloadUpdateTimer;

    FsTreeTableModel*       m_channelFsTreeTableModel;
    DownloadsTableModel*    m_downloadsTableModel;

    FsTreeTableModel*       m_driveFsTreeTableModel;
    DriveTreeModel*         m_driveTreeModel;
    DiffTableModel*         m_diffTableModel;
    DriveTreeModel*         m_diffTreeModel;

    OnChainClient*          m_onChainClient;
    QFileSystemWatcher*     m_modificationsWatcher;

    ModifyProgressPanel*    m_modifyProgressPanel;
    QListWidget*            m_notificationsWidget;

    std::string             m_lastSelectedChannelKey;
    std::string             m_lastSelectedDriveKey;
};
