#pragma once

#include <string>
#include <cereal/types/chrono.hpp>

class CachedReplicator {
    public:
        CachedReplicator();
        ~CachedReplicator();

    public:
        std::string getName() const;
        void setName(const std::string& name);
        std::string getPublicKey() const;
        void setPublicKey(const std::string& key);
        std::string getPrivateKey() const;
        void setPrivateKey(const std::string& key);

        template<class Archive>
        void serialize( Archive &ar )
        {
            ar( m_name,
                m_publicKey,
                m_privateKey );
        }

    private:
        std::string m_name;
        std::string m_publicKey;
        std::string m_privateKey;
};
