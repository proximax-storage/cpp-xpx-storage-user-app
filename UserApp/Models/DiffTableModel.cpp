#include "DiffTableModel.h"

#include <QApplication>
#include <QStyle>
#include <QIcon>
#include <QPushButton>

DiffTableModel::DiffTableModel(Model *model)
    : mp_model(model)
{}

void DiffTableModel::updateModel()
{
    beginResetModel();

    m_actionList.clear();
    auto drive = mp_model->currentDrive();
    if ( !mp_model->getDrives().empty() && drive && drive->isLocalFolderExists() )
    {
        m_actionList = drive->getActionsList();
        m_correctLocalFolder = true;
    }
    else
    {
        m_correctLocalFolder = false;
    }

    endResetModel();
}

int DiffTableModel::rowCount(const QModelIndex &) const
{
    if ( m_correctLocalFolder )
    {
        return (int)m_actionList.size();
    } else if (mp_model->getDrives().empty()) {
        return 0;
    }

    return 1;
}

int DiffTableModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant DiffTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    switch ( role )
    {
        case Qt::DecorationRole:
        {
            if ( ! m_correctLocalFolder || mp_model->getDrives().empty())
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
            if (! m_correctLocalFolder && mp_model->getDrives().empty())
            {
                return {};
            }
            else if ( ! m_correctLocalFolder )
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

            return {};
        }


        case Qt::DisplayRole:
        {
            if (! m_correctLocalFolder && mp_model->getDrives().empty())
            {
                return {};
            }
            else if ( ! m_correctLocalFolder )
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
                            return QString::fromStdString( action.m_param2 );
                        case sirius::drive::action_list_id::remove:
                            return QString::fromStdString( action.m_param1 );
                        default:
                            return "???";
                    }
                }
                case 1:
                {
                    switch( action.m_actionId )
                    {
                        case sirius::drive::action_list_id::upload:
                            return "add";
                        case sirius::drive::action_list_id::remove:
                            return "del";
                        default:
                            return "???";
                    }
                }
            }
        }
        break;

        default:
            break;
    }

    return {};
}

QVariant DiffTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return {};
}
