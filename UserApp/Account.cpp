#include "Account.h"

Account::Account(QObject* parent)
    : QObject(parent)
{}

Account::~Account()
{}

Account::Account( const Account& a )
{
    *this = a;
}

Account& Account::operator=( const Account& a )
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

void Account::initAccount( std::string name, std::string privateKeyStr )
{
    m_accountName   = name;
    m_privateKeyStr = privateKeyStr;

    if ( m_privateKeyStr.size() == 64 )
    {
        m_keyPair =
                sirius::crypto::KeyPair::FromPrivate(
                        sirius::crypto::PrivateKey::FromString( m_privateKeyStr ));

        m_publicKeyStr = sirius::drive::toString( m_keyPair->publicKey().array() );
    }
}