//#include "moc_Settings.cpp"

#include "Settings.h"
#include "StorageEngine.h"
//#include "Diff.h"
#include "drive/Utils.h"
#include <fstream>
#include <filesystem>

#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <cereal/archives/portable_binary.hpp>

#include <QDebug>
#include <QMessageBox>

namespace fs = std::filesystem;

fs::path settingsFolder()
{
    fs::path path;
#ifdef _WIN32
    qDebug() << LOG_SOURCE << "!NOT IMPLEMNTED! FOR WIN32: settingsPath()";
    exit(1);
#else
    const char* homePath = getenv("HOME");
    path = std::string(homePath) + "/.XpxSiriusStorageClient";
#endif

    std::error_code ec;
    if ( ! fs::exists( path, ec ) )
    {
        fs::create_directories( path, ec );
        if ( ec )
        {
            QMessageBox msgBox;
            msgBox.setText( QString::fromStdString( "Cannot create folder: " + std::string(path) ) );
            msgBox.setInformativeText( QString::fromStdString( ec.message() ) );
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            exit(1);
        }
    }

    return path;
}

Settings::Account::Account() {}

Settings::Account::Account( const Account& a )
{
    *this = a;
}

Settings::Account& Settings::Account::operator=( const Account& a )
{
    initAccount( a.m_accountName, a.m_privateKeyStr );

    m_dnChannels            = a.m_dnChannels;
    m_currentDnChannelIndex = a.m_currentDnChannelIndex;
    m_downloadFolder        = a.m_downloadFolder;
    m_downloads             = a.m_downloads;
    m_drives                = a.m_drives;
    m_currentDriveIndex     = a.m_currentDriveIndex;

    return *this;
}

void Settings::initForTests()
{
    if ( Model::homeFolder() == "/Users/alex" )
    {
        gSettings.m_restBootstrap       = "54.151.169.225:3000";

        m_accounts.push_back({});
        setCurrentAccountIndex( m_accounts.size()-1 );
        config().initAccount( "test_genkins", "fd59b9e34bc07f59f5a05f9bd550e6186d483a264269554fd163f53298dfcbe4" );
        config().m_downloadFolder = "/Users/alex/000-Downloads";

        m_accounts.push_back({});
        setCurrentAccountIndex( m_accounts.size()-1 );
        config().initAccount( "test_genkins_2", "b04fbd908d0fd4315efc4e970bb899d5f82f3828256b7b1f3d4782fef02820e1" );
        config().m_downloadFolder = "/Users/alex/000-Downloads";

        m_accounts.push_back({});
        setCurrentAccountIndex( m_accounts.size()-1 );
        config().initAccount( "alex_local_test", "0000000000010203040501020304050102030405010203040501020304050102" );
        config().m_downloadFolder = "/Users/alex/000-Downloads";
    }
}

