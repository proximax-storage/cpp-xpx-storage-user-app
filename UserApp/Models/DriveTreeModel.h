#pragma once

#include <QAbstractItemModel>
#include <QApplication>
#include <QStyle>
#include <QVariant>
#include <QList>

#include <filesystem>

#include "Models/Model.h"
#include "LocalDriveItem.h"

namespace fs = std::filesystem;

class DriveTreeItem
{
    bool                    m_notFs;
    bool                    m_isFolder;
    int                     m_ldiStatus;
    QList<DriveTreeItem*>   m_childItems;
    QList<QVariant>         m_itemData;
    DriveTreeItem*          m_parentItem = nullptr;

public:
    explicit DriveTreeItem( bool isFolder, bool notFs, int ldiStatus, const QList<QVariant>& data, DriveTreeItem* parentItem = nullptr )
        : m_isFolder(isFolder), m_notFs(notFs), m_ldiStatus(ldiStatus), m_itemData(data), m_parentItem(parentItem)
    {}

    ~DriveTreeItem() {  qDeleteAll(m_childItems); }

    bool isFolder() const { return m_isFolder; }

    bool notFs() const { return m_notFs; }

    int  ldiStatus() const { return m_ldiStatus; }

    DriveTreeItem* child( int row )
    {
        if ( row<0 || row>=m_childItems.size() )
        {
            return nullptr;
        }

        return m_childItems.at(row);
    }

    int childCount() const { return (int)m_childItems.count(); }

    int columnCount() const { return (int)m_itemData.count(); }

    QVariant data( int column ) const
    {
        if ( column < 0 || column >= m_itemData.size() )
        {
            return {};
        }

        return m_itemData.at(column);
    }

    int row() const
    {
        if ( m_parentItem )
        {
            return (int)m_parentItem->m_childItems.indexOf( const_cast<DriveTreeItem*>(this) );
        }

        return 0;
    }

    DriveTreeItem *parentItem() {  return m_parentItem; }

    void appendChild( DriveTreeItem* item )
    {
        m_childItems.append( item );
    }
};

class DriveTreeModel : public QAbstractItemModel
{
    Q_OBJECT

    DriveTreeItem*  m_rootItem;
    bool            m_isDiffTree;
    Model*          mp_model;

public:
    explicit DriveTreeModel( Model* model, bool isDiffTree, QObject *parent = nullptr );
    ~DriveTreeModel() override { delete m_rootItem; }

    void updateModel( bool skipNotChanged );

    QVariant data( const QModelIndex &index, int role ) const override
    {
        if ( !index.isValid() )
        {
            return {};
        }

        if ( role == Qt::DecorationRole )
        {
            if ( index.column() == 1 )
            {
                auto item = static_cast<DriveTreeItem*>(index.internalPointer());
                if ( item->notFs() )
                {
                    return {};
                }
                else if ( item->isFolder() )
                {
                    return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
                }
                else
                {
                    return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
                }
            }
        }

        if ( role == Qt::ForegroundRole )
        {
            if ( index.column() == 1 )
            {
                auto item = static_cast<DriveTreeItem*>(index.internalPointer());
                switch( item->ldiStatus() )
                {
                    case ldi_added:
                        return QVariant( QColor( Qt::darkGreen ) );
                    case ldi_changed:
                        return QVariant( QColor( 0xffa500 ) );
                    case ldi_removed:
                        return QVariant( QColor( Qt::darkRed ) );
                    default:
                        return QVariant( QColor( Qt::black ) );
                }
            }

            return {};
        }

        if ( role != Qt::DisplayRole )
        {
            return {};
        }

        auto item = static_cast<DriveTreeItem*>(index.internalPointer());
        return item->data(index.column());
    }

    Qt::ItemFlags flags( const QModelIndex &index ) const override
    {
        if ( !index.isValid() )
        {
            return Qt::NoItemFlags;
        }

        return QAbstractItemModel::flags(index);
    }

    QVariant headerData( int section, Qt::Orientation orientation, int role ) const override
    {
        if ( orientation == Qt::Horizontal && role == Qt::DisplayRole )
        {
            return m_rootItem->data(section);
        }

        return {};
    }

    QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const override
    {
        if ( !hasIndex(row, column, parent) )
        {
            return {};
        }

        DriveTreeItem *parentItem;

        if ( !parent.isValid() )
        {
            parentItem = m_rootItem;
        }
        else
        {
            parentItem = static_cast<DriveTreeItem*>(parent.internalPointer());
        }


        if ( DriveTreeItem* childItem = parentItem->child(row); childItem )
        {
            return createIndex(row, column, childItem);
        }

        return {};
    }

    QModelIndex parent( const QModelIndex &index ) const override
    {
        if ( !index.isValid() )
        {
            return {};
        }

        auto childItem = static_cast<DriveTreeItem*>(index.internalPointer());
        auto parentItem = childItem ? childItem->parentItem() : nullptr;
        if ( parentItem == m_rootItem || !parentItem )
        {
            return {};
        }

        return createIndex(parentItem->row(), 0, parentItem);
    }

    int rowCount( const QModelIndex &parent = QModelIndex() ) const override
    {
        if (parent.isValid()) {
            auto item = static_cast<DriveTreeItem*>(parent.internalPointer());
            if (item)
            {
                return item->childCount();
            }
        }

        return m_rootItem->childCount();
    }

    int columnCount( const QModelIndex &parent = QModelIndex() ) const override
    {
        if ( parent.isValid() )
        {
            return static_cast<DriveTreeItem*>( parent.internalPointer() )->columnCount();
        }

        return m_rootItem->columnCount();
    }
};