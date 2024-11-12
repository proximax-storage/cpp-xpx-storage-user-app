#pragma once

#include "drive/FsTree.h"
#include "Utils.h"

namespace fs = std::filesystem;

enum { ldi_not_changed, ldi_added, ldi_removed, ldi_changed };

//
// LocalDriveItem
//
struct LocalDriveItem
{
    bool                                    m_isFolder;
    QString                                 m_name;
    uint64_t                                m_size;
    std::array<uint8_t,32>                  m_fileHash;     // file hash
    std::map<QString, LocalDriveItem>       m_childs;
    fs::file_time_type                      m_modifyTime;

    int                                     m_ldiStatus = ldi_not_changed;

    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_isFolder, m_name, m_size, m_fileHash, m_childs, m_modifyTime, m_ldiStatus );
    }

    bool operator<( const LocalDriveItem& rhs ) const
    {
        if ( m_isFolder && !rhs.m_isFolder )
        {
            return true;
        }
        if ( !m_isFolder && rhs.m_isFolder )
        {
            return false;
        }
        return m_name < rhs.m_name;
    }

    bool operator==( const LocalDriveItem& rhs ) const
    {
        return ( m_isFolder == rhs.m_isFolder ) && ( m_name == rhs.m_name );
    }
};
