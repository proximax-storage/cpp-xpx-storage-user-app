#include "FsTreeTableModel.h"
#include "Settings.h"

#include <QApplication>
#include <QStyle>
#include <QIcon>
#include <QPushButton>
#include <QThread>

FsTreeTableModel::FsTreeTableModel()
{}

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
}

void FsTreeTableModel::update()
{
    beginResetModel();
    endResetModel();
}

void FsTreeTableModel::updateRows()
{
    beginResetModel();

    m_rows.clear();
    m_rows.reserve( m_currentFolder->childs().size()+1 );

    m_rows.emplace_back( Row{true, "..", 0} );

    for( const auto& child : m_currentFolder->childs() )
    {
        if ( sirius::drive::isFolder(child.second) )
        {
//            qDebug() << LOG_SOURCE << "updateRows isFolder: " << sirius::drive::getFolder(child.second).name().c_str();
            m_rows.emplace_back( Row{ true, sirius::drive::getFolder(child.second).name(), 0, {} } );
        }
        else
        {
            const auto& file = sirius::drive::getFile(child.second);
//            qDebug() << LOG_SOURCE << "updateRows isFile: " << sirius::drive::getFile(child.second).name().c_str() << " "
//                     << sirius::drive::toString( file.hash().array() ).c_str();
            m_rows.emplace_back( Row{ false, file.name(), file.size(), file.hash().array() } );
//            qDebug() << LOG_SOURCE << "updateRows isFile: " << sirius::drive::toString( m_rows.back().m_hash ).c_str();
        }
    }

    endResetModel();
}

int FsTreeTableModel::onDoubleClick( int row )
{
    qDebug() << LOG_SOURCE << "onDoubleClick: " << row;

    if ( row == 0 )
    {
        //qDebug() << LOG_SOURCE << "m_currentPath.size(): " << m_currentPath.size();
        if ( m_currentPath.empty() )
        {
            // do not change selected row
            return row;
        }

        auto currentFolder = m_currentFolder;
        m_currentFolder = m_currentPath.back();
        m_currentPath.pop_back();

        updateRows();
        //emit this->layoutChanged();

        int toBeSelectedRow = 0;
        for( const auto& child: m_currentFolder->childs() )
        {
            toBeSelectedRow++;
            if ( sirius::drive::isFolder(child.second) && currentFolder->name() == sirius::drive::getFolder(child.second).name() )
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
        //emit this->layoutChanged();
        return 0;
    }
    return 0;
}

std::string FsTreeTableModel::currentPath() const
{
//    qDebug() << LOG_SOURCE << "currentPath: ";

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
    {
        std::lock_guard<std::recursive_mutex> lock( gSettingsMutex );

        auto channelInfo = gSettings.currentChannelInfoPtr();

        if ( channelInfo == nullptr || channelInfo->m_waitingFsTree || !gSettings.config().m_channelsLoaded )
        {
            //qDebug() << LOG_SOURCE << "rowCount: channelInfo == nullptr || channelInfo->m_waitingFsTree";
            //qDebug() << LOG_SOURCE << "rowCount: 1";
            return 1;
        }
    }
    //qDebug() << "rowCount: " << m_rows.size();
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
            {
                std::lock_guard<std::recursive_mutex> lock( gSettingsMutex );

                auto channelInfo = gSettings.currentChannelInfoPtr();

                if ( channelInfo == nullptr || channelInfo->m_waitingFsTree || !gSettings.config().m_channelsLoaded )
                {
                    return value;
                }
            }
            if ( index.column() == 0 )
            {
                if ( m_rows[index.row()].m_isFolder )
                {
                    return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
                }
                return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
            }
            break;
        }

        case Qt::DisplayRole:
        {
            switch( index.column() )
            {
                case 0:
                {
                    std::lock_guard<std::recursive_mutex> lock( gSettingsMutex );

                    if ( ! gSettings.config().m_channelsLoaded )
                    {
                        return QString("Loading...");
                    }

                    auto channelInfo = gSettings.currentChannelInfoPtr();

                    if ( channelInfo == nullptr )
                    {
                        return QString("No channel selected");
                    }
                    if ( channelInfo->m_waitingFsTree )
                    {
                        return QString("Loading...");
                    }
                    return QString::fromStdString( m_rows[index.row()].m_name );
                }
                case 1:
                {
                    auto channelInfo = gSettings.currentChannelInfoPtr();

                    if ( channelInfo == nullptr || channelInfo->m_waitingFsTree  || !gSettings.config().m_channelsLoaded )
                    {
                        return QString("");
                    }

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
