#pragma once

#include <QMainWindow>
#include <QFileSystemModel>

#define TEST_UI_INTERFACE

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


private slots:
    void onSettingsBtn();

private:
    bool requestPrivateKey();
    void initDriveTree();
    void setupDownloadsTab();
    void setupFsTreeTable();
    void setupDownloadsTable();

    void setDownloadPath( );

public:
    // if private key is not set it will be 'true'
    bool          m_mustExit = false;

private:
    static MainWin*         m_instanse;

    Ui::MainWin*            ui;

    FsTreeTableModel*       m_fsTreeTableModel;
    DownloadsTableModel*    m_downloadsTableModel;
};
