#include "Diff.h"
#include "drive/Session.h"
#include "drive/Utils.h"
#include "Utils.h"

#include <QDebug>

void Diff::calcLocalDriveInfoR( LocalDriveItem& parent, fs::path path, bool calculateHashes, std::array<uint8_t,32>* driveKey )
{
    if ( ! fs::exists(path) || ! fs::is_directory(path) )
    {
        return;
    }

    for( const auto& entry : std::filesystem::directory_iterator(path) )
    {
        const auto entryName = entry.path().filename().string();
        if ( entry.is_directory() )
        {
            LocalDriveItem child{ true, entryName, 0, "", {}, fs::last_write_time(entry) };
            calcLocalDriveInfoR( child, path / entryName, calculateHashes, driveKey );
            parent.m_childs[entryName] = std::move(child);
        }
        else if ( entry.is_regular_file() && entryName != ".DS_Store" )
        {
            size_t fileSize = entry.file_size();

            if ( calculateHashes )
            {
                sirius::drive::InfoHash hash;
                if ( entry.file_size() == 0 )
                {
                    std::memset( &hash[0], 0, sizeof(hash) );
                }
                else
                {
                    hash = sirius::drive::calculateInfoHash( path / entryName, sirius::Key(*driveKey) ).array();
                }

                parent.m_childs[entryName] = LocalDriveItem{ false, entryName, fileSize, hash.array(), {}, fs::last_write_time(entry) };
            }
            else
            {
                parent.m_childs[entryName] = LocalDriveItem{ false, entryName, fileSize, {}, {}, fs::last_write_time(entry) };
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
            LocalDriveItem child{ true, entryName, 0, "", {}, fs::last_write_time(entry) };

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
            size_t fileSize = entry.file_size();

            auto oldIt = oldRoot.m_childs.find( entryName );
            if ( oldIt != oldRoot.m_childs.end() && ! oldIt->second.m_isFolder )
            {
                auto modifyTime = fs::last_write_time( fsPath/entryName );
                if ( modifyTime == oldIt->second.m_modifyTime )
                {
                    newRoot.m_childs[entryName] = LocalDriveItem{ false, entryName, fileSize, oldIt->second.m_fileHash, {}, modifyTime };
                    continue;
                }
            }

            sirius::drive::InfoHash hash;
            if ( entry.file_size() == 0 )
            {
                std::memset( &hash[0], 0, sizeof(hash) );
            }
            else
            {
                hash = sirius::drive::calculateInfoHash( fsPath / entryName, sirius::Key(driveKey) ).array();
            }

            newRoot.m_childs[entryName] = LocalDriveItem{ false, entryName, fileSize, hash.array(), {}, fs::last_write_time(entry) };
        }
    }
}

bool Diff::calcDiff( LocalDriveItem&                localFolder,
                     fs::path                       localFolderPath,
                     const sirius::drive::Folder&   fsFolder,
                     fs::path                       fsPath)
{
    bool isChanged = false;

    auto& localChilds = localFolder.m_childs;

    // 'removedItems' are files (or folders) that
    // that will be added in localFolder as 'ldi_removed'
    //
    std::list<LocalDriveItem> removedItems;

    // Find files that were added or modifyed by user
    //
    for( auto& [name, localChild] : localChilds )
    {
        // Logic path of file/folder on the drive
        fs::path childPath = localFolderPath / name;
        fs::path fsChildPath = fsPath.empty() ? fs::path(localChild.m_name) : fsPath / name;
        qDebug() << LOG_SOURCE << "!!! fsChildPath: " << fsChildPath.string().c_str();

        // check that file (or folder) exists on the drive
        //
        if ( auto fsIt = fsFolder.childs().find(name); fsIt != fsFolder.childs().end() )
        {
            // Item exists in local folder and in remote drive
            //
            if ( localChild.m_isFolder )
            {
                if ( isFolder(fsIt->second) )
                {
                    // continue calculation
                    if ( calcDiff( localChild, childPath, getFolder(fsIt->second), fsChildPath ) )
                    {
                        isChanged = true;
                    }
                }
                else
                {
                    // Remote file has been replaced by folder with same name

                    // save removed file into 'replacedItems'
                    removedItems.push_back( LocalDriveItem{ false, name, getFile(fsIt->second).size(), {}, {}, {}, ldi_removed } );

                    // remove remote file
                    m_diffActionList.push_back( sirius::drive::Action::remove( fsChildPath ) );

                    // add local folder
                    addFolderWithChilds( localChild, childPath, fsChildPath );
                    localChild.m_ldiStatus = ldi_added;
                }
            }
            else
            {
                if ( isFolder(fsIt->second) )
                {
                    // Remote folder has been replaced by file with same name

                    // remove folder content (from drive)
                    LocalDriveItem removedFolder{ false, name, 0, {}, {}, {}, ldi_removed };
                    removeFolderWithChilds( getFolder(fsIt->second), removedFolder, fsChildPath );

                    // save it into 'replacedItems'
                    removedItems.push_back( std::move(removedFolder) );

                    // remove fs folder
                    m_diffActionList.push_back( sirius::drive::Action::remove( fsChildPath ) );

                    // add local file
                    if ( localChild.m_size > 0 )
                    {
                        m_diffActionList.push_back( sirius::drive::Action::upload( childPath, fsChildPath ) );
                        localChild.m_ldiStatus = ldi_added;
                    }
                }
                else
                {
                    if ( localChild.m_fileHash != getFile(fsIt->second).hash().array() )
                    {
                        // Replace file

                        // add (upload) local file
                        if ( localChild.m_size == 0 )
                        {
                            m_diffActionList.push_back( sirius::drive::Action::remove( fsChildPath ) );
                        }
                        else
                        {
                            m_diffActionList.push_back( sirius::drive::Action::upload( childPath, fsChildPath ) );
                        }

                        localChild.m_ldiStatus = ldi_changed;
                        isChanged = true;
                    }
                }
            }
        }
        else
        {
            // Item exists in local folder and absent in remote drive
            //
            if ( localChild.m_isFolder )
            {
                // add local folder
                addFolderWithChilds( localChild, childPath, fsChildPath );
                localChild.m_ldiStatus = ldi_added;
            }
            else
            {
                // add (upload) local file
                if ( localChild.m_size > 0 )
                {
                    m_diffActionList.push_back( sirius::drive::Action::upload( childPath, fsChildPath ) );
                    localChild.m_ldiStatus = ldi_added;
                }
            }

            isChanged = true;
        }
    }

    // Calculate files/folders that were removed by user
    //
    for( const auto& [name,fsChild] : fsFolder.childs() )
    {
        // check that file/folder is absent in local drive copy
        if ( auto localIt = localChilds.find(name); localIt == localChilds.end() )
        {
            if ( isFolder(fsChild) )
            {
                // Remove folder
                fs::path fsChildPath = fsPath.empty() ? fs::path(getFolder(fsChild).name()) : fsPath / getFolder(fsChild).name();
                qDebug() << LOG_SOURCE << "! fsChildPath: " << fsChildPath.string().c_str() << " ?: " << getFolder(fsChild).name().c_str();

                // remov folder content (from drive)
                LocalDriveItem removedFolder{ true,name,0,{},{},{},ldi_removed};
                removeFolderWithChilds( getFolder(fsChild), removedFolder, fsChildPath );

                // save removed folder into 'removedItems'
                removedItems.push_back( std::move(removedFolder) );

                // remove fs folder
                m_diffActionList.push_back( sirius::drive::Action::remove( fsChildPath ) );

            }
            else
            {
                // Remove file
                fs::path fsChildPath = fsPath.empty() ? fs::path(getFile(fsChild).name()) : fsPath / getFile(fsChild).name();
                qDebug() << LOG_SOURCE << "! remove fsChildPath: " << fsChildPath.string().c_str();

                // save removed file into 'removedItems'
                removedItems.push_back( LocalDriveItem{ false,name,getFile(fsChild).size(),{},{},{},ldi_removed} );

                // remove remote file
                m_diffActionList.push_back( sirius::drive::Action::remove( fsChildPath ) );
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
        isChanged = true;
    }

    if ( isChanged )
    {
        localFolder.m_ldiStatus = ldi_changed;
    }

    return isChanged;
}

void Diff::removeFolderWithChilds( const sirius::drive::Folder& remoteFolder, LocalDriveItem& localFolder, fs::path path )
{
    for( const auto& [name,child]: remoteFolder.childs() )
    {
        if ( isFolder(child) )
        {
            LocalDriveItem folder{ true,name,{},0,{},{},ldi_removed};
            removeFolderWithChilds( getFolder(child), folder, path / name );
            //m_diffActionList.push_back( sirius::drive::Action::remove( path / name ) );

            localFolder.m_childs[name] = std::move(folder);
        }
        else
        {
            //m_diffActionList.push_back( sirius::drive::Action::remove( path / name ) );

            localFolder.m_childs[name] = LocalDriveItem{false,name,getFile(child).size(),{},{},{},ldi_removed};
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
            if ( item.m_size > 0 )
            {
                m_diffActionList.push_back( sirius::drive::Action::upload( localFolderPath / name, path /name ) );
                item.m_ldiStatus = ldi_added;
            }
        }
    }
}
