#pragma once

#include <QString>
#include <cereal/types/chrono.hpp>
#include "Utils.h"

class CachedReplicator {
    public:
        CachedReplicator();
        ~CachedReplicator();

    public:
        QString getName() const;
        void setName(const QString& name);
        QString getPublicKey() const;
        void setPublicKey(const QString& key);
        QString getPrivateKey() const;
        void setPrivateKey(const QString& key);

        template<class Archive>
        void serialize( Archive &ar )
        {
            ar(m_name, m_publicKey, m_privateKey);
        }

    private:
        QString m_name;
        QString m_publicKey;
        QString m_privateKey;
};
