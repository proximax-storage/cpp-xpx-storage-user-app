#include <QApplication>
#include <QPalette>
#include <QPushButton>
#include <QDateTime>
#include <QFile>
#include <QUrl>
#include <QHostInfo>
#include <QTextStream>
#include <QHostAddress>
#include <QRegularExpression>
#include <QNetworkAccessManager>
#include <QMutex>
#include <random>
#include <iostream>
#include <xpxchaincpp/utils/HexParser.h>
#include <utils/HexFormatter.h>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <boost/beast/http/message.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <vector>
#include <tuple>
#include <string>
#include <atomic>
#include <mutex>
#include <chrono>
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

bool isFolderExists(const QString& path)
{
    const auto pathUtf8 = qStringToStdStringUTF8(path);
    return std::filesystem::exists( pathUtf8 ) || ! std::filesystem::is_directory( pathUtf8 );
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

QString getPrettyDriveState(int state) {
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

QString dataSizeToString(uint64_t bytes)
{
    QString units[] = {"bytes", "kB", "MB", "GB", "TB"};
    double b = bytes;
    int i = 0;
    for( ; b >= 1000 ; ++i )
    {
        b /= 1024.0;
    }

    std::ostringstream oss;
    if ( bytes < 1000)
    {
        oss << std::fixed << bytes << " " << qStringToStdStringUTF8(units[i]);
    }
    else
    {
        if ( b < 10.0 )
        {
            oss << std::fixed << std::setprecision(3) << b << " " << qStringToStdStringUTF8(units[i]);
        }
        else if ( b < 100.0 )
        {
            oss << std::fixed << std::setprecision(2) << b << " " << qStringToStdStringUTF8(units[i]);
        }
        else
        {
            oss << std::fixed << std::setprecision(1) << b << " " << qStringToStdStringUTF8(units[i]);
        }
    }

    return stdStringToQStringUtf8(oss.str());
}

bool isIpAddress(const QString &input) {
    QHostAddress address(input);
    if (address.protocol() != QAbstractSocket::UnknownNetworkLayerProtocol) {
        return true;
    }

    QRegularExpression ipRegex(R"((\d{1,3}\.){3}\d{1,3}(:\d+)?)");
    QRegularExpressionMatch match = ipRegex.match(input);
    return match.hasMatch();
}

bool isResolvedToIpAddress(QString& host) {
    if (isIpAddress(host)) {
        return true;
    }

    if (!host.startsWith("http://") || !host.startsWith("https://")) {
        host = "https://" + host;
    }

    QUrl parsedUrl(host);
    const auto name = parsedUrl.host();
    QHostInfo info = QHostInfo::fromName(name);
    if (info.error() == QHostInfo::NoError && !info.addresses().empty()) {
        host = info.addresses().first().toString() + ":" + QString::number(parsedUrl.port());
        return true;
    }

    qWarning() << "Utils::isResolvedToIpAddress: Cannot resolve host: " << host << " error: " << info.errorString();
    return false;
}

bool isEndpointAvailable(const std::string& ip, const std::string& port, std::atomic<bool>& found) {
    QTcpSocket socket;
    socket.connectToHost(QString::fromStdString(ip), QString::fromStdString(port).toUShort());
    if (!socket.waitForConnected(100)) {
        qWarning() << "isEndpointAvailable: Connection failed to: " << ip << " on port: " << port << " Error:" << socket.errorString();
        return false;
    }

    socket.close();
    return true;
}

QString getRandomEndpoint(std::vector<std::tuple<QString, QString, QString>>& nodes) {
    if (nodes.empty()) {
        return "";
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, nodes.size() - 1);

    int randomIndex = dis(gen);
    std::iter_swap(nodes.begin(), nodes.begin() + randomIndex);
    return std::get<0>(nodes[0]) + ":" + std::get<1>(nodes[0]);
}

QString extractEndpointFromComboBox(QComboBox* comboBox) {
    const int currentIndex = comboBox->currentIndex();
    const QString endpoint = comboBox->itemData(currentIndex, Qt::UserRole).toString();
    return endpoint;
}

bool isDarkSystemTheme() {
    QPalette palette = QApplication::palette();
    QColor textColor = palette.color(QPalette::WindowText);
    QColor backgroundColor = palette.color(QPalette::Window);
    return textColor.lightness() > backgroundColor.lightness();
}

QString prettyBalance(uint64_t value) {
    uint64_t decimals = 1000000;
    uint64_t integerPart = value / decimals;
    uint64_t fractionalPart = value % decimals;

    QLocale locale(QLocale::French);
    const QString formattedIntegerPart = locale.toString(integerPart);
    const QString formattedBalance = QString("%1.%2")
            .arg(formattedIntegerPart)
            .arg(fractionalPart, 6, 10, QChar('0'));

    return formattedBalance;
}

bool showConfirmationDialog(const QString& transactionFee, const QString& message) {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Confirmation");

    if (message.isEmpty())
    {
        msgBox.setText("The transaction will be sent, with an estimated cost\n"
                       "of approximately: " + transactionFee + " XPX.");
    } else
    {
        msgBox.setText("Do you want to continue downloading " + message + " ?\n"
                       "The transaction will be sent with an estimated cost\n"
                       "of approximately: " + transactionFee + " XPX.");
    }

    msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
    msgBox.button(QMessageBox::Ok)->setText("Confirm");
    msgBox.setModal(true);

    return msgBox.exec() == QMessageBox::Ok;
}

std::string qStringToStdStringUTF8(const QString& data) {
    std::string result = data.toUtf8().constData();
    return result;
}

QString stdStringToQStringUtf8(const std::string& data) {
    QString result = QString::fromUtf8(data.c_str(), static_cast<long long>(data.size()));
    return result;
}

std::vector<QString> convertToQStringVector(const std::vector<std::string>& stdStrings) {
    std::vector<QString> qStrings;
    qStrings.reserve(stdStrings.size());
    for (const auto& str : stdStrings) {
        qStrings.emplace_back(stdStringToQStringUtf8(str));
    }

    return qStrings;
}