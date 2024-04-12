#pragma once

#include <iostream>
#include <array>
#include <filesystem>


using DownloadNotification = std::function<void( bool                         success,
                                                 const std::array<uint8_t,32> infoHash,
                                                 const std::filesystem::path  filePath,
                                                 size_t                       downloaded,
                                                 size_t                       fileSize,
                                                 const std::string&           errorText )>;

