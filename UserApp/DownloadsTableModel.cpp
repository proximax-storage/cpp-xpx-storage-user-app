#include "DownloadsTableModel.h"
#include "Settings.h"
#include "mainwin.h"

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
                    std::lock_guard<std::recursive_mutex> lock(gSettingsMutex);
                    return QString::fromStdString( gSettings.config().m_downloads[index.row()].m_fileName );
                }
                case 1: {
                    std::lock_guard<std::recursive_mutex> lock(gSettingsMutex);

                    const auto& dnInfo = gSettings.config().m_downloads[index.row()];
                    if ( dnInfo.isCompleted() )
                    {
                        //qDebug() << "isCompleted:"
                        return QString::fromStdString("done");
                    }
                    if ( ! dnInfo.m_ltHandle.is_valid() )
                    {
                        //qDebug() << "isCompleted:"
                        return QString::fromStdString("0%");
                    }
                    return QString::fromStdString( std::to_string( (dnInfo.m_progress+5)/10 ) ) + "%";
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

            //qDebug() << "fp.size(): " << fp.size();
            for( uint32_t i=0; i<fp.size(); i++ )
            {
                //qDebug() << "file_name: " << std::string( dnInfo.m_ltHandle.torrent_file()->files().file_name(i) ).c_str();
                auto fsize = dnInfo.m_ltHandle.torrent_file()->files().file_size(i);
                dnBytes    += fp[i];
                totalBytes += fsize;
            }


            double progress = (totalBytes==0) ? 0 : (1000.0 * dnBytes) / double(totalBytes);
            //qDebug() << "progress: " << progress << ". dnBytes: " << dnBytes << ". totalBytes: " << totalBytes;
            dnInfo.m_progress = progress;

            if ( totalBytes>0 && totalBytes==dnBytes )
            {
                dnInfo.m_isCompleted = true;
            }
        }
    }
    endResetModel();

    MainWin::instanse().selectDownloadRow( m_selectedRow );
}
