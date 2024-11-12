#pragma once

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#include "xpxchaincpp/model/storage/replicator.h"
#include "Entities/CachedReplicator.h"

class ReplicatorTreeItem;
class Model;

class ReplicatorTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ReplicatorTreeModel(QObject *parent = nullptr);
    ~ReplicatorTreeModel();

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void setupModelData(const std::vector<xpx_chain_sdk::Replicator>& replicators, Model* model);
    void setupModelData(const std::map<QString, CachedReplicator>& cachedReplicators);
private:
    ReplicatorTreeItem *rootItem;
};
