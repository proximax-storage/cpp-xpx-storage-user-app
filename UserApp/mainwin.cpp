#include "mainwin.h"
#include "./ui_mainwin.h"
#include "FsTreeTableModel.h"
#include "DownloadsTableModel.h"

#include "Settings.h"
#include "SettingsDialog.h"
#include "PrivKeyDialog.h"
#include "StorageEngine.h"

#include "crypto/Signer.h"
#include "drive/Utils.h"

#include <QFileIconProvider>
#include <QScreen>
#include <QScroller>

#include <random>

MainWin* MainWin::m_instanse = nullptr;

MainWin::MainWin(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWin)
{
    assert( m_instanse == nullptr );
    m_instanse = this;

    ui->setupUi(this);

    connect( ui->m_settingsBtn, SIGNAL (released()), this, SLOT( onSettingsBtn() ));

    if ( ! gSettings.load("") )
    {
        if ( ! requestPrivateKey() )
        {
            m_mustExit = true;
        }
    }

    if ( ! m_mustExit )
    {
        setupDownloadsTab();

        gStorageEngine = std::make_shared<StorageEngine>();
        gStorageEngine->start();
    }
}

void MainWin::setupDownloadsTab()
{
    setupFsTreeTable();
    setupDownloadsTable();
}

void MainWin::setupFsTreeTable()
{
    connect( ui->m_fsTreeTableView, &QTableView::doubleClicked, this, [this] (const QModelIndex &index)
    {
        //qDebug() << index;
        int toBeSelectedRow = m_fsTreeTableModel->onDoubleClick( index.row() );
        ui->m_fsTreeTableView->selectRow( toBeSelectedRow );
        ui->m_path->setText( "Path: " + QString::fromStdString(m_fsTreeTableModel->currentPath()));
    });

    connect( ui->m_fsTreeTableView, &QTableView::pressed, this, [this] (const QModelIndex &index)
    {
        ui->m_fsTreeTableView->selectRow( index.row() );
    });

    connect( ui->m_fsTreeTableView, &QTableView::clicked, this, [this] (const QModelIndex &index)
    {
        ui->m_fsTreeTableView->selectRow( index.row() );
    });

    m_fsTreeTableModel = new FsTreeTableModel();

    ui->m_fsTreeTableView->setModel( m_fsTreeTableModel );
    ui->m_fsTreeTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_fsTreeTableView->horizontalHeader()->hide();
    ui->m_fsTreeTableView->setGridStyle( Qt::NoPen );

#ifdef TEST_UI_INTERFACE
    sirius::drive::FsTree fsTree;
    fsTree.addFolder( "f1" );
    fsTree.addFolder( "f2" );
    fsTree.addFolder( "f2/f3" );
    fsTree.addFile( "", "file1", sirius::drive::InfoHash(), 1024*1024 );
    fsTree.addFile( "", "file2", sirius::drive::InfoHash(), 1024*1024 );
    fsTree.addFile( "", "file3", sirius::drive::InfoHash(), 1024*1024 );
    fsTree.addFile( "f2", "file21", sirius::drive::InfoHash(), 1024*1024 );
    fsTree.addFile( "f2", "file22", sirius::drive::InfoHash(), 1024*1024 );
    fsTree.addFile( "f2", "file23", sirius::drive::InfoHash(), 1024*1024 );
    fsTree.addFile( "f2/f3", "file21", sirius::drive::InfoHash(), 1024*1024 );
    fsTree.addFile( "f2/f3", "file22", sirius::drive::InfoHash(), 1024*1024 );
    fsTree.addFile( "f2/f3", "file23", sirius::drive::InfoHash(), 1024*1024 );
    m_fsTreeTableModel->setFsTree( fsTree, {} );
#endif

    ui->m_fsTreeTableView->update();
    ui->m_path->setText( "Path: " + QString::fromStdString(m_fsTreeTableModel->currentPath()));
}

void MainWin::setupDownloadsTable()
{
#ifdef TEST_UI_INTERFACE
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f1", "~/Downloads", 100});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f2", "~/Downloads", 50});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f3", "~/Downloads", 25});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f4", "~/Downloads", 0});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f11", "~/Downloads", 100});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f21", "~/Downloads", 50});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f31", "~/Downloads", 25});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f41", "~/Downloads", 0});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f12", "~/Downloads", 100});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f22", "~/Downloads", 50});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f32", "~/Downloads", 25});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f42", "~/Downloads", 0});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f13", "~/Downloads", 100});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f23", "~/Downloads", 50});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f33", "~/Downloads", 25});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f43", "~/Downloads", 0});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f1", "~/Downloads", 100});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f2", "~/Downloads", 50});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f3", "~/Downloads", 25});
    gSettings.config().m_downloads.push_back( Settings::DownloadInfo{ {}, "f4", "~/Downloads", 0});

#endif

    connect( ui->m_downloadsTableView, &QTableView::doubleClicked, this, [this] (const QModelIndex &index)
    {
        qDebug() << index;
        ui->m_downloadsTableView->selectRow( index.row() );
    });

//    connect( ui->m_downloadsTableView, &QTableView::pressed, this, [this] (const QModelIndex &index)
//    {
//        ui->m_downloadsTableView->selectRow( index.row() );
//    });

//    connect( ui->m_downloadsTableView, &QTableView::clicked, this, [this] (const QModelIndex &index)
//    {
//        ui->m_downloadsTableView->selectRow( index.row() );
//    });

    m_downloadsTableModel = new DownloadsTableModel();

    ui->m_downloadsTableView->setModel( m_downloadsTableModel );
    ui->m_downloadsTableView->horizontalHeader()->setStretchLastSection(true);
    ui->m_downloadsTableView->horizontalHeader()->hide();
    ui->m_downloadsTableView->setGridStyle( Qt::NoPen );
    ui->m_downloadsTableView->setSelectionMode( QAbstractItemView::NoSelection );

    ui->m_downloadsTableView->update();
}

MainWin::~MainWin()
{
    delete ui;
}

void MainWin::onSettingsBtn()
{
    SettingsDialog settingsDialog( this );
    settingsDialog.exec();
}

bool MainWin::requestPrivateKey()
{
    {
        PrivKeyDialog pKeyDialog( this, gSettings );

        pKeyDialog.exec();

        if ( pKeyDialog.result() != QDialog::Accepted )
        {
            qDebug() << "not accepted";
            return false;
        }
        qDebug() << "accepted";
    }

    SettingsDialog settingsDialog( this, true );
    settingsDialog.exec();

    if ( settingsDialog.result() != QDialog::Accepted )
    {
        return false;
    }

    return true;
}

void MainWin::initDriveTree()
{
    QFileSystemModel* m_model = new QFileSystemModel();
    m_model->setRootPath("~/");
    ui->m_driveTree->setModel(m_model);
    ui->m_driveTree->header()->resizeSection(0, 320);
    ui->m_driveTree->header()->resizeSection(1, 30);
    ui->m_driveTree->header()->setDefaultAlignment(Qt::AlignLeft);
    ui->m_driveTree->header()->hideSection(2);
    ui->m_driveTree->header()->hideSection(3);

    ui->m_diffView->setColumnWidth(0,330);

//    ui->m_driveFiles->setColumnWidth(0,420);
}

