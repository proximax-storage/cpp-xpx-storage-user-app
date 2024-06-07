#pragma once
#include <string>

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
    std::string             m_itemName;
    std::array<uint8_t, 32> m_driveKey = {};
    std::string             m_driveName;
    std::string             m_path;
    uint64_t                m_totalSize;

    DataInfo() = default;
    
    DataInfo( const std::string&             itemName,
              const std::array<uint8_t, 32>& driveKey,
              std::string                    driveName,
              const std::string&             path,
              uint64_t                       totalSize )
        : m_itemName(itemName)
        , m_driveKey(driveKey)
        , m_driveName(driveName)
        , m_path(path)
        , m_totalSize(totalSize)
    {
    }

    void parseLink( const std::string& link);

    std::string getLink() const;

};

