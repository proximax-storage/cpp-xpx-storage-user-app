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
    std::array<uint8_t, 32> m_driveKey;
    std::string             m_path;

    DataInfo( const std::array<uint8_t, 32>& driveKey,
              const std::string& path )
        :m_driveKey(driveKey)
        ,m_path(path)
    {
    }

    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_driveKey, m_path );
    }

    std::string getLink() const;

    void parseLink( const std::string& link);
};

