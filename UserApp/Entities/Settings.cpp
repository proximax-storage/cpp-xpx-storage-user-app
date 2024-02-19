#include "Settings.h"
#include "Models/Model.h"
#include "Engines/StorageEngine.h"
#include "drive/Utils.h"
#include <fstream>
#include <filesystem>

#include <cereal/types/string.hpp>
#include <cereal/archives/portable_binary.hpp>

#include <QDebug>
#include <QMessageBox>

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_configPath(fs::path(getSettingsFolder().string() + "/config").make_preferred())
{
}

Settings::~Settings()
{}

Settings::Settings(const Settings& s)
{
    m_restBootstrap = s.m_restBootstrap;
    m_replicatorBootstrap = s.m_replicatorBootstrap;
    m_udpPort = s.m_udpPort;
    m_feeMultiplier = s.m_feeMultiplier;
    m_windowGeometry = s.m_windowGeometry;
    m_isDriveStructureAsTree = s.m_isDriveStructureAsTree;
    m_accounts = s.m_accounts;
    m_currentAccountIndex = s.m_currentAccountIndex;
    m_loaded = s.m_loaded;
}

Settings &Settings::operator=(const Settings &s) {
    if (this == &s) {
        return *this;
    }

    m_restBootstrap = s.m_restBootstrap;
    m_replicatorBootstrap = s.m_replicatorBootstrap;
    m_udpPort = s.m_udpPort;
    m_feeMultiplier = s.m_feeMultiplier;
    m_windowGeometry = s.m_windowGeometry;
    m_isDriveStructureAsTree = s.m_isDriveStructureAsTree;
    m_accounts = s.m_accounts;
    m_currentAccountIndex = s.m_currentAccountIndex;
    m_loaded = s.m_loaded;

    return *this;
}

void Settings::initForTests()
{
    if ( m_accounts.empty() && Model::homeFolder() == "/Users/alex" )
    {
        m_restBootstrap       = "54.151.169.225:3000";

        m_accounts.emplace_back();
        setCurrentAccountIndex( (int)m_accounts.size() - 1 );
        config().initAccount( "test_staging_DA", "3C7C91E82BF69B206A523E64DB21B07598834970065ABCFA2BB4212138637E0B" );
        config().m_downloadFolder = "/Users/alex/000-Downloads";

        m_accounts.emplace_back();
        setCurrentAccountIndex( (int)m_accounts.size() - 1 );
        config().initAccount( "test_staging_FA", "FEB6F12EEF165E0BF19B8A1D02A2C3BEF5DA6B88897E241C3A47B12A6E2FC153" );
        config().m_downloadFolder = "/Users/alex/000-Downloads";

        m_accounts.emplace_back();
        setCurrentAccountIndex( (int)m_accounts.size() - 1 );
        config().initAccount( "test_staging_F95", "834C1DBBEC0E8A6E5262BF409CC66DDA6DB3B1A292B39B58CC83FEBA7FF33973" );
        config().m_downloadFolder = "/Users/alex/000-Downloads";

        if ( ! ALEX_LOCAL_TEST )
        {
            setCurrentAccountIndex( 2 );
        }
    }
}

