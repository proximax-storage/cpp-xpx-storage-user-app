#include "FsTreeTableModel.h"
#include "Settings.h"

#include <QApplication>
#include <QStyle>
#include <QPushButton>
#include <QThread>

FsTreeTableModel::FsTreeTableModel(Model* model, bool isChannelFsModel) 
    : m_isChannelFsModel(isChannelFsModel)
    , mp_model(model)
{}

void FsTreeTableModel::setFsTree( const sirius::drive::FsTree& fsTree, const std::vector<std::string>& path )
{
    m_fsTree = fsTree;
    m_currentFolder = &m_fsTree;
    m_currentPath.clear();
    for( const auto& folder: path )
    {
        m_currentPath.push_back( m_currentFolder );
        auto it = m_currentFolder->findChild( folder );
        if ( it && sirius::drive::isFolder(*it) )
        {
            m_currentFolder = &sirius::drive::getFolder(*it);
        }
    }

    updateRows();
}

void FsTreeTableModel::updateRows()
{
    beginResetModel();

    m_rows.clear();
    m_rows.reserve( m_currentFolder->childs().size() + 1 );

    if ((m_isChannelFsModel && !mp_model->getDownloadChannels().empty()) || !mp_model->getDrives().empty()) {
        m_rows.emplace_back( Row { true, "..", 0 } );
    }

    QModelIndex parentIndex = createIndex(0, 0);
    if (parentIndex.isValid()) {
        emit dataChanged(parentIndex, parentIndex);
    }

    int i = 1;
    for( const auto& child : m_currentFolder->childs() )
    {
        if ( sirius::drive::isFolder(child.second) )
        {
            m_rows.emplace_back( Row{ true, sirius::drive::getFolder(child.second).name(), 0, {} } );
        }
        else
        {
            const auto& file = sirius::drive::getFile(child.second);
            m_rows.emplace_back( Row{ false, file.name(), file.size(), file.hash().array() } );
        }

        QModelIndex childIndex = createIndex(i, 0);
        if (childIndex.isValid()) {
            emit dataChanged(childIndex, childIndex);
        }

        i++;
    }

    endResetModel();
}

int FsTreeTableModel::onDoubleClick( int row )
{
    if ( row == 0 )
    {
        if ( m_currentPath.empty() )
        {
            // do not change selected row
            updateRows();
            return row;
        }

        auto currentFolder = m_currentFolder;
        m_currentFolder = m_currentPath.back();
        m_currentPath.pop_back();

        updateRows();

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
            updateRows();
            return row;
        }

        auto* it = m_currentFolder->findChild( m_rows[row].m_name );
        ASSERT( it != nullptr )
        ASSERT( sirius::drive::isFolder(*it) );
        m_currentPath.emplace_back( m_currentFolder );
        m_currentFolder = &sirius::drive::getFolder(*it);

        updateRows();
        return 0;
    }
}

std::string FsTreeTableModel::currentPathString() const
{
    std::string path;
    if ( m_currentPath.empty() )
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

std::vector<std::string> FsTreeTableModel::currentPath() const
{
    std::vector<std::string> path;
    for( int i=1; i<m_currentPath.size(); i++)
    {
        path.push_back( m_currentPath[i]->name() );
    }
    path.push_back( m_currentFolder->name() );

    return path;
}

int FsTreeTableModel::rowCount(const QModelIndex &) const
{
    return (int)m_rows.size();
}

int FsTreeTableModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant FsTreeTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if ( m_isChannelFsModel )
    {
        return channelData( index, role );
    }

    return driveData( index, role );
}

QVariant FsTreeTableModel::channelData(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

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
                auto channelInfo = mp_model->currentDownloadChannel();
                if ( channelInfo == nullptr || channelInfo->isDownloadingFsTree() || !mp_model->isDownloadChannelsLoaded() )
                {
                    return {};
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
                    if (rowCount(index) == 0) {
                        if ( !mp_model->isDownloadChannelsLoaded() )
                        {
                            return QString("Loading...");
                        }

                        if (mp_model->getDownloadChannels().empty())
                        {
                            return QString("You don't have download channels.");
                        }

                        auto channelInfo = mp_model->currentDownloadChannel();
                        if ( !channelInfo )
                        {
                            return QString("No download channel selected");
                        }
                        if ( channelInfo->isDownloadingFsTree() )
                        {
                            return QString("Loading...");
                        }
                    }

                    return QString::fromStdString( m_rows[index.row()].m_name );
                }
                case 1:
                {
                    auto channelInfo = mp_model->currentDownloadChannel();
                    if ( channelInfo == nullptr || channelInfo->isDownloadingFsTree()  || !mp_model->isDownloadChannelsLoaded() )
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

    return {};
}

QVariant FsTreeTableModel::driveData(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

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
                auto driveInfo = mp_model->currentDrive();

                if ( driveInfo == nullptr || driveInfo->isDownloadingFsTree() || !mp_model->isDrivesLoaded() )
                {
                    return {};
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
                    if (rowCount(index) == 0)
                    {
                        if ( ! mp_model->isDrivesLoaded() )
                        {
                            return QString("Loading...");
                        }

                        if (mp_model->getDrives().empty())
                        {
                            return QString("You don't have drives.");
                        }

                        auto driveInfo = mp_model->currentDrive();
                        if ( !driveInfo )
                        {
                            return QString("No drive selected.");
                        }

                        if ( driveInfo->isDownloadingFsTree() )
                        {
                            return QString("Loading...");
                        }
                    }

                    return QString::fromStdString( m_rows[index.row()].m_name );
                }
                case 1:
                {
                    auto driveInfo = mp_model->currentDrive();

                    if ( driveInfo == nullptr || driveInfo->isDownloadingFsTree() || !mp_model->isDrivesLoaded() )
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

    return {};
}

QVariant FsTreeTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return {};
}
