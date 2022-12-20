#pragma once

#include <QModelIndex>
#include "Model.h"
#include "drive/ActionList.h"

class DiffTableModel: public QAbstractListModel
{
public:
    DiffTableModel(Model* model);

    void updateModel();

    int onDoubleClick( int row );
    std::string currentPath() const;

    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

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
    sirius::drive::ActionList   m_actionList = {};
    bool                        m_correctLocalFolder = true;
    Model*                      mp_model;
};
