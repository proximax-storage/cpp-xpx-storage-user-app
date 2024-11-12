#include "CachedReplicator.h"

CachedReplicator::CachedReplicator()
    : m_name("")
    , m_publicKey("")
    , m_privateKey("")
{}

CachedReplicator::~CachedReplicator()
{}

QString CachedReplicator::getName() const {
    return m_name;
}

void CachedReplicator::setName(const QString& name) {
    m_name = name;
}

QString CachedReplicator::getPublicKey() const {
    return m_publicKey;
}

void CachedReplicator::setPublicKey(const QString& key) {
    m_publicKey = key;
}

QString CachedReplicator::getPrivateKey() const {
    return m_privateKey;
}

void CachedReplicator::setPrivateKey(const QString& key) {
    m_privateKey = key;
}