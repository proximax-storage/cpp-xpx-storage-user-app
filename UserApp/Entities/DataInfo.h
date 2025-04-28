#pragma once
#include <filesystem>

#include <QString>
#include <QSettings>
#include <QDir>
#include <QObject>
#include <QDateTime>
#include <QDebug>

#include <cereal/types/string.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/portable_binary.hpp>

struct DataInfo
{
    uint8_t                 m_version = 0;
    QString                 m_itemName;
    std::array<uint8_t, 32> m_driveKey = {};
    QString                 m_driveName;
    QString                 m_path;
    uint64_t                m_totalSize;

    DataInfo() = default;
    
    DataInfo( const std::array<uint8_t, 32>& driveKey,
              QString                    driveName,
              QString                    itemName,
              const QString&             path,
              uint64_t                   totalSize )
        : m_driveKey(driveKey)
        , m_driveName(driveName)
        , m_itemName(itemName)
        , m_path(path)
        , m_totalSize(totalSize)
    {
    }

    void parseLink( const QString& link);

    QString getLink() const;

    QString savingName() const
    {
        QString name;
        
        if ( m_itemName.isEmpty() )
        {
            name = QString::fromUtf8(std::filesystem::path(m_path.toUtf8().constData()).filename().string());
        }
        else
        {
            name = m_itemName;
        }

        return name;
    }
};

