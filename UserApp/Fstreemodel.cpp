#include "Fstreemodel.h"

FsTreeModel::FsTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , mIgnoreFiles(false)
    , mEnableMultipleSelection(true)
{
    qRegisterMetaType<Fstreeitem>("Fstreeitem");
    qRegisterMetaType<Fstreeitem*>("Fstreeitem*");
}

FsTreeModel::~FsTreeModel()
{
    delete rootItem;
}

void FsTreeModel::setHeaders(const QStringList &headers)
{
    QVector<QVariant> rootData;
    for (const QString &header : headers)
        rootData << header;

    rootItem = new Fstreeitem(rootData);
}

QVariant FsTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if (index.column() == 0 && role == Qt::CheckStateRole) {
        return mCheckList.contains(index) ? Qt::Checked : Qt::Unchecked;
    }

    if (role == Roles::FsTreeItemRole) {
        return QVariant::fromValue<Fstreeitem*>(getItem(index));
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return {};
    }

    Fstreeitem *item = getItem(index);

    return item->data(index.column());
}

QVariant FsTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return {};
}

QModelIndex FsTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return {};

    Fstreeitem *parentItem = getItem(parent);
    if (!parentItem)
        return {};

    Fstreeitem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return {};
}

QModelIndex FsTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    Fstreeitem *childItem = getItem(index);
    Fstreeitem *parentItem = childItem ? childItem->parent() : nullptr;

    if (parentItem == rootItem || !parentItem)
        return {};

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

int FsTreeModel::rowCount(const QModelIndex &parent) const
{
    const Fstreeitem *parentItem = getItem(parent);

    return parentItem ? parentItem->childCount() : 0;
}

int FsTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return rootItem->columnCount();
}

Qt::ItemFlags FsTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

bool FsTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.column() == 0 && role == Qt::CheckStateRole) {
        if (value == Qt::Checked) {
            auto item = getItem(index);
            if (mEnableMultipleSelection && item && item->getType() == Fstreeitem::ItemType::Folder && item->childCount() > 0) {
                readFolderRecursively(item, true);
            } else {
                mCheckList.insert(index);
            }
        } else {
            auto item = getItem(index);
            if (mEnableMultipleSelection && item && item->getType() == Fstreeitem::ItemType::Folder && item->childCount() > 0) {
                readFolderRecursively(item, false);
            } else {
                mCheckList.remove(index);
            }
        }

        emit dataChanged(index, index);

        return true;
    }

    Fstreeitem *item = getItem(index);
    bool result = item->setData(index.column(), value);

    if (result) {
        emit dataChanged(index, index);
    }

    return result;
}

