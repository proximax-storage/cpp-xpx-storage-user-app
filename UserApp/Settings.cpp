//#include "moc_Settings.cpp"

#include "Settings.h"
#include "Model.h"
#include "StorageEngine.h"
#include "drive/Utils.h"
#include <fstream>
#include <filesystem>

#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/json.hpp>

#include <QDebug>
#include <QMessageBox>

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_isDriveStructureAsTree(false)
{}

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
    m_settingsVersion = s.m_settingsVersion;
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
    m_settingsVersion = s.m_settingsVersion;
    m_accounts = s.m_accounts;
    m_currentAccountIndex = s.m_currentAccountIndex;
    m_loaded = s.m_loaded;

    return *this;
}

void Settings::initForTests()
{
    if ( Model::homeFolder() == "/Users/alex" )
    {
        m_restBootstrap       = "54.151.169.225:3000";

        m_accounts.emplace_back();
        setCurrentAccountIndex( (int)m_accounts.size() - 1 );
        config().initAccount( "test_genkins", "fd59b9e34bc07f59f5a05f9bd550e6186d483a264269554fd163f53298dfcbe4" );
        config().m_downloadFolder = "/Users/alex/000-Downloads";

        m_accounts.emplace_back();
        setCurrentAccountIndex( (int)m_accounts.size() - 1 );
        config().initAccount( "test", "4DC8F4C8C84ED4A829E79A685A98E4A0BB97A3C6DA9C49FA83CA133676993D08" );
        config().m_downloadFolder = "/Users/alex/000-Downloads";

        m_accounts.emplace_back();
        setCurrentAccountIndex( (int)m_accounts.size() - 1 );
        config().initAccount( "alex_local_test", "0000000000010203040501020304050102030405010203040501020304050102" );
        config().m_downloadFolder = "/Users/alex/000-Downloads";

        if ( ! ALEX_LOCAL_TEST )
        {
            setCurrentAccountIndex( 1 );
        }
    }
}

bool Settings::load( const std::string& pwd )
{
    try
    {
        fs::path filePath = getSettingsFolder() / "config";

        if ( ! fs::exists( filePath ) )
        {
            return false;
        }

        std::ifstream ifStream( filePath, std::ios::binary );

        std::ostringstream os;
        os << ifStream.rdbuf();

        std::istringstream is( os.str(), std::ios::binary );
        cereal::PortableBinaryInputArchive iarchive( is );
        //cereal::XMLInputArchive iarchive( is );

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
    if ( ! fs::exists( getSettingsFolder(), ec ) )
    {
        fs::create_directories( getSettingsFolder(), ec );
        if ( ec )
        {
            QMessageBox msgBox;
            msgBox.setText( QString::fromStdString( "Cannot create folder: " + std::string(getSettingsFolder()) ) );
            msgBox.setInformativeText( QString::fromStdString( ec.message() ) );
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            exit(1);
        }
    }

    ASSERT( m_currentAccountIndex < m_accounts.size() );

    try
    {
        std::ostringstream os( std::ios::binary );
        cereal::PortableBinaryOutputArchive archive( os );
        archive( m_settingsVersion );
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

        std::ofstream fStream( getSettingsFolder() / "config", std::ios::binary );
        fStream << os.str();
        fStream.close();

        if (1)
        {
            std::ostringstream os( std::ios::binary );
            cereal::JSONOutputArchive archive( os );
            archive( m_settingsVersion );
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

            std::ofstream fStream( getSettingsFolder() / "config.json", std::ios::binary );
            fStream << os.str();
            fStream.close();

        }
    }
    catch( const std::exception& ex )
    {
        QMessageBox msgBox;
        msgBox.setText( QString::fromStdString( "Cannot save settings in file: " + std::string(getSettingsFolder() / "config") ) );
        msgBox.setInformativeText( QString::fromStdString( ec.message() ) );
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        exit(1);
    }

    qDebug() << LOG_SOURCE << "Settings saved";
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
                fs::path srcPath = fs::path(dnInfo.getDownloadFolder()) / sirius::drive::toString( dnInfo.getHash() );
                fs::path destPath = fs::path(dnInfo.getSaveFolder()) / dnInfo.getFileName();
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
                    destPath = fs::path(dnInfo.getSaveFolder()) / newName;
                    dnInfo.setFileName(newName);
                }

                dnInfo.getFileName() = destPath.filename();

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
        qDebug() << LOG_SOURCE << "removeFromDownloads: invalid index: " << index << "; downloads.size()=" << downloads.size();
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

    qDebug() << LOG_SOURCE << "hashCounter: " << hashCounter << " dnInfo.isCompleted()=" << dnInfo.isCompleted();

    if ( hashCounter == 1 )
    {
        gStorageEngine->removeTorrentSync( dnInfo.getHash() );
    }

    // remove file
    std::error_code ec;
    if ( dnInfo.isCompleted() )
    {
        fs::remove( downloadFolder() / dnInfo.getFileName(), ec );
        qDebug() << LOG_SOURCE << "remove: " << (downloadFolder() / dnInfo.getFileName()).string().c_str() << " ec=" << ec.message().c_str();
    }
    else if ( hashCounter == 1 )
    {
        fs::remove( downloadFolder() / sirius::drive::hashToFileName(dnInfo.getHash()), ec );
        qDebug() << LOG_SOURCE << "remove: " << (downloadFolder() / sirius::drive::hashToFileName(dnInfo.getHash())).string().c_str() << " ec=" << ec.message().c_str();
    }

    config().m_downloads.erase( config().m_downloads.begin()+index );
}
