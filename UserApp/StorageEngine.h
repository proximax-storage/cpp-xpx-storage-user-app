#ifndef STORAGEENGINE_H
#define STORAGEENGINE_H

#include <memory>
#include <string>
#include "crypto/KeyPair.h"

namespace sirius { namespace drive {
    class ClientSession;
}}

using  endpoint_list = std::vector<boost::asio::ip::tcp::endpoint>;

class StorageEngine
{
public:
    StorageEngine() {}

    void start();

    void restart();

private:
    void initClientSession( const sirius::crypto::KeyPair&  keyPair,
                            const std::string&              address,
                            const endpoint_list&            bootstraps );

    std::shared_ptr<sirius::drive::ClientSession> m_session;
};

inline std::shared_ptr<StorageEngine> gStorageEngine;

#endif // STORAGEENGINE_H
