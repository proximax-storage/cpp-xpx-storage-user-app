#include "DriveTreeModel.h"

#include <QApplication>
#include <QStyle>

void scanFolderR( DriveTreeItem* parent, const fs::path& path )
{
    for( const auto& entry : std::filesystem::directory_iterator(path) )
    {
        const auto entryName = entry.path().filename().string();

        if ( entry.is_directory() )
        {
            QList<QVariant> child;
            child << QApplication::style()->standardIcon(QStyle::SP_DirIcon);
            child << QString::fromStdString( entryName ) << "";
            auto* treeItem = new DriveTreeItem( child, parent );
            parent->appendChild( treeItem );

            scanFolderR( treeItem, path / entryName );
        }
        else if ( entry.is_regular_file() && entryName != ".DS_Store" )
        {
            QList<QVariant> child;
            child << QApplication::style()->standardIcon(QStyle::SP_FileIcon);
            child << QString::fromStdString( entryName ) << "124";
            auto* treeItem = new DriveTreeItem( child, parent );
            parent->appendChild( treeItem );
        }
    }
}

DriveTreeModel::DriveTreeModel( QObject *parent )
{
    QList<QVariant> root;
    root << QString("icon") << QString("name") << QString("size"); // it will be hidden!
    m_rootItem = new DriveTreeItem( root );

//    QList<QVariant> child1;
//    child1 << QString("child1");
//    m_rootItem->appendChild( new DriveTreeItem( child1, m_rootItem ) );

//    QList<QVariant> child2;
//    child2 << QString("child2");
//    m_rootItem->appendChild( new DriveTreeItem( child2, m_rootItem ) );

    scanFolderR( m_rootItem, "/Users/alex/000-LocalFolder" );
}

void DriveTreeModel::readFolder( const fs::path& folder )
{
    delete m_rootItem;

    QList<QVariant> root;
    root << QString("name") << QString("size"); // it will be hidden!
    m_rootItem = new DriveTreeItem( root );



}
