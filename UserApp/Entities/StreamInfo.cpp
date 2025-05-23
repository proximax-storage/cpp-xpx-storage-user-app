#include "StreamInfo.h"

#include <sstream>

static int charToInt( char input )
{
    if ( input >= 'A' && input <= 'P')
    {
        return input - 'A';
    }
    throw std::invalid_argument("Invalid input string");
}


QString StreamInfo::getLink() const
{
    std::ostringstream os( std::ios::binary );
    cereal::PortableBinaryOutputArchive archive( os );

    archive(m_version);
    archive(m_driveKey);
    archive(m_title);
    archive(m_secsSinceEpoch);
    archive(m_uniqueFolderName);

    auto rawString = os.str();
    
    qDebug() << "rawString.size: " << rawString.size();

    std::vector<char> link;
    link.reserve( rawString.size()*2 );
    for( char c : rawString )
    {
        uint8_t b1 = (*((uint8_t*)&c))&0x0F;
        uint8_t b2 = (*((uint8_t*)&c))>>4;
//        qDebug() << "c b1 b2: " << (*((uint8_t*)&c)) << " " << b1 << " " << b2;
        static char table[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P' };

        assert( b1 < sizeof(table) && b2 < sizeof(table) );
        link.push_back( table[b1] );
        link.push_back( table[b2] );
    }

    qDebug() << "link: " << QString::fromStdString( std::string( &link[0], link.size() ) );

    const std::string buffer(&link[0], link.size());
    return stdStringToQStringUtf8(buffer);
}

void StreamInfo::parseLink( const QString& linkString )
{
    qDebug() << "StreamInfo::parseLink: linkString: " << linkString;

    std::vector<uint8_t> rawLink;
    rawLink.reserve( linkString.size()/2+1 );
    for( size_t i=0; i<linkString.size()-1;)
    {
        uint8_t b1 = charToInt(linkString[i].toLatin1());
//        qDebug() << "c1: " << linkString[i];
        i++;
        uint8_t b2 = charToInt(linkString[i].toLatin1()) << 4;
//        qDebug() << "c2: " << linkString[i];
        i++;
        rawLink.push_back(b1|b2);
//        qDebug() << "rawLink.size: " << b2 << " " << b1;
    }
//    qDebug() << "rawLink.size: " << rawLink.size();

    std::string linkStr( (char*)&rawLink[0], rawLink.size() );

    std::istringstream is( linkStr, std::ios::binary );
    cereal::PortableBinaryInputArchive iarchive( is );

    iarchive(m_version );
    iarchive(m_driveKey);
    iarchive(m_title);
    iarchive( m_secsSinceEpoch );
    iarchive(m_uniqueFolderName);
}



