#include "Diff.h"
#ifndef WA_APP
#include "drive/Session.h"
#endif
#include "drive/Utils.h"
#include "Utils.h"

#include <QDebug>

void Diff::calcLocalDriveInfoR( LocalDriveItem& parent, fs::path path, bool calculateHashes, std::array<uint8_t,32>* driveKey )
{
    if ( ! fs::exists(path.make_preferred()) || ! fs::is_directory(path.make_preferred()) )
    {
        return;
    }

    for( const auto& entry : std::filesystem::directory_iterator(path.make_preferred()) )
    {
        const auto entryNameQStringUFT8 = stdStringToQStringUtf8(entry.path().filename().make_preferred().string());
        const auto entryNameStdStringUFT8 = qStringToStdStringUTF8(entryNameQStringUFT8);
        if ( entry.is_directory() )
        {
            std::error_code errorCode(errno, std::generic_category());
            auto lastWriteTime = fs::last_write_time(entry, errorCode);

            if (errorCode.value() == 0) {
                LocalDriveItem child{ true, entryNameQStringUFT8, 0, "", {}, lastWriteTime };
                calcLocalDriveInfoR( child, (path / entryNameStdStringUFT8).make_preferred(), calculateHashes, driveKey );
                parent.m_childs[entryNameQStringUFT8] = std::move(child);
            } else {
                qWarning () << "Diff::calcLocalDriveInfoR. entry.is_directory(). Error code: " << errorCode.value() << " message: " << errorCode.message().c_str();
            }
        }
        else if ( entry.is_regular_file() && entryNameQStringUFT8 != ".DS_Store" )
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

#ifdef WA_APP
                    //TODO_WA
#else
                    hash = sirius::drive::calculateInfoHash((path / entryNameStdStringUFT8).make_preferred(), sirius::Key(*driveKey) ).array();
#endif
                    qWarning () << "driveKey: " << sirius::drive::toString(*driveKey).c_str();
                    qWarning () << "filename: " << (path / entryNameStdStringUFT8).string().c_str();
                    qWarning () << "hash: " << sirius::drive::toString(hash).c_str();
                }

                std::error_code errorCode(errno, std::generic_category());
                auto lastWriteTime = fs::last_write_time(fs::path(entry).make_preferred(), errorCode);

                if (errorCode.value() == 0) {
                    parent.m_childs[entryNameQStringUFT8] = LocalDriveItem{ false, entryNameQStringUFT8, fileSize, hash.array(), {}, lastWriteTime };
                } else {
                    qWarning () << "Diff::calcLocalDriveInfoR. entry.is_regular_file(). Error code: " << errorCode.value() << " message: " << errorCode.message().c_str();
                }
            }
            else
            {
                std::error_code errorCode(errno, std::generic_category());
                auto lastWriteTime = fs::last_write_time(fs::path(entry).make_preferred(), errorCode);

                if (errorCode.value() == 0) {
                    parent.m_childs[entryNameQStringUFT8] = LocalDriveItem{ false, entryNameQStringUFT8, fileSize, {}, {}, lastWriteTime };
                } else {
                    qWarning () << "Diff::calcLocalDriveInfoR. entry.is_directory(). Error code: " << errorCode.value() << " message: " << errorCode.message().c_str();
                }
            }
        }
    }
}

