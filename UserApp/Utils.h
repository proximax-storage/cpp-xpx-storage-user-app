#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <filesystem>
#include <QMessageLogger>
#include <QComboBox>
#include <QString>
#include <cereal/cereal.hpp>

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

bool isFolderExists(const QString& path);

std::filesystem::path getSettingsFolder();

std::filesystem::path getFsTreesFolder();

QString getPrettyDriveState(int state);

QString dataSizeToString(uint64_t bytes);

//QDebug& operator<<(QDebug& out, const std::string& str);

enum ErrorType
{
    Network,
    NetworkInit,
    InvalidData,
    Critical,
    Storage
};

inline std::filesystem::path make_path( const QString& str ) { return std::filesystem::path(str.toUtf8().constData()).make_preferred(); };

bool isIpAddress(const QString &input);

bool isResolvedToIpAddress(QString& host);

bool isEndpointAvailable(const QString& host, const QString& port, std::atomic<bool>& found);

QString getRandomEndpoint(std::vector<std::tuple<QString, QString, QString>>& nodes);

QString extractEndpointFromComboBox(QComboBox* comboBox);

bool isDarkSystemTheme();

QString prettyBalance(uint64_t value);

bool showConfirmationDialog(const QString& transactionFee);

std::string qStringToStdStringUTF8(const QString& data);

QString stdStringToQStringUtf8(const std::string& data);

std::vector<QString> convertToQStringVector(const std::vector<std::string>& stdStrings);

template <class Archive>
void save(Archive& archive, const QString& qstr) {
    std::string stdStr = qStringToStdStringUTF8(qstr);
    archive(stdStr);
}

template <class Archive>
void load(Archive& archive, QString& qstr) {
    std::string stdStr;
    archive(stdStr);
    qstr = stdStringToQStringUtf8(stdStr);
}

#endif //UTILS_H
