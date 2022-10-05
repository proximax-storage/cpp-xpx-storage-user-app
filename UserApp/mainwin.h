#pragma once

#include <QMainWindow>
#include <QFileSystemModel>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWin; }
QT_END_NAMESPACE

class FsTreeTableModel;
class DownloadsTableModel;

class MainWin : public QMainWindow
{
    Q_OBJECT

public:
    MainWin(QWidget *parent = nullptr);
    ~MainWin();

    static MainWin& instanse() { return *m_instanse; }

    void selectDownloadRow( int );

private slots:
    void onSettingsBtn();
    void onDownloadUpdateTimer();

private:
    bool requestPrivateKey();
    void initDriveTree();

    void setupDownloadsTab();
    void setupFsTreeTable();
    void setupDownloadsTable();
    void selectFsTreeItem( int index );
    void onDownloadBtn();

    void updateChannelsCBox();

    void onChannelChanged( int index );
    void onFsTreeHashReceived( const std::string& channelHash, const std::array<uint8_t,32>& fsTreeHash );

    void setDownloadPath( );

    void setupDrivesTab();
    void updateDrivesCBox();

public:
    // if private key is not set it will be 'true'
    bool          m_mustExit = false;

private:
    static MainWin*         m_instanse;

    Ui::MainWin*            ui;

    QTimer*                 m_downloadUpdateTimer;

    FsTreeTableModel*       m_fsTreeTableModel;
    DownloadsTableModel*    m_downloadsTableModel;
};
