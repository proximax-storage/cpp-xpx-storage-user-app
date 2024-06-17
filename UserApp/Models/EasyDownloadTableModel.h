#pragma once

#include <QModelIndex>
#include "Models/Model.h"
#include "Entities/EasyDownloadInfo.h"

class QItemSelectionModel;

class EasyDownloadTableModel : public QAbstractListModel
{
public:
    explicit EasyDownloadTableModel( Model* model, QObject* parent = nullptr );

public:
    int onDoubleClick( int row );

    int rowCount(const QModelIndex& index) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void beginResetModel() { QAbstractListModel::beginResetModel(); }
    void endResetModel()   { QAbstractListModel::endResetModel(); }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void updateRows();

    void updateProgress( QItemSelectionModel* );

private:
//    std::string generateUniqueName(const FsTreeTableModel::Row& row);
//    bool isExists(const FsTreeTableModel::Row& row, std::string& name, int& index);
//    void readFolder(const FsTreeTableModel::Row& parentRow, std::vector<FsTreeTableModel::Row>& rows, std::vector<FsTreeTableModel::Row>& result);

private:
    Model* mp_model;
    QSet<QPersistentModelIndex>     m_checkList;
};
