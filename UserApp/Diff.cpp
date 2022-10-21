#include "Diff.h"
#include "drive/Session.h"
#include "drive/Utils.h"

#include <QDebug>

void Diff::calcLocalDriveInfoR( LocalDriveItem& parent, fs::path path, bool calculateHashes, std::array<uint8_t,32>* driveKey )
{
    for( const auto& entry : std::filesystem::directory_iterator(path) )
    {
        const auto entryName = entry.path().filename().string();

        if ( entry.is_directory() )
        {
            LocalDriveItem child{ true, entryName, "", {}, fs::last_write_time(entry) };
            calcLocalDriveInfoR( child, path / entryName );
            parent.m_childs[entryName] = std::move(child);
        }
        else if ( entry.is_regular_file() && entryName != ".DS_Store" )
        {
            if ( calculateHashes )
            {
                auto& hash = sirius::drive::calculateInfoHash( path / entryName, sirius::Key(*driveKey) ).array();
                parent.m_childs[entryName] = LocalDriveItem{ false, entryName, hash, {}, fs::last_write_time(entry) };
            }
            else
            {
                parent.m_childs[entryName] = LocalDriveItem{ false, entryName, {}, {}, fs::last_write_time(entry) };
            }
        }
    }
}

void Diff::updateLocalDriveInfoR( LocalDriveItem&           newRoot,
                                  const LocalDriveItem&     oldRoot,
                                  fs::path                  fsPath,
                                  std::array<uint8_t,32>&   driveKey )
{
    for( const auto& entry : std::filesystem::directory_iterator(fsPath) )
    {
        const auto entryName = entry.path().filename().string();

        if ( entry.is_directory() )
        {
            LocalDriveItem child{ true, entryName, "", {}, fs::last_write_time(entry) };

            auto oldIt = oldRoot.m_childs.find( entryName );
            if ( oldIt != oldRoot.m_childs.end() && oldIt->second.m_isFolder )
            {
                updateLocalDriveInfoR( child, oldIt->second, fsPath / entryName, driveKey );
            }
            else
            {
                updateLocalDriveInfoR( child, {}, fsPath / entryName, driveKey );
            }
            newRoot.m_childs[entryName] = std::move(child);
        }
        else if ( entry.is_regular_file() && entryName != ".DS_Store" )
        {
            auto oldIt = oldRoot.m_childs.find( entryName );
            if ( oldIt != oldRoot.m_childs.end() && ! oldIt->second.m_isFolder )
            {
                auto modifyTime = fs::last_write_time( fsPath/entryName );
                if ( modifyTime == oldIt->second.m_modifyTime )
                {
                    newRoot.m_childs[entryName] = LocalDriveItem{ false, entryName, oldIt->second.m_hash, {}, modifyTime };
                    continue;
                }

            }
            auto& hash = sirius::drive::calculateInfoHash( fsPath / entryName, sirius::Key(driveKey) ).array();
            newRoot.m_childs[entryName] = LocalDriveItem{ false, entryName, hash, {}, fs::last_write_time(entry) };
        }
    }
}

