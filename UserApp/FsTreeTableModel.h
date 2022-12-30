#pragma once

#include <QModelIndex>
#include "Model.h"
#include "drive/FsTree.h"

using FsTreePath = std::vector<sirius::drive::Folder*>;

class FsTreeTableModel: public QAbstractListModel
{
    Q_OBJECT

public:
    FsTreeTableModel( Model* model, bool isChannelFsModel );

    int onDoubleClick( int row );
    std::string currentPathString() const;
    std::vector<std::string> currentPath() const;

    int rowCount(const QModelIndex& index = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setFsTree( const sirius::drive::FsTree& fsTree, const std::vector<std::string>& path );

public:
        QVariant channelData(const QModelIndex &index, int role) const;
        QVariant driveData(const QModelIndex &index, int role) const;

public:
    struct Row
    {
        bool        m_isFolder;
        std::string m_name;
        size_t      m_size;
        std::array<uint8_t,32> m_hash;
    };

    void updateRows();

    std::vector<Row> m_rows;

private:
    bool                    m_isChannelFsModel;
    sirius::drive::FsTree   m_fsTree = {};
    FsTreePath              m_currentPath = {};
    sirius::drive::Folder*  m_currentFolder;
    Model*                  mp_model;
};
