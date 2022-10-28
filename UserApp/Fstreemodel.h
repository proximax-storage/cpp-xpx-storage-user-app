#ifndef FSTREEMODEL_H
#define FSTREEMODEL_H

#include <QSet>
#include <QPersistentModelIndex>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QDebug>
#include <vector>
#include "Fstreeitem.h"
#include "drive/Utils.h"
#include "drive/FsTree.h"

Q_DECLARE_METATYPE(Fstreeitem)
Q_DECLARE_METATYPE(Fstreeitem*)

class FsTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        FsTreeItemRole = Qt::UserRole + 1,
        FoldersOnlyRole = Qt::UserRole + 2
    };

public:
    explicit FsTreeModel(QObject *parent = nullptr);

    ~FsTreeModel() override;

    void setHeaders(const QStringList &headers);

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;

    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent) const override;

    int columnCount(const QModelIndex &parent) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role) override;

    bool insertColumns(int position, int columns, const QModelIndex &parent) override;

    bool removeColumns(int position, int columns, const QModelIndex &parent) override;

    bool insertRows(int position, int rows, const QModelIndex &parent) override;

    bool removeRows(int position, int rows, const QModelIndex &parent) override;

    void updateModelData(const sirius::drive::FsTree& fsTree);

    void ignoreFiles(bool isIgnoreFiles = false);

    void enableMultipleSelection(bool enableMultipleSelection);

    sirius::drive::FsTree getFsTree();

    QStringList getPathsForSelectedItems() const;

    QVariant getSelectedItem() const;

    std::vector<QVariant> getSelectedItems() const;

    QString getPathForSelectedItem() const;

    std::vector<sirius::drive::InfoHash> getHashesForSelectedItems() const;

    sirius::drive::InfoHash getHashForSelectedItem() const;

private:
    void readFolderRecursively(Fstreeitem* item, bool isCheckRole);

    void setupModelData(const sirius::drive::FsTree& fsTree, Fstreeitem *parent);

    void readFolder(const sirius::drive::Folder& folder, Fstreeitem *parent);

    Fstreeitem *getItem(const QModelIndex &index) const;

    Fstreeitem *rootItem;
    QSet<QPersistentModelIndex> mCheckList;
    sirius::drive::FsTree mFsTree;

    bool mIgnoreFiles;
    bool mEnableMultipleSelection;
};

#endif // FSTREEMODEL_H