bool FsTreeModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    const bool result = rootItem->setData(section, value);

    if (result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

bool FsTreeModel::insertColumns(int position, int columns, const QModelIndex &parent)
{
    beginInsertColumns(parent, position, position + columns - 1);
    const bool success = rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool FsTreeModel::removeColumns(int position, int columns, const QModelIndex &parent)
{
    beginRemoveColumns(parent, position, position + columns - 1);
    const bool success = rootItem->removeColumns(position, columns);
    endRemoveColumns();

    if (rootItem->columnCount() == 0)
        removeRows(0, rowCount(parent), parent);

    return success;
}

bool FsTreeModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    Fstreeitem *parentItem = getItem(parent);
    if (!parentItem)
        return false;

    beginInsertRows(parent, position, position + rows - 1);
    const bool success = parentItem->insertChildren(position,
                                                    rows,
                                                    rootItem->columnCount());
    endInsertRows();

    return success;
}

bool FsTreeModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    Fstreeitem *parentItem = getItem(parent);
    if (!parentItem)
        return false;

    beginRemoveRows(parent, position, position + rows - 1);
    const bool success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

void FsTreeModel::updateModelData(const sirius::drive::FsTree& fsTree)
{
    beginResetModel();

    mCheckList.clear();

    mFsTree = fsTree;

    auto newRootItem = new Fstreeitem(rootItem->getItemData());

    delete rootItem;
    rootItem = newRootItem;

    setupModelData(fsTree, rootItem);
    endResetModel();
}

void FsTreeModel::ignoreFiles(bool isIgnoreFiles)
{
    mIgnoreFiles = isIgnoreFiles;
}

void FsTreeModel::enableMultipleSelection(bool enableMultipleSelection)
{
    mEnableMultipleSelection = enableMultipleSelection;
}

sirius::drive::FsTree FsTreeModel::getFsTree()
{
    return mFsTree;
}

QStringList FsTreeModel::getPathsForSelectedItems() const
{
    QStringList paths;
    QSetIterator<QPersistentModelIndex> iterator(mCheckList);
    while (iterator.hasNext()) {
        const QString currentPath = getItem(iterator.next())->getPath();
        paths.append(currentPath);
    }

    return paths;
}

QVariant FsTreeModel::getSelectedItem() const
{
    return mCheckList.empty() ? "" : getSelectedItems()[0];
}

std::vector<QVariant> FsTreeModel::getSelectedItems() const
{
    std::vector<QVariant> items;
    items.reserve(mCheckList.size());
    QSetIterator<QPersistentModelIndex> iterator(mCheckList);
    while (iterator.hasNext()) {
        const QVariant item = QVariant::fromValue(getItem(iterator.next()));
        items.push_back(item);
    }

    return items;
}

QString FsTreeModel::getPathForSelectedItem() const
{
    return mCheckList.empty() ? "" : getPathsForSelectedItems()[0];
}

std::vector<sirius::drive::InfoHash> FsTreeModel::getHashesForSelectedItems() const
{
    std::vector<sirius::drive::InfoHash> hashes;
    hashes.reserve(mCheckList.size());
    QSetIterator<QPersistentModelIndex> iterator(mCheckList);
    while (iterator.hasNext()) {
        hashes.push_back(getItem(iterator.next())->getInfoHash());
    }

    return hashes;
}

sirius::drive::InfoHash FsTreeModel::getHashForSelectedItem() const
{
    return mCheckList.empty() ? sirius::drive::InfoHash() : getHashesForSelectedItems()[0];
}

void FsTreeModel::setupModelData(const sirius::drive::FsTree& fsTree, Fstreeitem *parent)
{
    readFolder(fsTree, parent);
}

void FsTreeModel::readFolderRecursively(Fstreeitem* item, bool isCheckRole) {
    QModelIndex parentIndex = createIndex(item->childNumber(), 0, item);
    if (parentIndex.isValid()) {
        item->setSelected(isCheckRole);
        if (isCheckRole && !mCheckList.contains(parentIndex)) {
            mCheckList.insert(parentIndex);
        } else if (!isCheckRole && mCheckList.contains(parentIndex)) {
            mCheckList.remove(parentIndex);
        }

        emit dataChanged(parentIndex, parentIndex);
    }

    for (int i = 0; i < item->childCount(); i++) {
        auto childItem = item->child(i);
        if (childItem->getType() == Fstreeitem::ItemType::Folder) {
            readFolderRecursively(childItem, isCheckRole);
        } else {
            QModelIndex childIndex = createIndex(childItem->childNumber(), 0, childItem);
            if (childIndex.isValid()) {
                if (isCheckRole && !mCheckList.contains(childIndex)) {
                    mCheckList.insert(childIndex);
                } else if (!isCheckRole && mCheckList.contains(childIndex)) {
                    mCheckList.remove(childIndex);
                }

                emit dataChanged(childIndex, childIndex);
            }
        }
    }
}

void FsTreeModel::readFolder(const sirius::drive::Folder& folder, Fstreeitem *parent)
{
    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
    parent->child(parent->childCount() - 1)->setData(0, folder.name().empty() ? "/" : folder.name().c_str());
    parent->child(parent->childCount() - 1)->setData(1, QVariant::fromValue(folder.childs().size()));
    parent->child(parent->childCount() - 1)->setData(2, "folder");
    parent->child(parent->childCount() - 1)->setData(3, "");
    parent->child(parent->childCount() - 1)->setInfoHash(rootItem->getInfoHash());
    parent->child(parent->childCount() - 1)->setType(Fstreeitem::Folder);

    std::string folderName = parent->child(parent->childCount() - 1)->data(0).toString().toStdString();
    if (folderName != "/") {
        folderName += '/';
    }

    const std::string path = parent->getPath().toStdString() + folderName;
    parent->child(parent->childCount() - 1)->setPath(path.c_str());


    for(const auto& it : folder.childs()) {
        if ( sirius::drive::isFolder(it.second) ) {
            readFolder(sirius::drive::getFolder(it.second), parent->child(parent->childCount() - 1));
        }
        else {
            if (!mIgnoreFiles) {
                Fstreeitem* child = parent->child(parent->childCount() - 1);
                child->insertChildren(child->childCount(), 1, rootItem->columnCount());

                const sirius::drive::File& file = sirius::drive::getFile(it.second);

                // Set metadata
                child->child(child->childCount() - 1)->setData(0, file.name().c_str());
                child->child(child->childCount() - 1)->setData(1, QVariant::fromValue(file.size()));
                child->child(child->childCount() - 1)->setData(2, "file");
                child->child(child->childCount() - 1)->setData(3, "");

                // Set data
                child->child(child->childCount() - 1)->setInfoHash(file.hash());
                child->child(child->childCount() - 1)->setPath(child->getPath() + file.name().c_str());
                child->child(child->childCount() - 1)->setType(Fstreeitem::File);
            }
        }
    }
}

Fstreeitem *FsTreeModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        auto item = static_cast<Fstreeitem*>(index.internalPointer());
        if (item)
        {
            return item;
        }
    }

    return rootItem;
}
