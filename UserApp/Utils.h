#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <filesystem>
#include <QMessageLogger>
#include <QComboBox>

#define LOG_SOURCE \
    __FILE__ << __LINE__

#define ASSERT(expr) { \
    if (!(expr)) {\
        std::cerr << ": " << __FILE__ << ":" << __LINE__ << " failed assert: " << #expr << "\n" << std::flush; \
        assert(0); \
    }\
}

#define CLIENT_SANDBOX_FOLDER "/modify_drive_data" // it is used for user modification torrents
#define CLIENT_SANDBOX_FOLDER0 "modify_drive_data" // it is used for user modification torrents

#ifdef WA_APP
#define MAP_CONTAINS(map,key) (map.find(key) != map.end())
#else
#define MAP_CONTAINS(map,key) map.contains(key)
#endif



void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

QString rawHashToHex(const std::array<uint8_t, 32>& rawHash);

std::array<uint8_t, 32> rawHashFromHex(const QString& hex);

QString getResource(const QString& resource);

bool isFolderExists(const std::string& path);

std::filesystem::path getSettingsFolder();

std::filesystem::path getFsTreesFolder();

std::string getPrettyDriveState(int state);

std::string dataSizeToString(uint64_t bytes);

QDebug& operator<<(QDebug& out, const std::string& str);

enum ErrorType
{
    Network,
    NetworkInit,
    InvalidData,
    Critical,
    Storage
};

inline std::filesystem::path make_path( const std::string& str ) { return std::filesystem::path(str).make_preferred(); };

bool isResolvedToIpAddress(QString& host);

bool isEndpointAvailable(const std::string& host, const std::string& port, std::atomic<bool>& found);

std::string getFastestEndpoint(std::vector<std::tuple<QString, QString, QString>>& nodes);

std::string extractEndpointFromComboBox(QComboBox* comboBox);

bool isDarkSystemTheme();

#endif //UTILS_H
