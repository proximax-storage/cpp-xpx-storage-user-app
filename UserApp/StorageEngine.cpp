#include "StorageEngine.h"
#include "Settings.h"
#include "drive/ClientSession.h"
#include "drive/Session.h"

#include <boost/algorithm/string.hpp>

void StorageEngine::start()
{
//        endpoint_list bootstraps;
//        std::vector<std::string> addressAndPort;
//        boost::split( addressAndPort, gSettings.config().m_replicatorBootstrap, [](char c){ return c==':'; } );
//        bootstraps.push_back( { boost::asio::ip::make_address(addressAndPort[0]),
//                               (uint16_t)atoi(addressAndPort[1].c_str()) } );

//        gStorageEngine->initClientSession( *gSettings.config().m_keyPair,
//                                           gSettings.config().m_udpPort,
//                                           bootstraps );

}

void StorageEngine::restart()
{
    //todo
}

void StorageEngine::initClientSession(  const sirius::crypto::KeyPair&  keyPair,
                                        const std::string&              address,
                                        const endpoint_list&            bootstraps )
{

    m_session = sirius::drive::createClientSession(  keyPair,
                                                     address,
                                                     []( const lt::alert* ) {},
                                                     bootstraps,
                                                     false,
                                                     "client_session" );

}
