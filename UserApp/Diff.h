#pragma once

#include "drive/FsTree.h"
#include "drive/ActionList.h"

namespace fs = std::filesystem;

enum { ldi_not_changed, ldi_added, ldi_removed, ldi_changed };

//
// LocalDriveItem
//
struct LocalDriveItem
{
    bool                                    m_isFolder;
    std::string                             m_name;
    std::array<uint8_t,32>                  m_hash;     // file hash
    std::map<std::string,LocalDriveItem>    m_childs;
    fs::file_time_type                      m_modifyTime;

    int                                     m_ldiStatus = ldi_not_changed;

    template<class Archive>
    void serialize( Archive &ar )
    {
        ar( m_isFolder, m_name, m_childs );
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


class Diff
{
    sirius::drive::ActionList m_diff;

public:

    Diff( LocalDriveItem&               driveInfo,
          const sirius::drive::FsTree&  fsTree,
          fs::path                      localFolderPath,
          std::array<uint8_t,32>&       driveKey )
    {
        calcDiff( driveInfo, fsTree, localFolderPath, "", driveKey );
    }

    static void calcLocalDriveInfoR( LocalDriveItem&            outDriveInfo,
                                     fs::path                   localFolderPath,
                                     bool                       calculateHashes = false,
                                     std::array<uint8_t,32>*    driveKey        = nullptr );

    static void updateLocalDriveInfoR( LocalDriveItem&          outNewDriveInfo,
                                       const LocalDriveItem&    oldDriveInfo,
                                       fs::path                 localFolderPath,
                                       std::array<uint8_t,32>&  driveKey );

private:

    void calcDiff( LocalDriveItem&              localFolder,
                   const sirius::drive::Folder& fsTree,
                   fs::path                     localFolderPath,
                   fs::path                     drivePath,
                   std::array<uint8_t,32>&      driveKey );

    void removeFolderWithChilds( const sirius::drive::Folder&   remoteFolder,
                                 LocalDriveItem&                localFolder,
                                 fs::path                       path );

    void addFolderWithChilds( LocalDriveItem&   localFolder,
                              fs::path          localFolderPath,
                              fs::path          path );

//    void calcDiff( const LocalDriveItem& oldRoot, const LocalDriveItem& newRoot, fs::path fsPath, const std::string& drivePath = "" );
//    void calcMissingFolder( const LocalDriveItem& newRoot, fs::path fsPath, const std::string& drivePath );
//    void calcRemovedFolder( const LocalDriveItem& oldRoot, const std::string& drivePath );

};

