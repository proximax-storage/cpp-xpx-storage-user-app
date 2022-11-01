#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <iostream>
#include <xpxchaincpp/utils/HexParser.h>
#include <utils/HexFormatter.h>
#include "Utils.h"

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    static QMutex mutex;
    QMutexLocker lock(&mutex);

    QString date = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QString("[%1] ").arg(date);

    switch (type)
    {
        case QtInfoMsg:
            txt += QString("{Info} \t\t %1").arg(msg);
            break;
        case QtDebugMsg:
            txt += QString("{Debug} \t\t %1").arg(msg);
            break;
        case QtWarningMsg:
            txt += QString("{Warning} \t %1").arg(msg);
            break;
        case QtCriticalMsg:
            txt += QString("{Critical} \t %1").arg(msg);
            break;
        case QtFatalMsg:
            txt += QString("{Fatal} \t\t %1").arg(msg);
            break;
    }

    std::cerr << qPrintable(qFormatLogMessage(type, context, txt)) << std::endl;

    static QFile file("logs.log");
    static bool logFileIsOpen = file.open(QIODevice::WriteOnly | QIODevice::Append);

    if (logFileIsOpen) {
        file.write(qFormatLogMessage(type, context, txt).toUtf8() + '\n');
        file.flush();
    }
}


QString rawHashToHex(const std::array<uint8_t, 32>& rawHash) {
    std::ostringstream stream;
    stream << sirius::utils::HexFormat(rawHash);
    return stream.str().c_str();
}

std::array<uint8_t, 32> rawHashFromHex(const QString& hex) {
    std::array<uint8_t, 32> hash{};
    xpx_chain_sdk::ParseHexStringIntoContainer(hex.toStdString().c_str(), hex.size(), hash);
    return hash;
};