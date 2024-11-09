#include "FsTreeTableModel.h"
#include "Entities/Settings.h"

#include <QApplication>
#include <QStyle>
#include <QThread>

FsTreeTableModel::FsTreeTableModel(Model* model, bool isChannelFsModel, QObject* parent)
    : QAbstractListModel(parent)
    , m_isChannelFsModel(isChannelFsModel)
    , mp_model(model)
{}

FsTreeTableModel::~FsTreeTableModel()
{}

void FsTreeTableModel::setFsTree( const sirius::drive::FsTree& fsTree, const std::vector<QString>& path )
{
    m_fsTree = fsTree;
    m_currentFolder = &m_fsTree;
    m_currentPath.clear();
    for( const auto& folder: path )
    {
        m_currentPath.push_back( m_currentFolder );
        auto it = m_currentFolder->findChild( folder.toStdString() );
        if ( it && sirius::drive::isFolder(*it) )
        {
            m_currentFolder = &sirius::drive::getFolder(*it);
        }
    }

    updateRows();
}

bool FsTreeTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ( !m_isChannelFsModel) {
        return false;
    }

    if (index.column() == 0 && role == Qt::CheckStateRole) {
        if (value == Qt::Checked) {
            m_checkList.insert(index);
        } else {
            m_checkList.remove(index);
        }

        emit dataChanged(index, index);

        return true;
    }

    emit dataChanged(index, index);

    return true;
}

Qt::ItemFlags FsTreeTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

void FsTreeTableModel::updateRows()
{
    beginResetModel();

    m_rows.clear();
    m_rows.reserve( m_currentFolder->childs().size() + 1 );

    if ((m_isChannelFsModel && !mp_model->getDownloadChannels().empty()) || (!m_isChannelFsModel && !mp_model->getDrives().empty())) {
        m_rows.emplace_back( Row{ true, "..", "", 0, {}, {} } );
    }

    QModelIndex parentIndex = createIndex(0, 0);
    if (parentIndex.isValid()) {
        emit dataChanged(parentIndex, parentIndex);
    }

    readFolder(*m_currentFolder, m_rows);

    endResetModel();
}

sirius::drive::Folder* FsTreeTableModel::currentSelectedItem( int row, QString& outPath, QString& outItemName  )
{
    outItemName = "";
    outPath = "";
    auto pathVector = m_currentPath;
    if ( m_currentFolder != nullptr )
    {
        pathVector.push_back( m_currentFolder );
    }
    for( size_t i=0;  i<pathVector.size(); i++ )
    {
        if ( pathVector[i]->name() != "/" )
        {
            outPath = outPath + "/" + QString::fromUtf8(pathVector[i]->name());
        }
    }
    if ( outPath.isEmpty() )
    {
        outPath = "/";
    }

    // skip '..' and overindex
    if ( row > 0 && row < m_rows.size() )
    {
        outItemName = m_rows[row].m_name;
        if ( outPath == "/" )
        {
            outPath = outPath + outItemName;
        }
        else
        {
            outPath = outPath + "/" + outItemName;
        }
    }

    return m_currentFolder;
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

        auto* it = m_currentFolder->findChild( m_rows[row].m_name.toStdString() );
        ASSERT( it != nullptr )
        ASSERT( sirius::drive::isFolder(*it) );
        m_currentPath.emplace_back( m_currentFolder );
        m_currentFolder = &sirius::drive::getFolder(*it);

        updateRows();
        return 0;
    }
}

QString FsTreeTableModel::currentPathString() const
{
    QString path;
    if ( m_currentPath.empty() )
    {
        path = "/";
        return path;
    }

    for( int i = 1; i < m_currentPath.size(); i++ )
    {
        path += "/" + QString::fromUtf8(m_currentPath[i]->name());
    }

    path += "/" + QString::fromUtf8(m_currentFolder->name());

    return path;
}

std::vector<QString> FsTreeTableModel::currentPath() const
{
    std::vector<QString> path;
    for( int i=1; i<m_currentPath.size(); i++)
    {
        path.push_back( QString::fromUtf8(m_currentPath[i]->name()) );
    }

    path.push_back( QString::fromUtf8(m_currentFolder->name()) );

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
        if (index.column() == 0 && role == Qt::CheckStateRole) {
            return m_checkList.contains(index) ? Qt::Checked : Qt::Unchecked;
        }

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

                    return m_rows[index.row()].m_name;
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
                    return QString::fromStdString( dataSizeToString(row.m_size) );
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

                    return m_rows[index.row()].m_name;
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
                    return QString::fromStdString( dataSizeToString(row.m_size) );
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

void FsTreeTableModel::readFolder(const sirius::drive::Folder& folder, std::vector<Row>& rows)
{
    size_t i = 0;
    for( const auto& child : folder.childs() )
    {
        if ( sirius::drive::isFolder(child.second) )
        {
            rows.emplace_back(Row{ true, QString::fromUtf8(sirius::drive::getFolder(child.second).name()), "", i, {}, {}});
            readFolder(sirius::drive::getFolder(child.second), rows[rows.size() - 1].m_children);
        }
        else
        {
            const auto& file = sirius::drive::getFile(child.second);
            Row newRow( false, QString::fromUtf8(file.name()), "", file.size(), file.hash().array(), {});
            rows.emplace_back(newRow);
        }

        i++;
    }
}

void FsTreeTableModel::readFolder(const Row& parentRow, std::vector<Row>& rows, std::vector<Row>& result)
{
    for (auto& row : rows)
    {
        row.m_path = parentRow.m_path.isEmpty() ? "/" + parentRow.m_name
                                                : parentRow.m_path + "/" + parentRow.m_name;

        if (row.m_isFolder)
        {
            readFolder(row, row.m_children, result);
        }
        else
        {
            result.emplace_back(row);
        }
    }
}

std::vector<FsTreeTableModel::Row> FsTreeTableModel::getSelectedRows(bool isSkipFolders)
{
    std::vector<FsTreeTableModel::Row> rows;
    rows.reserve(m_checkList.size());
    QSetIterator<QPersistentModelIndex> iterator(m_checkList);
    while (iterator.hasNext())
    {
        const QModelIndex index = iterator.next();
        if (index.isValid() && index.row() < m_rows.size())
        {
            auto row = m_rows[index.row()];

            // Return files only (if isSkipFolders = true)
            if (row.m_isFolder && isSkipFolders)
            {
                readFolder(row, row.m_children, rows);
            }
            else
            {
                rows.emplace_back(row);
            }
        }
    }

    return rows;
}

FsTreeTableModel::Row::Row(bool isFolder, const QString& name, const QString& path, size_t size, const std::array<uint8_t, 32>& hash, const std::vector<Row>& chailds)
    : m_isFolder(isFolder)
    , m_name(name)
    , m_path(path)
    , m_size(size)
    , m_hash(hash)
    , m_children(chailds)
{
}

FsTreeTableModel::Row::Row(const FsTreeTableModel::Row &row) {
    m_isFolder = row.m_isFolder;
    m_name = row.m_name;
    m_path = row.m_path;
    m_size = row.m_size;
    m_hash = row.m_hash;
    m_children = row.m_children;
}

FsTreeTableModel::Row &FsTreeTableModel::Row::operator=(const FsTreeTableModel::Row& row) {
    if (this == &row) {
        return *this;
    }

    m_isFolder = row.m_isFolder;
    m_name = row.m_name;
    m_path = row.m_path;
    m_size = row.m_size;
    m_hash = row.m_hash;
    m_children = row.m_children;

    return *this;
}
