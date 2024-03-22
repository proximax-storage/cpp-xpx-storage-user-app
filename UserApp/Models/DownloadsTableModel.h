#pragma once

#include <QModelIndex>
#include "Models/Model.h"
#include "FsTreeTableModel.h"

class DownloadsTableModel : public QAbstractListModel
{
public:
    explicit DownloadsTableModel( Model* model, QObject *parent );

public:
    int onDoubleClick( int row );

    int rowCount(const QModelIndex& index = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void addRow( const FsTreeTableModel::Row& row );

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void updateRows();

    //void updateProgress();

private:
    std::string generateUniqueName(const FsTreeTableModel::Row& row);
    bool isExists(const FsTreeTableModel::Row& row, std::string& name, int& index);
    void readFolder(const FsTreeTableModel::Row& parentRow, std::vector<FsTreeTableModel::Row>& rows, std::vector<FsTreeTableModel::Row>& result);

private:
    Model* mp_model;
    QSet<QPersistentModelIndex> m_checkList;
    std::vector<FsTreeTableModel::Row> m_rows;
    std::vector<std::pair<int, std::vector<FsTreeTableModel::Row>>> m_allRows;
};
