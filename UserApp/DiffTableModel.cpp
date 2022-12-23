#include "DiffTableModel.h"
#include "Settings.h"

#include <QApplication>
#include <QStyle>
#include <QIcon>
#include <QPushButton>

void DiffTableModel::updateModel()
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    const auto* drive = Model::currentDriveInfoPtr();

    beginResetModel();
    m_actionList.clear();

    if ( drive != nullptr )
    {
        if ( drive->m_localDriveFolderExists )
        {
            m_actionList = drive->m_actionList;
            m_correctLocalFolder = true;
        }
        else
        {
            m_correctLocalFolder = false;
        }
    }

    endResetModel();
}

int DiffTableModel::rowCount(const QModelIndex &) const
{
    if ( m_correctLocalFolder )
    {
        return (int)m_actionList.size();
    }
    return 1;
}

int DiffTableModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant DiffTableModel::data(const QModelIndex &index, int role) const
{
    QVariant value;

    switch ( role )
    {
//        case Qt::TextAlignmentRole:
//        {
//            if ( index.column() == 1 )
//            {
//                return Qt::AlignRight;
//            }
//            break;
//        }
        case Qt::DecorationRole:
        {
            if ( ! m_correctLocalFolder )
            {
                return {};
            }
            if ( index.column() == 0 )
            {
                return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
            }
            break;
        }

        case Qt::ForegroundRole:
        {
            if ( ! m_correctLocalFolder )
            {
                return QVariant( QColor( Qt::red ) );
            }
            if ( index.column() == 0 )
            {
                auto& action = m_actionList[ index.row() ];

                switch( action.m_actionId )
                {
                    case sirius::drive::action_list_id::upload:
                        return QVariant( QColor( Qt::darkGreen ) );
                    case sirius::drive::action_list_id::remove:
                        return QVariant( QColor( Qt::darkRed ) );
                    default:
                        return QVariant( QColor( Qt::black ) );
                }
            }
            return {};//QVariant( QColor( Qt::black ) );
        }


        case Qt::DisplayRole:
        {
            if ( ! m_correctLocalFolder )
            {
                return "Incorrect local folder";
            }
            auto& action = m_actionList[ index.row() ];

            switch( index.column() )
            {
                case 0:
                {
                    switch( action.m_actionId )
                    {
                        case sirius::drive::action_list_id::upload:
//                            qDebug() << "upload diff[" << index.row() << "]: " << QString::fromStdString( action.m_param2 );
                            return QString::fromStdString( action.m_param2 );
                        case sirius::drive::action_list_id::remove:
//                            qDebug() << "remove diff[" << index.row() << "]: " << QString::fromStdString( action.m_param1 );
                            return QString::fromStdString( action.m_param1 );
                        default:
//                            qDebug() << "remove diff[" << index.row() << "]: ???? " << int(action.m_actionId);
                            return QString::fromStdString( "???" );
                    }
                    return QString::fromStdString( "???" );
                }
                case 1:
                {
                    switch( action.m_actionId )
                    {
                        case sirius::drive::action_list_id::upload:
                            return QString::fromStdString( "add" );
                        case sirius::drive::action_list_id::remove:
                            return QString::fromStdString( "del" );
                        default:
                            return QString::fromStdString( "???" );
                    }

                    return QString::fromStdString( "???" );
                }
            }
        }
        break;

        default:
            break;
    }

    return value;
}

QVariant DiffTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}
