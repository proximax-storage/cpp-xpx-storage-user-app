#pragma once

#include <QMainWindow>
#include <QFileSystemModel>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWin; }
QT_END_NAMESPACE

class FsTreeModel;
class FsTreeTableModel;
class ChannelInfo;
class DownloadsTableModel;
class DriveTreeModel;
class DiffTableModel;
class OnChainClient;

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

    void updateChannelsCBox();

    void onDriveCreationConfirmed( const std::string& alias, const std::string& driveKey );
    void onDriveCreationFailed( const std::string& alias, const std::string& driveKey, const std::string& errorText );
    void onDriveDeleted( const std::string& driveKey );
    void onCurrentDriveChanged( int index );
    void onApplyChanges();
    void onRefresh();

    void onFsTreeHashReceived( ChannelInfo& channel, const std::array<uint8_t,32>& fsTreeHash );

    void setDownloadPath( );

    void setupDrivesTab();
    void updateDrivesCBox();

public:
    // if private key is not set it will be 'true'
    bool          m_mustExit = false;

private:
    Ui::MainWin*            ui;

    QTimer*                 m_downloadUpdateTimer;

    FsTreeModel*            m_fsTreeModel;
    FsTreeTableModel*       m_fsTreeTableModel;
    DownloadsTableModel*    m_downloadsTableModel;
    DriveTreeModel*         m_driveTreeModel;
    DiffTableModel*         m_diffTableModel;
    OnChainClient*          m_onChainClient;
};