bool Settings::load( const std::string& pwd )
{
    try
    {
        fs::path filePath = settingsFolder() / "config";

        if ( ! fs::exists( filePath ) )
        {
            return false;
        }

        std::ifstream ifStream( filePath, std::ios::binary );

        std::ostringstream os;
        os << ifStream.rdbuf();

        std::istringstream is( os.str(), std::ios::binary );
        cereal::PortableBinaryInputArchive iarchive( is );

        bool isCorrupted = false;

        try
        {
            uint32_t settingsVersion;
            iarchive( settingsVersion );

            if ( settingsVersion != m_settingsVersion )
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
            iarchive( m_currentAccountIndex );
            iarchive( m_accounts );
        }
        catch(...)
        {
            isCorrupted = true;
        }

        if ( isCorrupted || m_accounts.size() <= size_t(m_currentAccountIndex) )
        {
            std::cerr << "Your config data is corrupted" << std::endl;
            std::cerr << filePath << std::endl;
            QMessageBox msgBox;
            msgBox.setText( QString::fromStdString( "Your config data is corrupted" ) );
            msgBox.setInformativeText( QString::fromStdString( filePath ) );
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            exit(1);
        }

        for( auto& account: m_accounts )
        {
            account.initAccount( account.m_accountName, account.m_privateKeyStr );
            qDebug() << LOG_SOURCE << "account keys(private/public):" << account.m_privateKeyStr.c_str() << " / " << account.m_publicKeyStr.c_str();
        }

        qDebug() << LOG_SOURCE << "currentAccountKeyStr: " << config().m_publicKeyStr.c_str();
        qDebug() << LOG_SOURCE << "currentAccountIndex: " << m_currentAccountIndex;
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
    if ( ! fs::exists( settingsFolder(), ec ) )
    {
        fs::create_directories( settingsFolder(), ec );
        if ( ec )
        {
            QMessageBox msgBox;
            msgBox.setText( QString::fromStdString( "Cannot create folder: " + std::string(settingsFolder()) ) );
            msgBox.setInformativeText( QString::fromStdString( ec.message() ) );
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            exit(1);
        }
    }

    assert( m_currentAccountIndex < m_accounts.size() );

    try
    {
        std::ostringstream os( std::ios::binary );
        cereal::PortableBinaryOutputArchive archive( os );
        archive( m_settingsVersion );
        archive( m_restBootstrap );
        archive( m_replicatorBootstrap );
        archive( m_udpPort );
        archive( m_currentAccountIndex );
        archive( m_accounts );

        std::ofstream fStream( settingsFolder() / "config", std::ios::binary );
        fStream << os.str();
        fStream.close();
    }
    catch( const std::exception& ex )
    {
        QMessageBox msgBox;
        msgBox.setText( QString::fromStdString( "Cannot save settings in file: " + std::string(settingsFolder() / "config") ) );
        msgBox.setInformativeText( QString::fromStdString( ec.message() ) );
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        exit(1);
    }

    qDebug() << LOG_SOURCE << "Settings saved";
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

void Settings::onDownloadCompleted( lt::torrent_handle handle )
{
    std::thread( [ this, handle ]
    {
        qDebug() << LOG_SOURCE << "onDownloadCompleted: before lock";
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

        auto& downloads = config().m_downloads;

        int counter = 0;

        for( auto& dnInfo: downloads )
        {
            if ( dnInfo.m_ltHandle == handle )
            {
                counter++;
            }
        }
        qDebug() << LOG_SOURCE << "onDownloadCompleted: counter: " << counter;

        for( auto& dnInfo: downloads )
        {
            if ( dnInfo.m_ltHandle == handle )
            {
                fs::path srcPath = fs::path(dnInfo.m_saveFolder) / sirius::drive::toString( dnInfo.m_hash );
                fs::path destPath = fs::path(dnInfo.m_saveFolder) / dnInfo.m_fileName;

                std::error_code ec;

                int index = 0;
                while( fs::exists(destPath,ec) )
                {
                    index++;
                    auto newName = fs::path(dnInfo.m_fileName).stem().string()
                                    + " (" + std::to_string(index) + ")"
                                    + fs::path(dnInfo.m_fileName).extension().string();
                    destPath = fs::path(dnInfo.m_saveFolder) / newName;
                }

                dnInfo.m_fileName = destPath.filename();

                if ( --counter == 0 )
                {
                    qDebug() << LOG_SOURCE << "onDownloadCompleted: rename(): " << srcPath << " : " << destPath;
                    fs::rename( srcPath, destPath, ec );
                }
                else
                {
                    qDebug() << LOG_SOURCE << "onDownloadCompleted: copy(): " << srcPath << " : " << destPath;
                    fs::copy( srcPath, destPath, ec );
                }

                if ( ec )
                {
                    qDebug() << LOG_SOURCE << "onDownloadCompleted: Cannot save file: " << ec.message();

                    QMessageBox msgBox;
                    msgBox.setText( QString::fromStdString( "Cannot save file: " + std::string(destPath.filename()) ) );
                    msgBox.setInformativeText( QString::fromStdString( ec.message() ) );
                    msgBox.setStandardButtons(QMessageBox::Ok);
                    msgBox.exec();
                }

                dnInfo.m_isCompleted = true;
            }
        }

    }).detach();
}

void Settings::removeFromDownloads( int index )
{
    std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );

    auto& downloads = config().m_downloads;

    if ( index < 0 || index >= downloads.size() )
    {
        qDebug() << LOG_SOURCE << "removeFromDownloads: invalid index: " << index << "; downloads.size()=" << downloads.size();
        return;
    }

    auto& dnInfo = downloads[index];

    int  hashCounter = 0;

    for( auto it = downloads.begin(); it != downloads.end(); it++ )
    {
        if ( it->m_hash == dnInfo.m_hash )
        {
            hashCounter++;
        }
    }

    qDebug() << LOG_SOURCE << "hashCounter: " << hashCounter << " dnInfo.isCompleted()=" << dnInfo.isCompleted();

    if ( hashCounter == 1 )
    {
        gStorageEngine->removeTorrentSync( dnInfo.m_hash );
    }

    // remove file
    std::error_code ec;
    if ( dnInfo.isCompleted() )
    {
        fs::remove( downloadFolder() / dnInfo.m_fileName, ec );
        qDebug() << LOG_SOURCE << "remove: " << (downloadFolder() / dnInfo.m_fileName).string().c_str() << " ec=" << ec.message().c_str();
    }
    else if ( hashCounter == 1 )
    {
        fs::remove( downloadFolder() / sirius::drive::hashToFileName(dnInfo.m_hash), ec );
        qDebug() << LOG_SOURCE << "remove: " << (downloadFolder() / sirius::drive::hashToFileName(dnInfo.m_hash)).string().c_str() << " ec=" << ec.message().c_str();
    }

    config().m_downloads.erase( config().m_downloads.begin()+index );
}
