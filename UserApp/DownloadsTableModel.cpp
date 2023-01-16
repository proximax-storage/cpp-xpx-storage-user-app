#include "DownloadsTableModel.h"
#include "mainwin.h"

#include <QApplication>
#include <QStyle>
#include <QIcon>
#include <QIdentityProxyModel>

DownloadsTableModel::DownloadsTableModel( Model* model, QObject *parent )
    : QAbstractListModel(parent)
    , mp_model(model)
{
}

int DownloadsTableModel::rowCount(const QModelIndex &) const
{
    return (int)mp_model->downloads().size();
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
            
        case Qt::ForegroundRole:
        {
            if ( index.column() == 0 )
            {
                if ( mp_model->downloads()[index.row()].isChannelOutdated() )
                {
                    return QVariant( QColor( Qt::red ) );
                }
            }
            return {};//QVariant( QColor( Qt::black ) );
        }

        case Qt::DisplayRole:
        {
            switch( index.column() )
            {
                case 0: {
                    if ( mp_model->downloads()[index.row()].isChannelOutdated() )
                    {
                        return QString::fromStdString( "no channel: " + mp_model->downloads()[index.row()].getFileName() );
                    }
                    return QString::fromStdString( mp_model->downloads()[index.row()].getFileName() );
                }
                case 1: {
                    const auto& dnInfo = mp_model->downloads()[index.row()];
                    if ( dnInfo.isCompleted() )
                    {
                        return QString::fromStdString("done");
                    }
                    if ( ! dnInfo.getHandle().is_valid() )
                    {
                        return QString::fromStdString("0%");
                    }

                    return QString::fromStdString( std::to_string( ( dnInfo.getProgress() + 5 ) / 10 ) ) + "%";
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

void DownloadsTableModel::updateProgress()
{
    auto& downloads = mp_model->downloads();
    beginResetModel();
    {
        for( auto& dnInfo: downloads )
        {
            if ( dnInfo.isCompleted() || ! dnInfo.getHandle().is_valid() )
            {
                continue;
            }

            std::vector<int64_t> fp = dnInfo.getHandle().file_progress();

            uint64_t dnBytes = 0;
            uint64_t totalBytes = 0;

            //qDebug() << LOG_SOURCE << "fp.size(): " << fp.size();
            for( uint32_t i=0; i<fp.size(); i++ )
            {
                //qDebug() << LOG_SOURCE << "file_name: " << std::string( dnInfo.m_ltHandle.torrent_file()->files().file_name(i) ).c_str();
                auto fsize = dnInfo.getHandle().torrent_file()->files().file_size(i);
                dnBytes    += fp[i];
                totalBytes += fsize;
            }


            double progress = (totalBytes==0) ? 0 : (1000.0 * dnBytes) / double(totalBytes);
            //qDebug() << LOG_SOURCE << "progress: " << progress << ". dnBytes: " << dnBytes << ". totalBytes: " << totalBytes;
            dnInfo.setProgress((int)progress);

            if ( totalBytes>0 && totalBytes==dnBytes )
            {
                dnInfo.setCompleted(true);
            }
        }
    }
    endResetModel();
}
