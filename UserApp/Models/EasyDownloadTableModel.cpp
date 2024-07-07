#include "EasyDownloadTableModel.h"
#include "Utils.h"
#include "mainwin.h"

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QStyle>
#include <QIcon>
#include <QIdentityProxyModel>

EasyDownloadTableModel::EasyDownloadTableModel( Model* model, QObject *parent )
    : QAbstractListModel(parent)
    , mp_model(model)
{
}

int EasyDownloadTableModel::rowCount(const QModelIndex& index) const
{
    return (int)mp_model->easyDownloads().size();
}

int EasyDownloadTableModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant EasyDownloadTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

//    if (index.column() == 0 && role == Qt::CheckStateRole)
//    {
//        return m_checkList.contains(index) ? Qt::Checked : Qt::Unchecked;
//    }

    switch ( role )
    {
        case Qt::TextAlignmentRole:
        {
            if ( index.column() == 3 )
            {
                return Qt::AlignRight;
            }
            break;
        }
        case Qt::DecorationRole:
        {
            if ( index.column() == 0 )
            {
                if ( mp_model->easyDownloads()[index.row()].m_isFolder )
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

                    return QString::fromStdString( mp_model->easyDownloads()[index.row()].m_itemName );
                }
                case 1:
                {
                    const auto& row = mp_model->easyDownloads()[index.row()];
                    if ( row.m_progress == 0 && !row.m_isCompleted )
                    {
                        return "(preparing...)";
                    }
                    else if ( row.m_progress == 1000 || row.m_isCompleted )
                    {
                        std::string sizeInMb = dataSizeToString(row.m_calcTotalSize);
                        return QString::fromStdString( sizeInMb );
                    }
                    std::string sizeInMb = dataSizeToString(row.m_calcTotalSize);
                    return QString::fromStdString( sizeInMb.substr(0, sizeInMb.find('.') + 3)  + " Mb (" +std::to_string( row.m_progress/10) + "%)" );
                }
            }
        }
            break;

        default:
            break;
    }

    return {};
}

QVariant EasyDownloadTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return {};
}

//void EasyDownloadTableModel::addRow( const EasyDownloadInfo& row )
//{
//    beginResetModel();
//    mp_model->easyDownloads().emplace_back(row);
//    endResetModel();
//}

bool EasyDownloadTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
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

Qt::ItemFlags EasyDownloadTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }

    return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

void EasyDownloadTableModel::updateRows()
{
    beginResetModel();

//    if (mp_model->easyDownloads().empty() || (!mp_model->easyDownloads().empty() && mp_model->easyDownloads()[0].m_name != ".." ))
//    {
//        mp_model->easyDownloads().insert(mp_model->easyDownloads().begin(), FsTreeTableModel::Row{ true, "..", "", 0, {}, {} });
//    }
//
//    QModelIndex parentIndex = createIndex(0, 0);
//    if (parentIndex.isValid())
//    {
//        emit dataChanged(parentIndex, parentIndex);
//    }

    endResetModel();
}

//bool EasyDownloadTableModel::isExists(const FsTreeTableModel::Row& row, std::string& name, int& index)
//{
//    for (const auto& r : mp_model->easyDownloads())
//    {
//        bool isExistsLocally;
//        fs::path path;
//        if (row.m_isFolder)
//        {
//            path = fs::path(mp_model->getDownloadFolder().string() + "/" + name).make_preferred();
//            QDir dir(QString::fromStdString(path.string()));
//            isExistsLocally = dir.exists();
//        }
//        else
//        {
//            path = fs::path(mp_model->getDownloadFolder().string() + row.m_path + "/" + name).make_preferred();
//            QFile file(QString::fromStdString(path.string()));
//            isExistsLocally = file.exists();
//        }
//
//        QDir parentDir(QString::fromStdString(path.parent_path().string()));
//        if ( ! parentDir.exists() && !QDir().mkpath(parentDir.path()))
//        {
//            qWarning() << "EasyDownloadTableModel::isExists: Cannot create path recursively!";
//            continue;
//        }
//
//        if (r.m_name == name || isExistsLocally)
//        {
//            index++;
//            name = fs::path(row.m_name).stem().string() + " (" + std::to_string(index) + ")" + fs::path(row.m_name).extension().string();
//            return isExists(row, name, index);
//        }
//    }
//
//    return false;
//}

