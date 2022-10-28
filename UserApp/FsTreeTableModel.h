#pragma once

#include <QModelIndex>
#include "drive/FsTree.h"

using FsTreePath = std::vector<sirius::drive::Folder*>;

class FsTreeTableModel: public QAbstractListModel
{
    Q_OBJECT
public:
    FsTreeTableModel();

    void update();

    int onDoubleClick( int row );
    std::string currentPath() const;

    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    public slots:
        void setFsTree( const sirius::drive::FsTree& fsTree, const FsTreePath& path );

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
    sirius::drive::FsTree   m_fsTree = {};
    FsTreePath              m_currentPath = {};
    sirius::drive::Folder*  m_currentFolder;
};
