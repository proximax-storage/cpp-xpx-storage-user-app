#pragma once

#include "LocalDriveItem.h"
#include "drive/ActionList.h"

namespace fs = std::filesystem;

class Diff
{
    sirius::drive::ActionList& m_diffActionList;

public:

    Diff( LocalDriveItem&                   driveRoot,
          fs::path                          localFolderPath,
          const sirius::drive::FsTree&      fsTree,
          const std::array<uint8_t,32>&     driveKey,
          sirius::drive::ActionList&        outDiffActionList )

      : m_diffActionList( outDiffActionList )
    {
        m_diffActionList.clear();
        calcDiff( driveRoot, localFolderPath, fsTree, {}, driveKey );
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

    bool calcDiff( LocalDriveItem&                  localFolder,
                   fs::path                         localFolderPath,
                   const sirius::drive::Folder&     fsTree,
                   fs::path                         drivePath,
                   const std::array<uint8_t,32>&    driveKey );

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

