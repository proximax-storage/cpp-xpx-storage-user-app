#pragma once

#include <QMainWindow>
#include <QFileSystemModel>
#include <QTimer>

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

    void setupIcons();
    void setupDownloadsTab();
    void setupFsTreeTable();
    void setupDownloadsTable();
    void selectFsTreeItem( int index );
    void onDownloadBtn();


    void onChannelCreationConfirmed( const std::string& alias, const std::string& channelKey, const std::string& driveKey );
    void onChannelCreationFailed( const std::string& channelKey, const std::string& errorText );
//    void onChannelDeleted( const std::string& channelKey );
    void onCurrentChannelChanged( int index );

    void onDriveCreationConfirmed( const std::string& alias, const std::string& driveKey );
    void onDriveCreationFailed( const std::string& alias, const std::string& driveKey, const std::string& errorText );
    void onDriveDeleted( const std::string& driveKey );
    void onCurrentDriveChanged( int index );
    void onApplyChanges();
    void onRefresh();
    void onDownloadPaymentConfirmed(const std::array<uint8_t, 32>& channelId);
    void onDownloadPaymentFailed(const std::array<uint8_t, 32> &channelId, const QString& errorText);
    void onStoragePaymentConfirmed(const std::array<uint8_t, 32>& driveKey);
    void onStoragePaymentFailed(const std::array<uint8_t, 32> &driveKey, const QString& errorText);
    void onDataModificationTransactionConfirmed(const std::array<uint8_t, 32>& modificationId);
    void onDataModificationTransactionFailed(const std::array<uint8_t, 32>& modificationId);
    void onDataModificationApprovalTransactionConfirmed(const std::array<uint8_t, 32>& driveId, const std::string& fileStructureCdi);
    void onDataModificationApprovalTransactionFailed(const std::array<uint8_t, 32>& driveId);
    void loadBalance();

    void setDownloadPath( );

    void setupDrivesTab();

    void downloadLatestFsTree( const std::string& driveKey );
    void onFsTreeReceived( const std::string& driveKey, const std::array<uint8_t,32>& fsTreeHash, const sirius::drive::FsTree& );
    void continueCalcDiff( DriveInfo& drive );

    void lockMainButtons(bool state);

private slots:
    void updateChannelsCBox();
    void updateDrivesCBox();

public:
    // if private key is not set it will be 'true'
    bool          m_mustExit = false;

private:
    Ui::MainWin*            ui;

    QTimer*                 m_downloadUpdateTimer;

    FsTreeTableModel*       m_fsTreeTableModel;
    DownloadsTableModel*    m_downloadsTableModel;
    DriveTreeModel*         m_driveTreeModel;
    DiffTableModel*         m_diffTableModel;
    OnChainClient*          m_onChainClient;

    std::string             m_lastSelectedChannelKey;
    std::string             m_lastSelectedDriveKey;
};
