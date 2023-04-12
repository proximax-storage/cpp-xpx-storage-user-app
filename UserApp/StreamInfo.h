#pragma once
#include <string>
#include <vector>

#include <QObject>
#include <QDateTime>
#include <QDebug>

#include <cereal/types/string.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/portable_binary.hpp>

struct StreamInfo
{
    uint8_t                 m_version;
    std::string             m_driveKey;
    uint64_t                m_streamIndex = -1; // unique index of stream on this drive
    std::string             m_title;
    uint64_t                m_secsSinceEpoch;
    std::string             m_localFolder;

    StreamInfo() {}
    StreamInfo( const std::string&  driveKey,
                const std::string&  title,
                uint64_t            secsSinceEpoch,
                const std::string&  localFolder )
        :
         m_version(1)
        ,m_driveKey(driveKey)
        ,m_title(title)
        ,m_secsSinceEpoch(secsSinceEpoch)
        ,m_localFolder(localFolder)
    {
    }

    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_version,
            m_driveKey,
            m_streamIndex,
            m_title,
            m_secsSinceEpoch,
            m_localFolder
           );
    }

    std::string getLink() const;

    void parseLink( const std::string& );
};

