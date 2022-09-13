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
    qDebug() << "m_downloads.size: " << gSettings.config().m_downloads.size();
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
                    return QString::fromStdString( std::to_string( row.m_percents) ) + "%";
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