bool Settings::load( const std::string& pwd )
{
    try
    {
        std::error_code ec;
        bool isExists = fs::exists( m_configPath, ec );
        if (ec)
        {
            qCritical () << "Settings::load. fs::exists error: " << ec.message() << " code: " << std::to_string(ec.value()) << " path: " << m_configPath.string();
            return false;
        }

        if ( ! isExists )
        {
            return false;
        }

        std::ifstream ifStream( m_configPath, std::ios::binary );

        std::ostringstream os;
        os << ifStream.rdbuf();

        std::istringstream is( os.str(), std::ios::binary );
        cereal::PortableBinaryInputArchive iarchive( is );

        bool isCorrupted = false;

        try
        {
            iarchive( gSettingsVersion );

            if ( gSettingsVersion < MIN_SETTINGS_VERSION || gSettingsVersion > MAX_SETTINGS_VERSION )
            {
                QMessageBox msgBox;
                msgBox.setText( QString::fromStdString( "Your config data has old version" ) );
                msgBox.setInformativeText( "It will be lost" );
                msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
                int reply = msgBox.exec();
                if ( reply == QMessageBox::Cancel )
                {
                    exit(1);
                }

                return false;
            }

            iarchive( m_restBootstrap );
            iarchive( m_replicatorBootstrap );
            iarchive( m_udpPort );
            iarchive( m_feeMultiplier );
            iarchive( m_currentAccountIndex );
            iarchive( m_accounts );

            int top;
            iarchive( top );
            m_windowGeometry.setTop(top);

            int left;
            iarchive( left );
            m_windowGeometry.setLeft(left);

            int width;
            iarchive( width );
            m_windowGeometry.setWidth(width);

            int height;
            iarchive( height );
            m_windowGeometry.setHeight(height);

            iarchive( m_isDriveStructureAsTree );
        }
        catch(...)
        {
            isCorrupted = true;
        }

        if ( isCorrupted || m_accounts.size() <= size_t(m_currentAccountIndex) )
        {
            std::cerr << "Your config data is corrupted" << std::endl;
            std::cerr << m_configPath << std::endl;
            QMessageBox msgBox;
            msgBox.setText( QString::fromStdString( "Your config data is corrupted" ) );
            msgBox.setInformativeText( QString::fromStdString( m_configPath.string() ) );
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            exit(1);
        }

        qDebug() << "/*****************************************************************************************/.";
        qDebug() << "Settings::load. New session has been started.";

        for( auto& account: m_accounts )
        {
            account.initAccount( account.m_accountName, account.m_privateKeyStr );
            qDebug() << "Settings::load. Account keys(private/public):" << account.m_privateKeyStr.c_str() << " / " << account.m_publicKeyStr.c_str();
        }

        qDebug() << "Settings::load. Current account public key: " << QString::fromStdString(config().m_publicKeyStr).toUpper();
        qDebug() << "/*****************************************************************************************/.";
    }
    catch( std::runtime_error& err )
    {
        QMessageBox msgBox;
        msgBox.setText( QString::fromStdString( "Cannot load config data" ) );
        msgBox.setInformativeText( QString::fromStdString( err.what() ) );
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return false;
    }

    m_loaded = true;
    return true;
}

void Settings::save()
{
    std::error_code ec;
    bool isExists = fs::exists( getSettingsFolder().make_preferred(), ec );
    if (ec)
    {
        qCritical () << "Settings::save. fs::exists error: " << ec.message() << " code: " << std::to_string(ec.value());
        return;
    }

    if ( ! isExists )
    {
        fs::create_directories( getSettingsFolder().make_preferred(), ec );
        if ( ec )
        {
            QMessageBox msgBox;
            msgBox.setText( QString::fromStdString( "Cannot create folder: " + getSettingsFolder().string()));
            msgBox.setInformativeText( QString::fromStdString( ec.message() ) );
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();

            qCritical () << "Settings::save. fs::create_directories error: " << ec.message() << " code: " << std::to_string(ec.value())
                         << " path: " << getSettingsFolder().make_preferred().string();

            exit(1);
        }
    }

    ASSERT( m_currentAccountIndex < m_accounts.size() );

    try
    {
        std::ostringstream os( std::ios::binary );
        cereal::PortableBinaryOutputArchive archive( os );
        gSettingsVersion = MAX_SETTINGS_VERSION;
        archive( gSettingsVersion );
        archive( m_restBootstrap );
        archive( m_replicatorBootstrap );
        archive( m_udpPort );
        archive( m_feeMultiplier );
        archive( m_currentAccountIndex );
        archive( m_accounts );
        archive( m_windowGeometry.top() );
        archive( m_windowGeometry.left() );
        archive( m_windowGeometry.width() );
        archive( m_windowGeometry.height() );
        archive( m_isDriveStructureAsTree );

        std::ofstream fStream( m_configPath, std::ios::binary );
        fStream << os.str();
        fStream.close();
    }
    catch( const std::exception& ex )
    {
        QMessageBox msgBox;
        msgBox.setText( QString::fromStdString( "Cannot save settings in file: " + m_configPath.string() ) );
        msgBox.setInformativeText( QString::fromStdString( ec.message() ) );
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        qCritical () << "Settings::save. msgBox error: " << msgBox.text();
        exit(1);
    }

    qDebug() << "Settings::save. Settings saved";
}

bool Settings::loaded() const
{
    return m_loaded;
}

std::vector<std::string> Settings::accountList()
{
    std::vector<std::string> list;
    for( const auto& account : m_accounts )
    {
        list.push_back( account.m_accountName );
    }

    return list;
}

Account& Settings::config()
{
    ASSERT( m_currentAccountIndex >= 0 && m_currentAccountIndex < m_accounts.size() );
    return m_accounts[m_currentAccountIndex];
}

fs::path Settings::downloadFolder()
{
    return config().m_downloadFolder;
}

void Settings::setCurrentAccountIndex( int currentAccountIndex )
{
    qDebug() << "Settings::setCurrentAccountIndex: " << currentAccountIndex;
    m_currentAccountIndex = currentAccountIndex;
    ASSERT( m_currentAccountIndex >= 0 && m_currentAccountIndex < m_accounts.size() )
}

