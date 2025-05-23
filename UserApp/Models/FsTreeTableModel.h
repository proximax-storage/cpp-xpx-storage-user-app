#pragma once

#include <QModelIndex>
#include "Models/Model.h"
#include "drive/FsTree.h"

using FsTreePath = std::vector<sirius::drive::Folder*>;

class FsTreeTableModel: public QAbstractListModel
{
    Q_OBJECT

public:
    FsTreeTableModel( Model* model, bool isChannelFsModel, QObject* parent = nullptr );
    ~FsTreeTableModel() override;

    int onDoubleClick( int row );
    QString currentPathString() const;
    std::vector<QString> currentPath() const;
    
    sirius::drive::Folder* currentSelectedItem( int row,            // index in table-view
                                               QString& outPath,    // path in FsTree
                                               QString& outItemName // name of folder or file
                                               );

    int rowCount(const QModelIndex& index = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setFsTree( const sirius::drive::FsTree& fsTree, const std::vector<QString>& path );

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

public:
        QVariant channelData(const QModelIndex &index, int role) const;
        QVariant driveData(const QModelIndex &index, int role) const;

public:
    struct Row
    {
        Row(bool, const QString&, const QString&, size_t, const std::array<uint8_t,32>&, const std::vector<Row>&);
        Row(const Row& row);
        Row& operator=(const Row&);

        bool        m_isFolder;
        QString     m_name;
        QString     m_path;
        size_t      m_size;
        std::array<uint8_t,32> m_hash;
        std::vector<Row> m_children;
    };

    std::vector<Row> getSelectedRows(bool isSkipFolders = true);

    void updateRows();

    std::vector<Row> m_rows;

private:
    void readFolder(const sirius::drive::Folder& folder, std::vector<Row>& rows);
    void readFolder(const Row& parentRow, std::vector<Row>& rows, std::vector<Row>& result);

private:
    bool                            m_isChannelFsModel;
    sirius::drive::FsTree           m_fsTree = {};
    FsTreePath                      m_currentPath = {};
    sirius::drive::Folder*          m_currentFolder;
    Model*                          mp_model;
    QSet<QPersistentModelIndex>     m_checkList;
};
