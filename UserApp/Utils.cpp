#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QCoreApplication>
#include <iostream>
#include <xpxchaincpp/utils/HexParser.h>
#include <utils/HexFormatter.h>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include "Utils.h"
#include "Entities/Drive.h"

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)

    static QMutex mutex;
    QMutexLocker lock(&mutex);

    QString date = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QString("[%1] ").arg(date);

    switch (type)
    {
        case QtInfoMsg:
            txt += QString("{Info} %1").arg(msg);
            break;
        case QtDebugMsg:
            txt += QString("{Debug} %1").arg(msg);
            break;
        case QtWarningMsg:
            txt += QString("{Warning} %1").arg(msg);
            break;
        case QtCriticalMsg:
            txt += QString("{Critical} %1").arg(msg);
            break;
        case QtFatalMsg:
            txt += QString("{Fatal} %1").arg(msg);
            break;
    }

    std::cerr << qPrintable(qFormatLogMessage(type, context, txt)) << std::endl;

    static const auto path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (path.isEmpty())
    {
        qFatal("Cannot determine settings storage location");
    }

    static QDir d(path);
    if (!d.mkpath(d.absolutePath()))
    {
        qFatal("Cannot create path to settings dir");
    }

    // Windows: 'C:\Users\user\AppData\Roaming\ProximaX\StorageManager'
    // Linux: '/home/user/.local/share/ProximaX/StorageManager'
    static QFile file(d.absolutePath() + "/logs.txt");
    bool isLimitSucceeded = file.size() >= 50 * 1024 * 1024;
    if (isLimitSucceeded)
    {
        file.resize(0);
    }

    static bool logFileIsOpen = file.open(QIODevice::WriteOnly | QIODevice::Append);
    if (logFileIsOpen)
    {
        file.write(qFormatLogMessage(type, context, txt).toUtf8() + '\n');
        file.flush();
    }
}


QString rawHashToHex(const std::array<uint8_t, 32>& rawHash)
{
    std::ostringstream stream;
    stream << sirius::utils::HexFormat(rawHash);
    return stream.str().c_str();
}

std::array<uint8_t, 32> rawHashFromHex(const QString& hex)
{
    std::array<uint8_t, 32> hash{};
    xpx_chain_sdk::ParseHexStringIntoContainer(hex.toStdString().c_str(), hex.size(), hash);
    return hash;
}

QString getResource( const QString& resource )
{
#ifdef __APPLE__
    return QCoreApplication::applicationDirPath() + "/." + resource;
#else
    return resource;
#endif
}

bool isFolderExists(const std::string& path)
{
    return std::filesystem::exists( path ) || ! std::filesystem::is_directory( path );
}

std::filesystem::path getSettingsFolder()
{
    std::filesystem::path path;
#ifdef _WIN32
    path = QDir::currentPath().toStdString() + "/XpxSiriusStorageClient";
#else
    const char* homePath = getenv("HOME");
    path = std::string(homePath) + "/.XpxSiriusStorageClient";
#endif

    std::error_code ec;
    if ( ! std::filesystem::exists( path.make_preferred(), ec ) )
    {
        std::filesystem::create_directories( path, ec );
        if ( ec )
        {
            QMessageBox msgBox;
            msgBox.setText( QString::fromStdString( "Cannot create folder: " + path.make_preferred().string() ) );
            msgBox.setInformativeText( QString::fromStdString( ec.message() ) );
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            exit(1);
        }
    }

    return path;
}

std::filesystem::path getFsTreesFolder()
{
    return {getSettingsFolder().string() + "/FsTrees" };
}

std::string getPrettyDriveState(int state) {
    switch (state)
    {
        case no_modifications:
            return "no_modifications";
        case registering:
            return "registering";
        case approved:
            return "approved";
        case failed:
            return "failed";
        case canceling:
            return "canceling";
        case canceled:
            return "canceled";
        case uploading:
            return "uploading";
        case creating:
            return "creating";
        case unconfirmed:
            return "unconfirmed";
        case deleting:
            return "deleting";
        case deleted:
            return "deleted";
        default:
            return "unknown drive state";
    }
}

QDebug& operator<<(QDebug& out, const std::string& str)
{
    out << QString::fromStdString(str);
    return out;
}
