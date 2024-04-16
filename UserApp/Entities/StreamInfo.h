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
    QString m_recordingPath;
    QString m_obsPath;
    QString m_outputMode;
    
    ObsProfileData()
    {
#if defined _WIN32
#elif defined __APPLE__
        QString homePath = QDir::homePath();
        m_obsPath = homePath + "/Library/Application Support/obs-studio";
        QSettings my_settings( homePath + "/Library/Application Support/obs-studio/basic/profiles/Siriusstream/basic.ini", QSettings::IniFormat);
        m_recordingPath = my_settings.value("SimpleOutput/FilePath", "").toString().trimmed();
        m_outputMode = "Simple"
#else // LINUX
        QString homePath = QDir::homePath();
        m_obsPath = homePath + "/.config/obs-studio";
        QSettings my_settings( homePath + "/.config/obs-studio/basic/profiles/Siriusstream/basic.ini", QSettings::IniFormat);
        m_recordingPath = my_settings.value("AdvOut/RecFilePath", "").toString().trimmed();
        m_outputMode = "Advanced";
#endif

    }
    
    bool isObsInstalled()
    {
        QDir directory(m_obsPath);

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
        QString dirName = m_obsPath + "/basic/profiles/Siriusstream";
        QDir directory(dirName);

        if( !directory.exists() )
        {
            qDebug() << "The 'Sirius-stream' profile does not exist in OBS.";
            return false;
        }

        qDebug() << "The 'Sirius-stream' profile exists in OBS.";
        return true;
    }

    bool isOutputModeCorrect()
    {
        QSettings my_settings( m_obsPath + "/basic/profiles/Siriusstream/basic.ini", QSettings::IniFormat );
        QString currentMode = my_settings.value("Output/Mode", "").toString().trimmed();

        if( !(currentMode == m_outputMode) )
        {
            qDebug() << "Incorrect output mode.";
            return false;
        }

        qDebug() << "Output mode set correctly.";
        return true;
    }

    bool isRecordingPathSet()
    {
        return ! m_recordingPath.isEmpty();
    }

    bool isSiriusProfileSet()
    {
        QSettings globalSettings( m_obsPath + "/global.ini", QSettings::IniFormat);
        QString profile = globalSettings.value("Basic/Profile", "").toString().trimmed();

        if( profile.isEmpty() || !(profile == "Sirius-stream") )
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
    enum StreamingStatus { ss_registering, ss_created, ss_deleting, ss_running, ss_finished };
    
    uint8_t                 m_version = 1;
    std::string             m_driveKey;
    std::string             m_title;
    std::string             m_annotation;
    uint64_t                m_secsSinceEpoch = 0;   // start time
    std::string             m_uniqueFolderName;
    int                     m_streamingStatus = ss_registering;
    
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