int EasyDownloadTableModel::onDoubleClick( int row )
{
    return row;
//    int result = 0;
//    if (!mp_model->easyDownloads().empty() && row < mp_model->easyDownloads().size() && !mp_model->easyDownloads()[row].m_isFolder)
//    {
//        return row;
//    }
//
//    if (mp_model->easyDownloads()[row].m_name == ".." && row == 0)
//    {
//        if (m_allRows.empty())
//        {
//            return result;
//        }
//        else
//        {
//            result = m_allRows[m_allRows.size() - 1].first;
//            mp_model->easyDownloads() = m_allRows[m_allRows.size() - 1].second;
//            m_allRows.pop_back();
//        }
//    }
//    else
//    {
//        m_allRows.emplace_back( row, mp_model->easyDownloads() );
//        const auto childs = mp_model->easyDownloads()[row].m_children;
//        mp_model->easyDownloads() = childs;
//    }
//
//    updateRows();
//
//    return result;
}

void EasyDownloadTableModel::updateProgress( QItemSelectionModel* selectionModel )
{
    QList<QModelIndex> selectedIndixes = selectionModel->selectedIndexes();
    
    auto& downloads = mp_model->easyDownloads();
    
    beginResetModel();
    {
        for( auto& info: downloads )
        {
            if ( info.m_isCompleted || info.m_childs.empty() )
            {
                continue;
            }

            uint64_t dnBytes = 0;
            uint64_t totalBytes = 0;
            
            bool isSomeChildsNotCompleted = false;
            for( const auto& dnInfo: info.m_childs )
            {
                bool isCompleted = false;
                if ( dnInfo.m_isCompleted )
                {
                    isCompleted = true;
                    info.m_progress = 1000;
                    continue;
                }
                if ( ! dnInfo.m_ltHandle.is_valid() )
                {
                    if ( info.m_progress == 0 )
                    {
                        isSomeChildsNotCompleted = true;
                    }
                    continue;
                }
                else
                {
                    //                torrent_status status(status_flags_t flags = status_flags_t::all()) const;
                    auto ltStatus = dnInfo.m_ltHandle.status();// lt::status_flags_t::file_progress_flags_t );
                    
                    if ( ltStatus.state  == lt::torrent_status::finished )
                    {
                        isCompleted = true;
                    }
                }
                
                if ( isCompleted )
                {
                    dnBytes     += dnInfo.m_size;
                    totalBytes  += dnInfo.m_size;
                    continue;
                }
                
                isSomeChildsNotCompleted = true;

                std::vector<int64_t> fp = dnInfo.m_ltHandle.file_progress();
                
                //qDebug() << LOG_SOURCE << "fp.size(): " << fp.size();
                for( uint32_t i=0; i<fp.size(); i++ )
                {
                    //qDebug() << LOG_SOURCE << "file_name: " << std::string( dnInfo.m_ltHandle.torrent_file()->files().file_name(i) ).c_str();
                    auto fsize = dnInfo.m_ltHandle.torrent_file()->files().file_size(i);
                    dnBytes    += fp[i];
                    totalBytes += fsize;
                }
                
                double progress = (totalBytes==0) ? 0 : (1000.0 * dnBytes) / double(totalBytes);
                //qDebug() << LOG_SOURCE << "progress: " << progress << ". dnBytes: " << dnBytes << ". totalBytes: " << totalBytes;
                info.m_progress = progress;
            }
            
            if ( ! isSomeChildsNotCompleted )
            {
                info.m_isCompleted = true;
                info.m_progress = 1000;
            }
        }
    }
    endResetModel();
    
    for( auto& index: selectedIndixes )
    {
        selectionModel->select( index, QItemSelectionModel::Select );
    }
}