void Diff::updateLocalDriveInfoR( LocalDriveItem&           newRoot,
                                  const LocalDriveItem&     oldRoot,
                                  fs::path                  fsPath,
                                  std::array<uint8_t,32>&   driveKey )
{
    for( const auto& entry : std::filesystem::directory_iterator(fsPath.make_preferred()) )
    {
        const auto entryName = entry.path().filename().string();
        const QString entryNameQUTF8 = stdStringToQStringUtf8(entryName);

        if ( entry.is_directory() )
        {
            std::error_code errorCode(errno, std::generic_category());
            auto lastWriteTime = fs::last_write_time(fs::path(entry).make_preferred(), errorCode);

            if (errorCode.value() == 0) {
                LocalDriveItem child{ true, entryNameQUTF8, 0, "", {}, lastWriteTime };

                auto oldIt = oldRoot.m_childs.find( entryNameQUTF8 );
                if ( oldIt != oldRoot.m_childs.end() && oldIt->second.m_isFolder )
                {
                    updateLocalDriveInfoR( child, oldIt->second, fs::path(fsPath.string() + "/" + entryName).make_preferred(), driveKey );
                }
                else
                {
                    updateLocalDriveInfoR( child, {}, fs::path(fsPath.string() + "/" + entryName).make_preferred(), driveKey );
                }

                newRoot.m_childs[entryNameQUTF8] = std::move(child);
            } else {
                qWarning () << "Diff::updateLocalDriveInfoR. entry.is_directory(). Error code: " << errorCode.value() << " message: " << errorCode.message().c_str();
            }
        }
        else if ( entry.is_regular_file() && entryName != ".DS_Store" )
        {
            size_t fileSize = entry.file_size();

            auto oldIt = oldRoot.m_childs.find( entryNameQUTF8 );
            if ( oldIt != oldRoot.m_childs.end() && ! oldIt->second.m_isFolder )
            {
                std::error_code errorCode(errno, std::generic_category());
                auto modifyTime = fs::last_write_time((fsPath/ entryName).make_preferred(), errorCode);

                if (errorCode.value() == 0) {
                    if ( modifyTime == oldIt->second.m_modifyTime )
                    {
                        newRoot.m_childs[entryNameQUTF8] = LocalDriveItem{ false, entryNameQUTF8, fileSize, oldIt->second.m_fileHash, {}, modifyTime };
                        continue;
                    }
                } else {
                    qWarning () << "Diff::updateLocalDriveInfoR. oldIt != oldRoot.m_childs.end(). Error code: " << errorCode.value() << " message: " << errorCode.message().c_str();
                }
            }

            sirius::drive::InfoHash hash;
            if ( entry.file_size() == 0 )
            {
                std::memset( &hash[0], 0, sizeof(hash) );
            }
            else
            {
#ifdef WA_APP
    //TODO_WA
#else
                hash = sirius::drive::calculateInfoHash( (fsPath / entryName).make_preferred(), sirius::Key(driveKey) ).array();
#endif
            }

            std::error_code errorCode(errno, std::generic_category());
            auto modifyTime = fs::last_write_time(fs::path(entry).make_preferred(), errorCode);

            if (errorCode.value() == 0) {
                newRoot.m_childs[entryNameQUTF8] = LocalDriveItem{ false, entryNameQUTF8, fileSize, hash.array(), {}, modifyTime };
            } else {
                qWarning () << "Diff::updateLocalDriveInfoR. Error code: " << errorCode.value() << " message: " << errorCode.message().c_str();
            }
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

    // Find files that were added or modified by user
    //
    for( auto& [name, localChild] : localChilds )
    {
        const auto nameUTF8 = qStringToStdStringUTF8(name);

        // Logic path of file/folder on the drive
        fs::path childPath = localFolderPath / nameUTF8;
        fs::path fsChildPath = fsPath.empty() ? fs::path(qStringToStdStringUTF8(localChild.m_name)) : fs::path(fsPath / nameUTF8);
        qDebug() << "Diff::calcDiff !!! fsChildPath: " << fsChildPath.string().c_str();

        // check that file (or folder) exists on the drive
        //
        //fsFolder.dbgPrint();
        if ( auto fsIt = fsFolder.childs().find(nameUTF8); fsIt != fsFolder.childs().end() )
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
                    LocalDriveItem item;
                    item.m_isFolder = false;
                    item.m_name = name;
                    item.m_size = getFile(fsIt->second).size();
                    item.m_fileHash = {};
                    item.m_childs = {};
                    item.m_modifyTime = {};
                    item.m_ldiStatus = ldi_removed;

                    removedItems.emplace_back(item);

                    // remove remote file
                    m_diffActionList.emplace_back( sirius::drive::Action::remove( fsChildPath.string() ) );

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
                    m_diffActionList.push_back( sirius::drive::Action::remove( fsChildPath.string() ) );

                    // add local file
                    if ( localChild.m_size > 0 )
                    {
                        m_diffActionList.push_back( sirius::drive::Action::upload( childPath.string(), fsChildPath.string() ) );
                        localChild.m_ldiStatus = ldi_added;
                    }
                }
                else
                {
                    if ( localChild.m_fileHash != getFile(fsIt->second).hash().array() )
                    {
                        // Replace file
                        qDebug() << "HASH!!! " << sirius::drive::arrayToString(localChild.m_fileHash).c_str() << " != "
                                    << sirius::drive::arrayToString(getFile(fsIt->second).hash().array()).c_str();

                        // add (upload) local file
                        if ( localChild.m_size == 0 )
                        {
                            m_diffActionList.push_back( sirius::drive::Action::remove( fsChildPath.string() ) );
                        }
                        else
                        {
                            m_diffActionList.push_back( sirius::drive::Action::upload( childPath.string(), fsChildPath.string() ) );
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
                addFolderWithChilds( localChild, childPath.make_preferred(), fsChildPath );
                localChild.m_ldiStatus = ldi_added;
            }
            else
            {
                // add (upload) local file
                if ( localChild.m_size > 0 )
                {
                    m_diffActionList.push_back( sirius::drive::Action::upload( childPath.string(), fsChildPath.string() ) );
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
        const auto nameUTF8 = stdStringToQStringUtf8(name);
        // check that file/folder is absent in local drive copy
        if ( auto localIt = localChilds.find(nameUTF8); localIt == localChilds.end() )
        {
            if ( isFolder(fsChild) )
            {
                // Remove folder
                fs::path fsChildPath = fsPath.empty() ? fs::path(getFolder(fsChild).name()) : fs::path(fsPath.string() + "/" + getFolder(fsChild).name());
                qDebug() << LOG_SOURCE << "! fsChildPath: " << fsChildPath.string().c_str() << " ?: " << getFolder(fsChild).name().c_str();

                // remov folder content (from drive)
                LocalDriveItem removedFolder{ true,nameUTF8,0,{},{},{},ldi_removed};
                removeFolderWithChilds( getFolder(fsChild), removedFolder, fsChildPath );

                // save removed folder into 'removedItems'
                removedItems.push_back( std::move(removedFolder) );

                // remove fs folder
                m_diffActionList.push_back( sirius::drive::Action::remove( fsChildPath.string() ) );

            }
            else
            {
                // Remove file
                fs::path fsChildPath = fsPath.empty() ? fs::path(getFile(fsChild).name()) : fs::path(fsPath.string() + "/" + getFile(fsChild).name());
                qDebug() << LOG_SOURCE << "! remove fsChildPath: " << fsChildPath.string().c_str();

                // save removed file into 'removedItems'
                removedItems.emplace_back(LocalDriveItem{ false,nameUTF8,getFile(fsChild).size(),{},{},{},ldi_removed} );

                // remove remote file
                m_diffActionList.push_back( sirius::drive::Action::remove( fsChildPath.string() ) );
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
        const auto nameUTF8 = stdStringToQStringUtf8(name);
        if ( isFolder(child) )
        {
            LocalDriveItem folder{ true, nameUTF8,{},0,{},{},ldi_removed};
            removeFolderWithChilds( getFolder(child), folder, path / name);

            localFolder.m_childs[nameUTF8] = std::move(folder);
        }
        else
        {
            localFolder.m_childs[nameUTF8] = LocalDriveItem{false,nameUTF8,getFile(child).size(),{},{},{},ldi_removed};
        }
    }
}

void Diff::addFolderWithChilds( LocalDriveItem& localFolder,
                                fs::path        localFolderPath,
                                fs::path        path )
{
    for( auto& [name,item]: localFolder.m_childs )
    {
        const auto nameUTF8 = qStringToStdStringUTF8(name);
        if ( item.m_isFolder )
        {
            addFolderWithChilds( item, localFolderPath / nameUTF8, path / nameUTF8 );
        }
        else
        {
            if ( item.m_size > 0 )
            {
                m_diffActionList.push_back( sirius::drive::Action::upload((localFolderPath / nameUTF8).string(), (path / nameUTF8).string()) );
                item.m_ldiStatus = ldi_added;
            }
        }
    }
}
