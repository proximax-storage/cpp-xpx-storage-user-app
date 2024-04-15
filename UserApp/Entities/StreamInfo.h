#pragma once
#include <string>
#include <vector>

#include <QSettings>
#include <QDir>
#include <QObject>
#include <QDateTime>
#include <QDebug>

#include <cereal/types/string.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/portable_binary.hpp>

struct ObsProfileData
{
    std::string m_recordingPath;
    std::string m_obsPath;
    
    ObsProfileData()
    {
#if defined _WIN32
#elif defined __APPLE__
        QString homePath = QDir::homePath();
        m_obsPath = homePath.toStdString() + "/Library/Application Support/obs-studio";
        QSettings my_settings( homePath + "/Library/Application Support/obs-studio/basic/profiles/Siriusstream/basic.ini", QSettings::IniFormat);
        m_recordingPath = my_settings.value("SimpleOutput/FilePath", "").toString().trimmed().toStdString();
#else // LINUX
        QString homePath = QDir::homePath();
        m_obsPath = homePath.toStdString() + "/.config/obs-studio";
        QSettings my_settings( homePath + "/.config/obs-studio/basic/profiles/Siriusstream/basic.ini", QSettings::IniFormat);
        m_recordingPath = my_settings.value("AdvOut/RecFilePath", "").toString().trimmed().toStdString();
#endif

    }
    
    bool isObsInstalled()
    {
        QDir directory(QString::fromStdString(m_obsPath));

        if( !directory.exists() )
        {
            qDebug() << "OBS is not installed.";
            return false;
        }

        qDebug() << "OBS is installed.";
        return true;
    }

    bool isObsProfileAvailable()
    {
        std::string dirName = m_obsPath + "/basic/profiles/Siriusstream";
        QDir directory(QString::fromStdString(dirName));

        if( !directory.exists() )
        {
            qDebug() << "The 'Sirius-stream' profile does not exist in OBS.";
            return false;
        }

        qDebug() << "The 'Sirius-stream' profile exists in OBS.";
        return true;
    }

    bool isRecordingPathSet()
    {
        return ! m_recordingPath.empty();
    }

    bool isSiriusProfileSet()
    {
        QSettings globalSettings( QString::fromStdString(m_obsPath) + "/global.ini", QSettings::IniFormat);
        std::string profile = globalSettings.value("Basic/Profile", "").toString().trimmed().toStdString();

        if( profile.empty() || !(profile == "Sirius-stream") )
        {
            qDebug() << "The 'Sirius-stream' profile is not set as main profile.";
            return false;
        }
        qDebug() << "The 'Sirius-stream' profile is set as main profile.";
        return true;
    }

};

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