void Settings::onDownloadCompleted( lt::torrent_handle handle )
{
    std::thread( [ this, handle ]
    {
        auto& downloads = config().m_downloads;

        int counter = 0;

        for( auto& dnInfo: downloads )
        {
            if ( dnInfo.getHandle() == handle )
            {
                counter++;
            }
        }

        for( auto& dnInfo: downloads )
        {
            if ( dnInfo.getHandle() == handle )
            {
                fs::path srcPath = fs::path(dnInfo.getDownloadFolder() + "/" + sirius::drive::toString( dnInfo.getHash())).make_preferred();
                fs::path destPath = fs::path(dnInfo.getSaveFolder() + "/" + dnInfo.getFileName()).make_preferred();
                qDebug() << "onDownloadCompleted: counter: " << counter << " " << destPath.c_str();

                std::error_code ec;
                if ( ! fs::exists( destPath.parent_path(), ec ) )
                {
                    fs::create_directories( destPath.parent_path(), ec );
                }

                if ( ec )
                {
                    qWarning() << "onDownloadCompleted: Cannot create destination subfolder: " << ec.message() << " " << srcPath.string().c_str() << " : " << destPath.string().c_str();

                    QString message;
                    message.append("Cannot create destination subfolder: ");
                    message.append(destPath.filename().string().c_str());
                    message.append(" (");
                    message.append(ec.message().c_str());
                    message.append(")");
                    emit downloadError(message);
                }

                int index = 0;
                while( fs::exists(destPath,ec) )
                {
                    index++;
                    auto newName = fs::path(dnInfo.getFileName()).stem().string()
                                    + " (" + std::to_string(index) + ")"
                                    + fs::path(dnInfo.getFileName()).extension().string();
                    destPath = fs::path(dnInfo.getSaveFolder() + "/" + newName).make_preferred();
                    dnInfo.setFileName(newName);
                }

                dnInfo.getFileName() = destPath.filename().string();

                if ( --counter == 0 )
                {
                    qDebug() <<  "onDownloadCompleted: rename(): " << srcPath.string().c_str() << " : " << destPath.string().c_str();
                    fs::rename( srcPath, destPath, ec );
                }
                else
                {
                    qDebug() << "onDownloadCompleted: copy(): " << srcPath.string().c_str() << " : " << destPath.string().c_str();
                    fs::copy( srcPath, destPath, ec );
                }

                if ( ec )
                {
                    qWarning() << "onDownloadCompleted: Cannot save file: " << ec.message() << " " << srcPath.string().c_str() << " : " << destPath.string().c_str();

                    QString message;
                    message.append("Cannot save file: ");
                    message.append(destPath.filename().string().c_str());
                    message.append(" (");
                    message.append(ec.message().c_str());
                    message.append(")");
                    emit downloadError(message);
                }

                dnInfo.setCompleted(true);
            }
        }

    }).detach();
}

void Settings::removeFromDownloads( int index )
{
    auto& downloads = config().m_downloads;

    if ( index < 0 || index >= downloads.size() )
    {
        qDebug() << "Settings::removeFromDownloads. Invalid index: " << index << "; downloads.size()=" << downloads.size();
        return;
    }

    auto& dnInfo = downloads[index];

    int  hashCounter = 0;

    for( auto it = downloads.begin(); it != downloads.end(); it++ )
    {
        if ( it->getHash() == dnInfo.getHash() )
        {
            hashCounter++;
        }
    }

    qDebug() << "Settings::removeFromDownloads. HashCounter: " << hashCounter << " dnInfo.isCompleted()=" << dnInfo.isCompleted();

    if ( hashCounter == 1 )
    {
        gStorageEngine->removeTorrentSync( dnInfo.getHash() );
    }

    // remove file
    std::error_code ec;
    if ( dnInfo.isCompleted() )
    {
        const std::string path = fs::path(downloadFolder().string() + "/" + dnInfo.getFileName()).make_preferred().string();
        fs::remove( path, ec );
        if (ec)
        {
            qCritical () << "Settings::save. fs::remove error: " << ec.message() << " code: " + std::to_string(ec.value()) << " path: " << path;
        }
        else
        {
            qDebug() << "Settings::removeFromDownloads. Remove(1): " << path;
        }
    }
    else if ( hashCounter == 1 )
    {
        const std::string path = fs::path(downloadFolder().string() + "/" + sirius::drive::hashToFileName(dnInfo.getHash())).make_preferred().string();
        fs::remove( path, ec );
        if (ec)
        {
            qCritical () << "Settings::save. fs::remove error: " << ec.message() << " code: " + std::to_string(ec.value()) << " path: " << path;
        }
        else
        {
            qDebug() << "Settings::removeFromDownloads. Remove(2): " << path;
        }
    }

    config().m_downloads.erase( config().m_downloads.begin()+index );
}
