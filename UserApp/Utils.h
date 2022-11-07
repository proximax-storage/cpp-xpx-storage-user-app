#ifndef UTILS_H
#define UTILS_H

#define LOG_SOURCE \
    __FILE__ << __LINE__
#include <QMessageLogger>

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
QString rawHashToHex(const std::array<uint8_t, 32>& rawHash);
std::array<uint8_t, 32> rawHashFromHex(const QString& hex);


#endif //UTILS_H
