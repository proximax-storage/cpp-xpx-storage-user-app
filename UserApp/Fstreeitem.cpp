#include "Fstreeitem.h"

Fstreeitem::Fstreeitem(Fstreeitem *parent)
    : itemData({})
    , mInfoHash(sirius::drive::InfoHash())
    , mIsSelected(false)
{

}

Fstreeitem::Fstreeitem(const QVector<QVariant> &data, Fstreeitem *parent)
    : itemData(data),
      parentItem(parent),
      mInfoHash(sirius::drive::InfoHash()),
      mIsSelected(false)
{

}

Fstreeitem::~Fstreeitem()
{
    qDeleteAll(childItems);
}

Fstreeitem *Fstreeitem::child(int number)
{
    if (number < 0 || number >= childItems.size())
        return nullptr;
    return childItems.at(number);
}

int Fstreeitem::childCount() const
{
    return childItems.count();
}

int Fstreeitem::columnCount() const
{
    return itemData.count();
}

QVariant Fstreeitem::data(int column) const
{
    if (column < 0 || column >= itemData.size())
        return {};
    return itemData.at(column);
}

bool Fstreeitem::insertChildren(int position, int count, int columns)
{
    if (position < 0 || position > childItems.size())
        return false;

    for (int row = 0; row < count; ++row) {
        QVector<QVariant> data(columns);
        auto *item = new Fstreeitem(data, this);
        childItems.insert(position, item);
    }

    return true;
}

bool Fstreeitem::insertColumns(int position, int columns)
{
    if (position < 0 || position > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.insert(position, QVariant());

    for (Fstreeitem *child : qAsConst(childItems))
        child->insertColumns(position, columns);

    return true;
}

Fstreeitem *Fstreeitem::parent()
{
    return parentItem;
}

bool Fstreeitem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete childItems.takeAt(position);

    return true;
}

bool Fstreeitem::removeColumns(int position, int columns)
{
    if (position < 0 || position + columns > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.remove(position);

    for (Fstreeitem *child : qAsConst(childItems))
        child->removeColumns(position, columns);

    return true;
}

void Fstreeitem::removeChildrens()
{
    qDeleteAll(childItems);
}

int Fstreeitem::childNumber() const
{
    if (parentItem) {
        return parentItem->childItems.indexOf(const_cast<Fstreeitem*>(this));
    }

    return 0;
}

bool Fstreeitem::setData(int column, const QVariant &value)
{
    if (column < 0 || column >= itemData.size())
        return false;

    itemData[column] = value;
    return true;
}

QVector<QVariant> Fstreeitem::getItemData()
{
    return itemData;
}

void Fstreeitem::setPath(const QString &path)
{
    mPath = path;
}

QString Fstreeitem::getPath()
{
    return mPath;
}

void Fstreeitem::setInfoHash(const sirius::drive::InfoHash& hash)
{
    mInfoHash = hash;
}

sirius::drive::InfoHash Fstreeitem::getInfoHash()
{
    return mInfoHash;
}

Fstreeitem::ItemType Fstreeitem::getType() const
{
    return mType;
}

bool Fstreeitem::isParentSelected() {
    return parentItem != nullptr && parentItem->mIsSelected;
}

QString Fstreeitem::getParentSelectedPaths() {
    QString result;
    auto pItem = parentItem;
    while(pItem && pItem->mIsSelected) {
        result.insert(0, "/" + pItem->getItemData().at(0).toString());
        pItem = pItem->parentItem;
    }

    return result;
}

void Fstreeitem::setSelected(bool isSelected) {
    mIsSelected = isSelected;
}

void Fstreeitem::setType(ItemType type)
{
    mType = type;
}
