#include "ReplicatorTreeModel.h"
#include "ReplicatorTreeItem.h"
#include "Models/Model.h"

#include <QStringList>

ReplicatorTreeModel::ReplicatorTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , rootItem(new ReplicatorTreeItem(ReplicatorTreeItem::ItemType::Replicator, "", ""))
{}

ReplicatorTreeModel::~ReplicatorTreeModel()
{
    delete rootItem;
}

int ReplicatorTreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return static_cast<ReplicatorTreeItem*>(parent.internalPointer())->columnCount();
    }

    return rootItem->columnCount();
}

QVariant ReplicatorTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return {};
    }

    if (role != Qt::DisplayRole)
    {
        return {};
    }

    auto item = static_cast<ReplicatorTreeItem*>(index.internalPointer());
    if (!item)
    {
        return {};
    }

    if (item->getRawType() == ReplicatorTreeItem::ItemType::Replicator)
    {
        if (index.column() == 0) {
            return item->getAlias();
        } else if (index.column() == 1) {
            return item->getPublicKey();
        } else {
            return item->getType(item->getRawType());
        }
    }
    else if (item->getRawType() == ReplicatorTreeItem::ItemType::Drive)
    {
        if (index.column() == 0) {
            return item->getAlias();
        } else if (index.column() == 1) {
            return item->getPublicKey();
        } else {
            return item->getType(item->getRawType());
        }
    }
    else if (item->getRawType() == ReplicatorTreeItem::ItemType::DownloadChannel)
    {
        if (index.column() == 0) {
            return item->getAlias();
        } else if (index.column() == 1) {
            return item->getPublicKey();
        } else {
            return item->getType(item->getRawType());
        }
    }
    else
    {
        qWarning () << "ReplicatorTreeModel::data: unknown item type: " << item->getRawType();
        return {};
    }
}

Qt::ItemFlags ReplicatorTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }

    return QAbstractItemModel::flags(index);
}

QVariant ReplicatorTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return section == 0 ? "Name" : "Key" ;
    }

    return {};
}

QModelIndex ReplicatorTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
    {
        return {};
    }

    ReplicatorTreeItem *parentItem;

    if (!parent.isValid())
    {
        parentItem = rootItem;
    }
    else
    {
        parentItem = static_cast<ReplicatorTreeItem*>(parent.internalPointer());
    }

    ReplicatorTreeItem *childItem = parentItem->child(row);
    if (childItem)
    {
        return createIndex(row, column, childItem);
    }

    return {};
}

QModelIndex ReplicatorTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return {};
    }

    auto *childItem = static_cast<ReplicatorTreeItem*>(index.internalPointer());
    auto *parentItem = childItem->parentItem();

    if (parentItem == rootItem || !parentItem)
    {
        return {};
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int ReplicatorTreeModel::rowCount(const QModelIndex &parent) const
{
    ReplicatorTreeItem *parentItem;
    if (parent.column() > 0)
    {
        return 0;
    }

    if (!parent.isValid())
    {
        parentItem = rootItem;
    }
    else
    {
        parentItem = static_cast<ReplicatorTreeItem*>(parent.internalPointer());
    }

    return parentItem->childCount();
}

void ReplicatorTreeModel::setupModelData(const std::vector<xpx_chain_sdk::Replicator>& replicators, Model* model)
{
    beginResetModel();

    for (const auto& replicator : replicators) {
        auto replicatorAlias = replicator.data.key;
        auto r = model->findReplicatorByPublicKey(replicator.data.key);
        if (!r.getName().empty()) {
            replicatorAlias = r.getName();
        }

        auto newReplicator = new ReplicatorTreeItem(ReplicatorTreeItem::ItemType::Replicator, replicator.data.key.c_str(), replicatorAlias.c_str(), rootItem);
        rootItem->appendChild(newReplicator);
    }

    endResetModel();
}

void ReplicatorTreeModel::setupModelData(const std::map<std::string, CachedReplicator>& cachedReplicators)
{
    beginResetModel();

    rootItem->clear();

    for (const auto& replicator : cachedReplicators) {
        auto newReplicator = new ReplicatorTreeItem(ReplicatorTreeItem::ItemType::Replicator, replicator.second.getPublicKey().c_str(), replicator.second.getName().c_str(), rootItem);
        rootItem->appendChild(newReplicator);
    }

    endResetModel();
}