#pragma once

#include <string>
#include <array>
#include "drive/FsTree.h"
#include "DataInfo.h"
#include "Common.h"

#include <QDebug>


struct EasyDownloadChildInfo
{
    //using DownloadNotification = std::function<void( const EasyDownloadInfo& )>;

    EasyDownloadChildInfo( const std::array<uint8_t,32>&  hash,
                           std::string                    fileName,
                           size_t                         size )
    : m_hash(hash), m_fileName(fileName), m_size(size) {}
    
    std::array<uint8_t,32>   m_hash;
    std::string              m_fileName;
    size_t                   m_size;
    std::string              m_saveFolder;
    std::string              m_downloadFolder; // folder where torrent will be saved before renaming (by copy or move)
    bool                     m_isCompleted = false;
    bool                     m_channelIsOutdated = false;
    int                      m_progress = 0; // m_progress==1001 means completed

    sirius::drive::lt_handle m_ltHandle;
    std::string              m_channelKey;

//    std::optional<DownloadNotification> m_notification;
    
    EasyDownloadChildInfo() {}
    ~EasyDownloadChildInfo() {}

public:
    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_hash, m_fileName, m_size, m_saveFolder, m_downloadFolder, m_isCompleted );
    }
};

struct EasyDownloadInfo
{
    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_fsTree, m_itemPath, m_isCompleted );
    }

    sirius::drive::FsTree   m_fsTree;
    std::array<uint8_t,32>  m_driveKey;
    std::string             m_name;     // item name
    std::string             m_itemPath; // path into fsTree
    size_t                  m_size;     // total size
    bool                    m_isCompleted = false;

    
    bool                     m_isFolder = false;
    sirius::drive::Folder*   m_downloadingFolder = nullptr;
    sirius::drive::File*     m_downloadingFile = nullptr;

    bool    m_channelIsOutdated = false;
    int     m_progress = 0; // m_progress==1001 means completed
    
    std::vector<EasyDownloadChildInfo> m_childs;

    EasyDownloadInfo( const std::array<uint8_t,32>& driveKey, std::string itemName, std::string itemPath, size_t totalSize )
        : m_driveKey(driveKey)
        , m_name(itemPath)
        , m_itemPath(itemPath)
        , m_size(totalSize)
        {}
    
    EasyDownloadInfo( const DataInfo& dataInfo )
        : m_driveKey( dataInfo.m_driveKey )
        , m_name( dataInfo.m_itemName )
        , m_itemPath( dataInfo.m_path )
        , m_size( dataInfo.m_totalSize )
        {}
    
    void init( sirius::drive::FsTree fsTree )
    {
        m_fsTree = fsTree;
        init();
    }
    
    void init()
    {
        if ( m_itemPath == "" || m_itemPath == "/" || m_itemPath == "\\" )
        {
            m_isFolder = true;
            m_downloadingFolder = &m_fsTree;
        }
        else
        {
            auto* child = m_fsTree.getEntryPtr( m_itemPath );

            if ( child == nullptr )
            {
                qWarning() << "EasyDownloadInfo: bad path: " << m_itemPath.c_str();
                return;
            }
            
            if ( sirius::drive::isFolder(*child) )
            {
                m_isFolder = true;
                m_downloadingFolder = &sirius::drive::getFolder(*child);
                
                m_downloadingFolder->iterate( [this] (const auto& file) -> bool
                {
                    m_childs.emplace_back( EasyDownloadChildInfo {  file.hash().array(),
                                                                    file.name(),
                                                                    file.size() } );
                    return false;
                });
            }
            else
            {
                m_isFolder = false;
                m_downloadingFile = &sirius::drive::getFile(*child);
                
                m_childs.emplace_back( EasyDownloadChildInfo {  m_downloadingFile->hash().array(),
                                                                m_downloadingFile->name(),
                                                                m_downloadingFile->size() } );
            }
        }
        
        
        
    }

};
