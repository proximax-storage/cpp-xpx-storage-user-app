#pragma once

#include <QModelIndex>
#include "Models/Model.h"

class DownloadsTableModel : public QAbstractListModel
{
public:
    explicit DownloadsTableModel( Model* model, QObject *parent );

    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void updateProgress();

    void beginResetModel() { QAbstractItemModel::beginResetModel(); }
    void endResetModel() { QAbstractItemModel::endResetModel(); }

private:
    Model* mp_model;
};
