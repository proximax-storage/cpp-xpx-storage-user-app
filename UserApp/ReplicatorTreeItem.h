#pragma once

#include <QVariant>
#include <QList>

class ReplicatorTreeItem
{
public:
    enum ItemType
    {
        Drive,
        DownloadChannel,
        Replicator
    };

public:
    explicit ReplicatorTreeItem(ItemType type, const QString& key, const QString& alias, ReplicatorTreeItem *parentItem = nullptr);
    ~ReplicatorTreeItem();

    void appendChild(ReplicatorTreeItem *child);

    ReplicatorTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QString getAlias() const;
    QString getPublicKey() const;
    int row() const;
    ReplicatorTreeItem *parentItem();
    ItemType getRawType() const;
    QString getType(ItemType type) const;
    void clear();

private:
    QString m_alias;
    QString m_publicKey;
    QList<ReplicatorTreeItem*> m_childItems;
    ReplicatorTreeItem *m_parentItem;
    ItemType m_type;
};
