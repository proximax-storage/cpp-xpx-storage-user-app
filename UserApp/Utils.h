#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <filesystem>
#include <QMessageLogger>

#define LOG_SOURCE \
    __FILE__ << __LINE__

#define ASSERT(expr) { \
    if (!(expr)) {\
        std::cerr << ": " << __FILE__ << ":" << __LINE__ << " failed assert: " << #expr << "\n" << std::flush; \
        assert(0); \
    }\
}

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

QString rawHashToHex(const std::array<uint8_t, 32>& rawHash);

std::array<uint8_t, 32> rawHashFromHex(const QString& hex);

QString getResource(const QString& resource);

bool isFolderExists(const std::string& path);

std::filesystem::path getSettingsFolder();

std::filesystem::path getFsTreesFolder();

QDebug& operator<<(QDebug& out, const std::string& str);

#endif //UTILS_H
