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
    enum StreamingStatus { ss_regestring, ss_created, ss_deleting, ss_running, ss_finished };
    
    uint8_t                 m_version = 1;
    std::string             m_driveKey;
    std::string             m_title;
    std::string             m_annotation;
    uint64_t                m_secsSinceEpoch = 0;   // start time
    std::string             m_uniqueFolderName;
    int                     m_streamingStatus = ss_regestring;
    
    uint64_t                m_totalStreamSizeBytes = 0;
    uint64_t                m_totalStreamStreamDurationSeconds = 0;

    StreamInfo() : m_streamingStatus(ss_deleting) {}
    StreamInfo( const std::string&  driveKey,
                const std::string&  title,
                const std::string&  annotation,
                uint64_t            secsSinceEpoch,
                const std::string&  streamFolder )
        :
        m_driveKey(driveKey)
        ,m_title(title)
        ,m_annotation(annotation)
        ,m_secsSinceEpoch(secsSinceEpoch)
        ,m_uniqueFolderName(streamFolder)
    {
    }

    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_version,
            m_driveKey,
            m_title,
            m_annotation,
            m_secsSinceEpoch,
            m_uniqueFolderName
           );
        ar( m_totalStreamSizeBytes );
        ar( m_totalStreamStreamDurationSeconds );
    }

    std::string getLink() const;

    void parseLink( const std::string& );
};

