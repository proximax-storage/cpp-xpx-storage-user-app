#include "Account.h"

Account::Account()
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

    m_dnChannels                = a.m_dnChannels;
    m_currentDownloadChannelKey = a.m_currentDownloadChannelKey;
    m_downloadFolder            = a.m_downloadFolder;
    m_downloads                 = a.m_downloads;
    m_drives                    = a.m_drives;
    m_currentDriveKey           = a.m_currentDriveKey;
    m_myReplicators             = a.m_myReplicators;

    return *this;
}

void Account::initAccount( const QString& name, const QString& privateKeyStr )
{
    m_accountName   = name;
    m_privateKeyStr = privateKeyStr;

    if ( m_privateKeyStr.size() == 64 )
    {
        const auto pKeyUTF8 = qStringToStdStringUTF8(m_privateKeyStr);
        m_keyPair = sirius::crypto::KeyPair::FromPrivate(sirius::crypto::PrivateKey::FromString(pKeyUTF8));

        const auto pubKey = sirius::drive::toString( m_keyPair->publicKey().array() );
        m_publicKeyStr = stdStringToQStringUtf8(pubKey);
    }
}
