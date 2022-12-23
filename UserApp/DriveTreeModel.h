#pragma once

#include <QAbstractItemModel>
#include <QApplication>
#include <QStyle>
#include <QVariant>
#include <QList>

#include <filesystem>

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
        if ( column<0 || column>=m_itemData.size() )
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

public:
    explicit DriveTreeModel( bool isDiffTree, QObject *parent = nullptr );
    ~DriveTreeModel() override { delete m_rootItem; }

    void updateModel( bool skipNotChanged );

    void update( const LocalDriveItem& localDriveRoot );

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
        DriveTreeItem *parentItem;
        if ( parent.column() > 0 )
        {
            return 0;
        }

        if ( !parent.isValid() )
        {
            parentItem = m_rootItem;
        }
        else
        {
            parentItem = static_cast<DriveTreeItem*>(parent.internalPointer());
        }

        return parentItem->childCount();
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


//void DriveTreeModel::setupModelData(const QStringList &lines, DriveTreeItem *parent)
//{
//    QList<DriveTreeItem *> parents;
//    QList<int> indentations;
//    parents << parent;
//    indentations << 0;

//    int number = 0;

//    while (number < lines.count())
//    {
//        int position = 0;
//        while (position < lines[number].length()) {
//            if (lines[number].at(position) != ' ')
//                break;
//            position++;
//        }

//        const QString lineData = lines[number].mid(position).trimmed();

//        if (!lineData.isEmpty()) {
//            // Read the column data from the rest of the line.
//            const QStringList columnStrings =
//                lineData.split(QLatin1Char('\t'), Qt::SkipEmptyParts);
//            QList<QVariant> columnData;
//            columnData.reserve(columnStrings.count());
//            for (const QString &columnString : columnStrings)
//                columnData << columnString;

//            if (position > indentations.last()) {
//                // The last child of the current parent is now the new parent
//                // unless the current parent has no children.

//                if (parents.last()->childCount() > 0) {
//                    parents << parents.last()->child(parents.last()->childCount()-1);
//                    indentations << position;
//                }
//            } else {
//                while (position < indentations.last() && parents.count() > 0) {
//                    parents.pop_back();
//                    indentations.pop_back();
//                }
//            }

//            // Append a new item to the current parent's list of children.
//            parents.last()->appendChild(new DriveTreeItem(columnData, parents.last()));
//        }
//        ++number;
//    }
//}
