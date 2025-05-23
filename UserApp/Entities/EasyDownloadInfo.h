#pragma once

#include <string>
#include <array>
#include "drive/FsTree.h"
#include "DataInfo.h"
#include "Common.h"
#include "Utils.h"
#include <cereal/types/variant.hpp>
#include <QDebug>

struct EasyDownloadChildInfo
{
    //using DownloadNotification = std::function<void( const EasyDownloadInfo& )>;

    EasyDownloadChildInfo( const std::array<uint8_t,32>&    hash,
                           QString                          path,
                           QString                          fileName,
                           size_t                           size )
    : m_hash(hash), m_path(path)
    , m_fileName(fileName)
    , m_size(size) {}
    
    std::array<uint8_t,32>   m_hash;
    QString                  m_path;
    QString                  m_fileName;
    size_t                   m_size;
    QString                  m_downloadFolder; // folder where torrent will be saved before renaming (by copy or move)
    bool                     m_isCompleted = false;
    
    int                      m_progress = 0; // m_progress==1001 means completed

    bool                     m_isStarted = false;
#ifdef WA_APP
    uint64_t                 m_ltHandle;
#else
    sirius::drive::lt_handle m_ltHandle;
#endif
    QString              m_channelKey;

//    std::optional<DownloadNotification> m_notification;
    
    EasyDownloadChildInfo() {}
    ~EasyDownloadChildInfo() {}

public:
    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_hash, m_path, m_fileName, m_size, m_downloadFolder, m_isCompleted );
        //qDebug() << "EasyDownloadChildInfo: " << m_path.c_str() << " size: " << m_size;
    }
};

struct EasyDownloadInfo
{
    template<class Archive>
    void save( Archive &ar ) const
    {
        ar( m_uniqueId, m_totalSize, m_driveKey, m_fsTree, m_itemPath, m_itemName, m_isCompleted, m_calcTotalSize, m_isFolder, m_childs );
    }

    template<class Archive>
    void load( Archive &ar )
    {
        ar( m_uniqueId, m_totalSize, m_driveKey, m_fsTree, m_itemPath, m_itemName, m_isCompleted, m_calcTotalSize, m_isFolder, m_childs );
        qDebug() << "EasyDownloadInfo: " << m_uniqueId << " " << m_totalSize << " " << m_itemName;
    }

    uint64_t                m_uniqueId;
    uint64_t                m_totalSize;    // declared total size (in link)
    sirius::drive::FsTree   m_fsTree;
    std::array<uint8_t,32>  m_driveKey;
    QString                 m_itemName;     // item name
    QString                 m_itemPath;     // path into fsTree
    size_t                  m_calcTotalSize;    // total size
    QString                 m_channelKey;
    bool                    m_isCompleted = false;

    bool                     m_isFolder = false;
    sirius::drive::Folder*   m_downloadingFolder = nullptr;
    sirius::drive::File*     m_downloadingFile = nullptr;

    //bool    m_channelIsOutdated = false;
    int     m_progress = 0; // m_progress==1001 means completed
    
    std::vector<EasyDownloadChildInfo> m_childs;

    EasyDownloadInfo() {}

//    EasyDownloadInfo( uint64_t uniqueId, const std::array<uint8_t,32>& driveKey, QString itemName, QString itemPath, size_t totalSize )
//        : m_uniqueId(uniqueId)
//        , m_driveKey(driveKey)
//        , m_name(itemPath)
//        , m_itemPath(itemPath)
//        , m_size(totalSize)
//    {
//    }
    
    EasyDownloadInfo( uint64_t uniqueId, const DataInfo& dataInfo )
        : m_uniqueId(uniqueId)
        , m_totalSize( dataInfo.m_totalSize)
        , m_driveKey( dataInfo.m_driveKey )
        , m_itemPath( dataInfo.m_path )
        , m_calcTotalSize( dataInfo.m_totalSize )
    {
        m_itemName = dataInfo.savingName();
        if ( m_itemPath.size() > 1 && (m_itemPath[0] == '/' || m_itemPath[0] == '\\') )
        {
            m_itemPath = dataInfo.m_path.mid(1);
        }
    }
    
    void init( const sirius::drive::FsTree& fsTree )
    {
        m_fsTree = fsTree;
        init();
    }
    
    // It will be used after deserialization
    void init()
    {
        m_calcTotalSize = 0;

        if ( m_itemPath == "" || m_itemPath == "/" || m_itemPath == "\\" )
        {
            m_isFolder = true;
            m_downloadingFolder = &m_fsTree;
            addChilds();
        }
        else
        {
            auto* child = m_fsTree.getEntryPtr( m_itemPath.toUtf8().constData() );

            if ( child == nullptr )
            {
                qWarning() << "EasyDownloadInfo: bad path: " << m_itemPath;
                return;
            }
            
            if ( sirius::drive::isFolder(*child) )
            {
                m_isFolder = true;
                m_downloadingFolder = &sirius::drive::getFolder(*child);
                addChilds();
            }
            else
            {
                m_isFolder = false;
                m_downloadingFile = &sirius::drive::getFile(*child);

                m_childs.emplace_back( EasyDownloadChildInfo {  m_downloadingFile->hash().array(),
                                                                "",
                                                                stdStringToQStringUtf8(m_downloadingFile->name()),
                                                                m_downloadingFile->size() } );
                m_calcTotalSize += m_downloadingFile->size();
            }
        }
    }
    
    void addChilds()
    {
        std::filesystem::path path = qStringToStdStringUTF8(m_itemPath);
        m_downloadingFolder->iterate( path, [&,this] ( const std::filesystem::path& filePath, const auto& file) -> bool
        {
            const auto relativePath = stdStringToQStringUtf8(std::filesystem::relative( filePath.string(), path ).string());
            const auto fileName = stdStringToQStringUtf8(file.name());

            m_childs.emplace_back( EasyDownloadChildInfo {  file.hash().array(),
                                                            relativePath,
                                                            fileName,
                                                            file.size() } );
            m_calcTotalSize += file.size();
            return false;
        });
    }
};
