#include "Settings.h"
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

static fs::path settingsFolder()
{
    fs::path path;
#ifdef _WIN32
    qDebug() << "!NOT IMPLEMNTED! FOR WIN32: settingsPath()";
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

        iarchive( m_currentAccountIndex );
        iarchive( m_accounts );

        if ( m_accounts.size() <= size_t(m_currentAccountIndex) )
        {
            QMessageBox msgBox;
            msgBox.setText( QString::fromStdString( "Your config data is corrupted" ) );
            msgBox.setInformativeText( QString::fromStdString( filePath ) );
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            exit(1);
        }

        for( auto& account: m_accounts )
        {
            account.updateKeyPair( account.m_privateKeyStr );
            qDebug() << "account keys(private/public):" << account.m_privateKeyStr.c_str() << " / " << account.m_publicKeyStr.c_str();
        }

        qDebug() << "currentAccountKeyStr: " << config().m_publicKeyStr.c_str();
    }
    catch( std::runtime_error& )
    {
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

    qDebug() << "Settings saved";
}

std::vector<std::string> Settings::accountList()
{
    std::vector<std::string> list;

    for( const auto& account : m_accounts )
    {
        list.push_back( account.m_publicKeyStr );
    }
    return list;
}