void Diff::calcDiff( LocalDriveItem&                localFolder,
                     const sirius::drive::Folder&   fsFolder,
                     fs::path                       localFolderPath,
                     fs::path                       drivePath,
                     std::array<uint8_t,32>&        driveKey )
{
    auto& localChilds = localFolder.m_childs;

    // 'removedItems' are files (or folders) that
    // that will be added in localFolder as 'ldi_removed'
    //
    std::list<LocalDriveItem> removedItems;

    // Find files that were added or modifyed by user
    //
    for( auto& [name,localItem] : localChilds )
    {
        // Logic path of file/folder on the drive
        fs::path itemPath = drivePath.empty() ? fs::path(localItem.m_name) : drivePath / localItem.m_name;

        // check that file (or folder) exists on the drive
        //
        if ( auto fsIt = fsFolder.childs().find(name); fsIt != fsFolder.childs().end() )
        {
            // Item exists in local folder and in remote drive
            //
            if ( localItem.m_isFolder )
            {
                if ( isFolder(fsIt->second) )
                {
                    // continue calculation
                    calcDiff( localItem, getFolder(fsIt->second), localFolderPath / name, itemPath, driveKey );
                }
                else
                {
                    // Remote file has been replaced by folder with same name

                    // save removed file into 'replacedItems'
                    removedItems.push_back( LocalDriveItem{ false,name,{},{},{},ldi_removed} );

                    // remove remote file
                    m_diffActionList.push_back( sirius::drive::Action::remove( itemPath ) );

                    // add local folder
                    addFolderWithChilds( localItem, itemPath, drivePath / name );
                    localItem.m_ldiStatus = ldi_added;
                }
            }
            else
            {
                if ( isFolder(fsIt->second) )
                {
                    // Remote folder has been replaced by file with same name

                    // calculate removed folder (from drive)
                    LocalDriveItem removedFolder{ false,name,{},{},{},ldi_removed};
                    removeFolderWithChilds( getFolder(fsIt->second), removedFolder, itemPath );
                    m_diffActionList.push_back( sirius::drive::Action::remove( itemPath ) );

                    // save removed folder into 'replacedItems'
                    removedItems.push_back( std::move(removedFolder) );

                    // add local file
                    m_diffActionList.push_back( sirius::drive::Action::upload( localFolderPath / name, itemPath ) );
                    localItem.m_ldiStatus = ldi_added;
                }
                else
                {
                    // add (upload) local file
                    m_diffActionList.push_back( sirius::drive::Action::upload( localFolderPath / name, itemPath ) );
                    localItem.m_ldiStatus = ldi_changed;
                }
            }
        }
        else
        {
            // Item exists in local folder and absent in remote drive
            //
            if ( localItem.m_isFolder )
            {
                // add local folder
                addFolderWithChilds( localItem, itemPath, drivePath / name );
                localItem.m_ldiStatus = ldi_added;
            }
            else
            {
                // add (upload) local file
                m_diffActionList.push_back( sirius::drive::Action::upload( localFolderPath / name, itemPath ) );
                localItem.m_ldiStatus = ldi_added;
            }
        }
    }

    // Calculate files/folders that were removed by user
    //
    for( const auto& [name,fsItem] : fsFolder.childs() )
    {
        // Logic path of file/folder on the drive
        fs::path itemPath = drivePath.empty() ? fs::path(fsFolder.name()) : drivePath / fsFolder.name();

        // check that file/folder is absent in local drive copy
        if ( auto localIt = localChilds.find(name); localIt == localChilds.end() )
        {
            if ( isFolder(fsItem) )
            {
                // Remove folder

                // calculate removed folder (from drive)
                LocalDriveItem removedFolder{ true,name,{},{},{},ldi_removed};
                removeFolderWithChilds( getFolder(fsItem), removedFolder, itemPath );
                m_diffActionList.push_back( sirius::drive::Action::remove( itemPath ) );

                // save removed folder into 'removedItems'
                removedItems.push_back( std::move(removedFolder) );
            }
            else
            {
                // Remove file

                // save removed file into 'removedItems'
                removedItems.push_back( LocalDriveItem{ false,name,{},{},{},ldi_removed} );

                // remove remote file
                m_diffActionList.push_back( sirius::drive::Action::remove( itemPath ) );
            }
        }
    }

    // Append removed files/folders into localFolder.m_childs
    //
    for( auto& item: removedItems )
    {
        item.m_ldiStatus = ldi_removed;
        item.m_name.append(" ");
        localFolder.m_childs[item.m_name] = std::move(item);
    }
}

void Diff::removeFolderWithChilds( const sirius::drive::Folder& remoteFolder, LocalDriveItem& localFolder, fs::path path )
{
    for( const auto& [name,child]: remoteFolder.childs() )
    {
        if ( isFolder(child) )
        {
            LocalDriveItem folder{ true,name,{},{},{},ldi_removed};
            removeFolderWithChilds( getFolder(child), folder, path / name );
            m_diffActionList.push_back( sirius::drive::Action::remove( path / name ) );

            localFolder.m_childs[name] = std::move(folder);
        }
        else
        {
            m_diffActionList.push_back( sirius::drive::Action::remove( path / name ) );

            localFolder.m_childs[name] = LocalDriveItem{false,name,{},{},{},ldi_removed};
        }
    }
}

void Diff::addFolderWithChilds( LocalDriveItem& localFolder,
                                fs::path        localFolderPath,
                                fs::path        path )
{
    for( auto& [name,item]: localFolder.m_childs )
    {
        if ( item.m_isFolder )
        {
            addFolderWithChilds( item, localFolderPath / name, path / name );
        }
        else
        {
            m_diffActionList.push_back( sirius::drive::Action::upload( localFolderPath / name, path /name ) );
            item.m_ldiStatus = ldi_added;
        }
    }
}
