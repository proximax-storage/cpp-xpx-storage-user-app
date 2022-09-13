#include "FsTreeTableModel.h"
#include <QApplication>
#include <QStyle>
#include <QIcon>
#include <QPushButton>

FsTreeTableModel::FsTreeTableModel()
{

}

void FsTreeTableModel::setFsTree( const sirius::drive::FsTree& fsTree, const FsTreePath& path )
{
    m_fsTree = fsTree;
    m_currentPath = path;
    m_currentFolder = &m_fsTree;

    for( auto& folder: path )
    {
        auto* it = m_currentFolder->findChild( folder->name() );
        if ( !sirius::drive::isFolder(*it) )
        {
            break;
        }
        m_currentFolder = &sirius::drive::getFolder(*it);
    }

    updateRows();

    emit this->layoutChanged();
}

void FsTreeTableModel::updateRows()
{
    m_rows.clear();
    m_rows.reserve( m_currentFolder->childs().size()+1 );

    m_rows.emplace_back( Row{true, "..", 0} );

    m_currentFolder->sort();
    for( const auto& child : m_currentFolder->childs() )
    {
        if ( sirius::drive::isFolder(child) )
        {
            m_rows.emplace_back( Row{true, sirius::drive::getFolder(child).name(), 0} );
        }
        else
        {
            const auto& file = sirius::drive::getFile(child);
            m_rows.emplace_back( Row{false, file.name(), file.size()} );
        }
    }
}

int FsTreeTableModel::onDoubleClick( int row )
{
    qDebug() << "onDoubleClick: " << row;

    if ( row == 0 )
    {
        //qDebug() << "m_currentPath.size(): " << m_currentPath.size();
        if ( m_currentPath.empty() )
        {
            // do not change selected row
            return row;
        }

        auto currentFolder = m_currentFolder;
        m_currentFolder = m_currentPath.back();
        m_currentPath.pop_back();

        updateRows();
        emit this->layoutChanged();

        int toBeSelectedRow = 0;
        for( const auto& child: m_currentFolder->childs() )
        {
            toBeSelectedRow++;
            if ( sirius::drive::isFolder(child) && currentFolder->name() == sirius::drive::getFolder(child).name() )
            {
                break;
            }
        }
        return toBeSelectedRow;
    }
    else
    {
        if ( row > m_rows.size() || ! m_rows[row].m_isFolder )
        {
            return row;
        }

        auto* it = m_currentFolder->findChild( m_rows[row].m_name );
        assert( sirius::drive::isFolder(*it) );
        m_currentPath.emplace_back( m_currentFolder );
        m_currentFolder = &sirius::drive::getFolder(*it);

        updateRows();
        emit this->layoutChanged();
        return 0;
    }
    return 0;
}

std::string FsTreeTableModel::currentPath() const
{
//    qDebug() << "currentPath: ";

    std::string path = "";
    if ( m_currentPath.size() == 0 )
    {
        path = "/";
        return path;
    }

    for( int i=1; i<m_currentPath.size(); i++)
    {
        path += "/" + m_currentPath[i]->name();
    }
    path += "/" + m_currentFolder->name();

    return path;
}

int FsTreeTableModel::rowCount(const QModelIndex &) const
{
    return m_rows.size();
}

int FsTreeTableModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant FsTreeTableModel::data(const QModelIndex &index, int role) const
{
    QVariant value;

    switch ( role )
    {
        case Qt::TextAlignmentRole:
        {
            if ( index.column() == 1 )
            {
                return Qt::AlignRight;
            }
            break;
        }
        case Qt::DecorationRole:
        {
            if ( index.column() == 0 )
            {
                if ( m_rows[index.row()].m_isFolder )
                {
                    return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
                }
                return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
            }
//            else if ( index.column() == 1 )
//            {
//                return new QPushButton("download");
//            }
            break;
        }

        case Qt::DisplayRole:
        {
            switch( index.column() )
            {
                case 0: {
                    return QString::fromStdString( m_rows[index.row()].m_name );
                }
                case 1: {
                    const auto& row = m_rows[index.row()];
                    if ( row.m_isFolder )
                    {
                        return QString();
                    }
                    return QString::fromStdString( std::to_string( row.m_size) );
                }
            }
        }
        break;

        default:
            break;
    }

    return value;
}

QVariant FsTreeTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}
