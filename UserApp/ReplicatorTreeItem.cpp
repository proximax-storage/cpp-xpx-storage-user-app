#include "ReplicatorTreeItem.h"

ReplicatorTreeItem::ReplicatorTreeItem(ItemType type, const QString& key, const QString& alias, ReplicatorTreeItem *parent)
    : m_type(type)
    , m_publicKey(key)
    , m_parentItem(parent)
    , m_alias(alias)
{}

ReplicatorTreeItem::~ReplicatorTreeItem()
{
    qDeleteAll(m_childItems);
}

void ReplicatorTreeItem::appendChild(ReplicatorTreeItem *item)
{
    m_childItems.append(item);
}

ReplicatorTreeItem *ReplicatorTreeItem::child(int row)
{
    if (row < 0 || row >= m_childItems.size())
    {
        return nullptr;
    }

    return m_childItems.at(row);
}

int ReplicatorTreeItem::childCount() const
{
    return (int)m_childItems.count();
}

int ReplicatorTreeItem::columnCount() const
{
    return 2;
}

QString ReplicatorTreeItem::getAlias() const
{
    return m_alias;
}

QString ReplicatorTreeItem::getPublicKey() const
{
    return m_publicKey;
}

ReplicatorTreeItem *ReplicatorTreeItem::parentItem()
{
    return m_parentItem;
}

ReplicatorTreeItem::ItemType ReplicatorTreeItem::getRawType() const
{
    return m_type;
}

QString ReplicatorTreeItem::getType(ItemType type) const
{
    if (type == ItemType::Drive) {
        return "D";
    } else if (type == ItemType::Replicator) {
        return "R";
    } else {
        return "C";
    }
}

int ReplicatorTreeItem::row() const
{
    if (m_parentItem)
    {
        return (int)m_parentItem->m_childItems.indexOf(const_cast<ReplicatorTreeItem*>(this));
    }

    return 0;
}

void ReplicatorTreeItem::clear() {
    m_childItems.clear();
}