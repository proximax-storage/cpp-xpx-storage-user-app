#ifndef FSTREEITEM_H
#define FSTREEITEM_H

#include <QVariant>
#include <QVector>
#include <types.h>

class Fstreeitem
{
public:
    enum ItemType
    {
        Folder,
        File
    };

public:
    Fstreeitem(Fstreeitem *parent = nullptr);
    explicit Fstreeitem(const QVector<QVariant> &data, Fstreeitem *parent = nullptr);
    ~Fstreeitem();

    Fstreeitem *child(int number);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    bool insertChildren(int position, int count, int columns);
    bool insertColumns(int position, int columns);
    Fstreeitem *parent();
    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);
    void removeChildrens();
    int childNumber() const;
    bool setData(int column, const QVariant &value);
    QVector<QVariant> getItemData();
    void setPath(const QString& path);
    QString getPath();
    void setInfoHash(const sirius::drive::InfoHash& hash);
    sirius::drive::InfoHash getInfoHash();
    ItemType getType() const;
    bool isParentSelected();
    QString getParentSelectedPaths();
    void setSelected(bool isSelected);
    void setType(ItemType type);


private:
    QVector<Fstreeitem*> childItems;
    QVector<QVariant> itemData;
    Fstreeitem *parentItem{};
    QString mPath;
    sirius::drive::InfoHash mInfoHash;
    ItemType mType;
    bool mIsSelected;
};

#endif // FSTREEITEM_H
