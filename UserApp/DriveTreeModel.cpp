#include "DriveTreeModel.h"
#include "Settings.h"

#include <QApplication>
#include <QStyle>

void scanFolderR( DriveTreeItem* parent, const fs::path& path )
{
    if ( ! fs::exists(path) || ! fs::is_directory(path ))
    {
        return;
    }
    try
    {
        for( const auto& entry : std::filesystem::directory_iterator(path) )
        {
            const auto entryName = entry.path().filename().string();

            if ( entry.is_directory() )
            {
                QList<QVariant> child;
                child << QApplication::style()->standardIcon(QStyle::SP_DirIcon);
                child << QString::fromStdString( entryName ) << "";
                auto* treeItem = new DriveTreeItem( true, false, ldi_not_changed, child, parent );
                parent->appendChild( treeItem );

                scanFolderR( treeItem, path / entryName );
            }
            else if ( entry.is_regular_file() && entryName != ".DS_Store" )
            {
                QList<QVariant> child;
                child << QApplication::style()->standardIcon(QStyle::SP_FileIcon);
                child << QString::fromStdString( entryName ) << QString::fromStdString( std::to_string( entry.file_size() ) );
                auto* treeItem = new DriveTreeItem( false, false, ldi_not_changed, child, parent );
                parent->appendChild( treeItem );
            }
        }
    }
    catch( const std::runtime_error& err )
    {
        qDebug() << LOG_SOURCE << "scanFolderR: error: " << err.what();
    }
}

DriveTreeModel::DriveTreeModel( QObject* )
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    if ( ! gSettings.config().m_drivesLoaded )
    {
        QList<QVariant> rootList;
        rootList << QString("unused") << QString("name") << QString("size"); // it will be hidden!
        m_rootItem = new DriveTreeItem( false, true, ldi_not_changed, rootList );

        QList<QVariant> driveRootList;
        driveRootList << QString("") << QString("Initializing...") << QString("");
        auto driveRoot = new DriveTreeItem( false, true, ldi_not_changed, driveRootList, m_rootItem );
        m_rootItem->appendChild( driveRoot );
        return;
    }

    QList<QVariant> rootList;
    rootList << QString("unused") << QString("name") << QString("size"); // it will be hidden!
    m_rootItem = new DriveTreeItem( true, false, ldi_not_changed, rootList );

    QList<QVariant> driveRootList;
    driveRootList << QString("/") << QString("/") << QString("");
    auto driveRoot = new DriveTreeItem( true, false, ldi_not_changed, driveRootList, m_rootItem );
    m_rootItem->appendChild( driveRoot );

    const auto* localDrive = Model::currentDriveInfoPtr();

    if ( localDrive != nullptr )
    {
        auto localDriveFolder = localDrive->m_localDriveFolder;
        lock.unlock();
        scanFolderR( driveRoot, localDriveFolder );
    }
}

void parseR( DriveTreeItem* parent, const LocalDriveItem& localFolder )
{
    for( const auto& [name,entry] : localFolder.m_childs )
    {
        if ( entry.m_isFolder )
        {
            QList<QVariant> child;
            child << QApplication::style()->standardIcon(QStyle::SP_DirIcon);
            child << QString::fromStdString( name ) << "";
            auto* treeItem = new DriveTreeItem( true, false, entry.m_ldiStatus, child, parent );
            parent->appendChild( treeItem );

            parseR( treeItem, entry );
        }
        else
        {
            QList<QVariant> child;
            child << QApplication::style()->standardIcon(QStyle::SP_FileIcon);
            child << QString::fromStdString( name ) << QString::fromStdString( std::to_string( entry.m_size ) );;
            auto* treeItem = new DriveTreeItem( false, false, entry.m_ldiStatus, child, parent );
            parent->appendChild( treeItem );
        }
    }
}

void DriveTreeModel::updateModel()
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    QList<QVariant> rootList;
    rootList << QString("unused") << QString("name") << QString("size"); // it will be hidden!
    m_rootItem = new DriveTreeItem( true, false, ldi_not_changed, rootList );

    QList<QVariant> driveRootList;
    driveRootList << QString("/") << QString("/") << QString("");
    auto driveRoot = new DriveTreeItem( true, false, ldi_not_changed, driveRootList, m_rootItem );
    m_rootItem->appendChild( driveRoot );

    const auto* localDrive = Model::currentDriveInfoPtr();

    if ( localDrive != nullptr && localDrive->m_localDrive )
    {
        beginResetModel();
        parseR( driveRoot, *localDrive->m_localDrive );
        endResetModel();
    }
    else if ( localDrive != nullptr )
    {
        scanFolderR( driveRoot, localDrive->m_localDriveFolder );
    }
}
