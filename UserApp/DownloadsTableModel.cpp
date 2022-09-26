#include "DownloadsTableModel.h"
#include "Settings.h"

#include <QApplication>
#include <QStyle>
#include <QIcon>
#include <QIdentityProxyModel>

DownloadsTableModel::DownloadsTableModel(QObject *parent)
    : QAbstractListModel{parent}
{

}

int DownloadsTableModel::rowCount(const QModelIndex &) const
{
    //qDebug() << "m_downloads.size: " << gSettings.config().m_downloads.size();
    return gSettings.config().m_downloads.size();
}

int DownloadsTableModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant DownloadsTableModel::data(const QModelIndex &index, int role) const
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
                return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
            }
            break;
        }

        case Qt::DisplayRole:
        {
            switch( index.column() )
            {
                case 0: {
                    return QString::fromStdString( gSettings.config().m_downloads[index.row()].m_fileName );
                }
                case 1: {
                    const auto& row = gSettings.config().m_downloads[index.row()];
                    //return QString::fromStdString( std::to_string( row.m_percents) ) + "%";
                    return QString::fromStdString("%");
                }
            }
        }
        break;

        default:
            break;
    }

    return value;
}

QVariant DownloadsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

void DownloadsTableModel::updateProgress()
{
    std::lock_guard<std::recursive_mutex> lock(gSettingsMutex);

    auto& downloads = gSettings.config().m_downloads;

    beginResetModel();
    {
        for( auto& dnInfo: downloads )
        {
            if ( dnInfo.isCompleted() || ! dnInfo.m_ltHandle.is_valid() )
            {
                continue;
            }

            std::vector<int64_t> fp = dnInfo.m_ltHandle.file_progress();

            uint64_t dnBytes = 0;
            uint64_t totalBytes = 0;

            for( uint32_t i=0; i<fp.size(); i++ )
            {
                auto fsize = dnInfo.m_ltHandle.torrent_file()->files().file_size(i);
                dnBytes    += fp[i];
                totalBytes += fsize;
            }

            double progress = (1000.0 * totalBytes) / double(dnBytes);
            dnInfo.m_progress = progress;
        }
    }
    endResetModel();
}

void DownloadsTableModel::onDownloadCompleted( const std::array<uint8_t,32>& hash )
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto& downloads = gSettings.config().m_downloads;

    beginResetModel();
    {
        for( auto& dnInfo: downloads )
        {
            if (  dnInfo.m_hash == hash )
            {
                dnInfo.m_progress = 1001;
            }
        }
    }
    endResetModel();
}
