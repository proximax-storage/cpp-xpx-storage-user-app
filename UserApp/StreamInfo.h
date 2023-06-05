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
    enum StreamingStatus { ss_creating, ss_created, ss_registring, ss_started, ss_finished };
    
    uint8_t                 m_version = 1;
    std::string             m_driveKey;
    uint64_t                m_streamIndex = -1; // unique index of stream on this drive
    std::string             m_title;
    std::string             m_annotation;
    uint64_t                m_secsSinceEpoch;   // start time
    std::string             m_streamFolder;
    std::string             m_streamTx;         // streamId for started stream
    int                     m_streamingStatus = ss_creating;

    StreamInfo() {}
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
        ,m_streamFolder(streamFolder)
    {
    }

    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_version,
            m_driveKey,
            m_streamIndex,
            m_title,
            m_annotation,
            m_secsSinceEpoch,
            m_streamFolder,
            m_streamTx,
            m_streamingStatus
           );
    }

    std::string getLink() const;

    void parseLink( const std::string& );
};

