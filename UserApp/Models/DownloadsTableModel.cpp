#include "DownloadsTableModel.h"
#include "mainwin.h"

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QStyle>
#include <QIcon>
#include <QIdentityProxyModel>

DownloadsTableModel::DownloadsTableModel( Model* model, QObject *parent )
    : QAbstractListModel(parent)
    , mp_model(model)
{
}

int DownloadsTableModel::rowCount(const QModelIndex& index) const
{
    return (int)m_rows.size();
}

int DownloadsTableModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant DownloadsTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if (index.column() == 0 && role == Qt::CheckStateRole)
    {
        return m_checkList.contains(index) ? Qt::Checked : Qt::Unchecked;
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
                    if (rowCount(index) == 0)
                    {
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

QVariant DownloadsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return {};
}

void DownloadsTableModel::addRow( const FsTreeTableModel::Row& row )
{
    beginResetModel();

    if (m_rows.empty())
    {
        m_rows.emplace_back( FsTreeTableModel::Row{ true, "..", "", 0, {}, {} } );
    }

    if(m_allRows.empty())
    {
        auto newName = generateUniqueName(row);
        if (newName.empty())
        {
            m_rows.emplace_back(row);
        }
        else
        {
            FsTreeTableModel::Row newRow(row);
            newRow.m_name = newName;
            m_rows.emplace_back(newRow);
        }
    }
    else
    {
        m_allRows[0].second.emplace_back(row);
    }

    endResetModel();
}

bool DownloadsTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.column() == 0 && role == Qt::CheckStateRole)
    {
        if (value == Qt::Checked)
        {
            m_checkList.insert(index);
        } else
        {
            m_checkList.remove(index);
        }

        emit dataChanged(index, index);

        return true;
    }

    emit dataChanged(index, index);

    return true;
}

Qt::ItemFlags DownloadsTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }

    return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

void DownloadsTableModel::updateRows()
{
    beginResetModel();

    if (m_rows.empty() || (!m_rows.empty() && m_rows[0].m_name != ".." ))
    {
        m_rows.insert(m_rows.begin(), FsTreeTableModel::Row{ true, "..", "", 0, {}, {} });
    }

    QModelIndex parentIndex = createIndex(0, 0);
    if (parentIndex.isValid())
    {
        emit dataChanged(parentIndex, parentIndex);
    }

    endResetModel();
}

bool DownloadsTableModel::isExists(const FsTreeTableModel::Row& row, std::string& name, int& index)
{
    for (const auto& r : m_rows)
    {
        bool isExistsLocally;
        fs::path path;
        if (row.m_isFolder)
        {
            path = fs::path(mp_model->getDownloadFolder().string() + "/" + name).make_preferred();
            QDir dir(path.c_str());
            isExistsLocally = dir.exists();
        }
        else
        {
            path = fs::path(mp_model->getDownloadFolder().string() + row.m_path + "/" + name).make_preferred();
            QFile file(path.c_str());
            isExistsLocally = file.exists();
        }

        QDir parentDir(path.parent_path().c_str());
        if ( ! parentDir.exists() && !QDir().mkpath(parentDir.path()))
        {
            qWarning() << "DownloadsTableModel::isExists: Cannot create path recursively!";
            continue;
        }

        if (r.m_name == name || isExistsLocally)
        {
            index++;
            name = fs::path(row.m_name).stem().string() + " (" + std::to_string(index) + ")" + fs::path(row.m_name).extension().string();
            return isExists(row, name, index);
        }
    }

    return false;
}

std::string DownloadsTableModel::generateUniqueName(const FsTreeTableModel::Row& row)
{
    int index = 0;
    auto currentName = row.m_name;
    if (!isExists(row, currentName, index))
    {
        return currentName;
    }

    return {};
}

int DownloadsTableModel::onDoubleClick( int row )
{
    int result = 0;
    if (!m_rows.empty() && row < m_rows.size() && !m_rows[row].m_isFolder)
    {
        return row;
    }

    if (m_rows[row].m_name == ".." && row == 0)
    {
        if (m_allRows.empty())
        {
            return result;
        }
        else
        {
            result = m_allRows[m_allRows.size() - 1].first;
            m_rows = m_allRows[m_allRows.size() - 1].second;
            m_allRows.pop_back();
        }
    }
    else
    {
        m_allRows.emplace_back( row, m_rows );
        const auto childs = m_rows[row].m_chailds;
        m_rows = childs;
    }

    updateRows();

    return result;
}

//void DownloadsTableModel::updateProgress()
//{
//    auto& downloads = mp_model->downloads();
//    beginResetModel();
//    {
//        for( auto& dnInfo: downloads )
//        {
//            if ( dnInfo.isCompleted() || ! dnInfo.getHandle().is_valid() )
//            {
//                continue;
//            }
//
//            std::vector<int64_t> fp = dnInfo.getHandle().file_progress();
//
//            uint64_t dnBytes = 0;
//            uint64_t totalBytes = 0;
//
//            //qDebug() << LOG_SOURCE << "fp.size(): " << fp.size();
//            for( uint32_t i=0; i<fp.size(); i++ )
//            {
//                //qDebug() << LOG_SOURCE << "file_name: " << std::string( dnInfo.m_ltHandle.torrent_file()->files().file_name(i) ).c_str();
//                auto fsize = dnInfo.getHandle().torrent_file()->files().file_size(i);
//                dnBytes    += fp[i];
//                totalBytes += fsize;
//            }
//
//
//            double progress = (totalBytes==0) ? 0 : (1000.0 * dnBytes) / double(totalBytes);
//            //qDebug() << LOG_SOURCE << "progress: " << progress << ". dnBytes: " << dnBytes << ". totalBytes: " << totalBytes;
//            dnInfo.setProgress((int)progress);
//
//            if ( totalBytes>0 && totalBytes==dnBytes )
//            {
//                dnInfo.setCompleted(true);
//            }
//        }
//    }
//    endResetModel();
//}
