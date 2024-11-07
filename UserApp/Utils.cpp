#include <QApplication>
#include <QPalette>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <iostream>
#include <xpxchaincpp/utils/HexParser.h>
#include <utils/HexFormatter.h>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/read.hpp>
#include <vector>
#include <tuple>
#include <string>
#include <future>
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

std::string dataSizeToString(uint64_t bytes)
{
    std::string units[] = {"bytes", "kB", "MB", "GB", "TB"};
    double b = bytes;
    int i = 0;
    for( ; b >= 1000 ; ++i )
    {
        b /= 1024.0;
    }
    std::ostringstream oss;
    if ( bytes < 1000)
    {
        oss << std::fixed << bytes << " " << units[i] ;
    }
    else
    {
        if ( b < 10.0 )
        {
            oss << std::fixed << std::setprecision(3) << b << " " << units[i] ;
        }
        else if ( b < 100.0 )
        {
            oss << std::fixed << std::setprecision(2) << b << " " << units[i] ;
        }
        else
        {
            oss << std::fixed << std::setprecision(1) << b << " " << units[i] ;
        }
    }
    return oss.str();
}

bool isResolvedToIpAddress(QString& host) {
    try {
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::resolver resolver(io_context);

        const auto rawEndpoint = host.split(':');
        if (rawEndpoint.size() == 2) {
            const auto& address = rawEndpoint[0];
            const auto& port = rawEndpoint[1];

            auto endpoints = resolver.resolve(address.toStdString(), port.toStdString());
            if (!endpoints.empty()) {
                host = QString::fromStdString(endpoints.begin()->endpoint().address().to_string() + ":" + port.toStdString());
                return true;
            }
        } else {
            qWarning() << "Utils::isResolvedToIpAddress: Port is missing: " << " host: " << host;
        }
    } catch (const boost::system::system_error& e) {
        qWarning() << "Utils::isResolvedToIpAddress: Cannot resolve host: " << e.what() << " host: " << host;
    }

    return false;
}

bool isEndpointAvailable(const std::string& ip, const std::string& port, std::atomic<bool>& found) {
    try {
        boost::asio::io_context ioc;
        boost::beast::tcp_stream stream(ioc);
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(ip), QString::fromStdString(port).toUShort());
        stream.connect(endpoint);

        boost::beast::http::request<boost::beast::http::string_body> request{boost::beast::http::verb::get, "/network", 11};
        request.set(boost::beast::http::field::host, ip);
        request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        boost::beast::http::write(stream, request);
        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::dynamic_body> res;
        boost::beast::http::read(stream, buffer, res);

        boost::beast::error_code ec;
        stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        return res.result() == boost::beast::http::status::ok && !found.load();

    } catch (const std::exception& e) {
        qWarning() << "Utils::isEndpointAvailable: Host error: " << e.what() << " ip: " << ip;
        return false;
    }
}

std::string getFastestEndpoint(std::vector<std::tuple<QString, QString, QString>>& nodes) {
    std::atomic<bool> isFound{false};
    std::string fastestEndpoint;
    std::mutex mutex;

    std::pair<std::chrono::milliseconds, int> minResponse{std::chrono::milliseconds::max(), -1};
    std::vector<std::future<void>> futures;
    for (int i = 0; i < nodes.size(); ++i) {
        const auto& [host, port, ip] = nodes[i];
        futures.push_back(std::async(std::launch::async, [&, i]() {
            auto start = std::chrono::steady_clock::now();
            if (!isFound.load() && isEndpointAvailable(ip.toStdString(), port.toStdString(), isFound)) {
                auto end = std::chrono::steady_clock::now();
                auto responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                std::lock_guard<std::mutex> lock(mutex);
                if (responseTime < minResponse.first) {
                    minResponse = {responseTime, i};
                    fastestEndpoint = host.toStdString() + ":" + port.toStdString();
                }
            }
        }));
    }

    for (auto& f : futures) {
        f.wait();
    }

    if (minResponse.second != -1) {
        std::iter_swap(nodes.begin(), nodes.begin() + minResponse.second);
    }

    return fastestEndpoint;
}

std::string extractEndpointFromComboBox(QComboBox* comboBox) {
    int currentIndex = comboBox->currentIndex();
    const QString endpoint = comboBox->itemData(currentIndex, Qt::UserRole).toString();
    return endpoint.toStdString();
}

bool isDarkSystemTheme() {
    QPalette palette = QApplication::palette();
    QColor textColor = palette.color(QPalette::WindowText);
    QColor backgroundColor = palette.color(QPalette::Window);
    return textColor.lightness() > backgroundColor.lightness();
}