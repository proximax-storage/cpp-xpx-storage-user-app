#pragma once

#include <QModelIndex>

class DownloadsTableModel : public QAbstractListModel
{
public:
    int m_selectedRow = -1;

public:
    explicit DownloadsTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void updateProgress();

    void beginResetModel() { QAbstractItemModel::beginResetModel(); }
    void endResetModel() { QAbstractItemModel::endResetModel(); }

};
