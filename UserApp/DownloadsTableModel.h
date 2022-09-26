#pragma once

#include <QModelIndex>

class DownloadsTableModel : public QAbstractListModel
{
public:
    explicit DownloadsTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void updateProgress();
    void onDownloadCompleted( const std::array<uint8_t,32>& hash );
};
