#include "CachedReplicator.h"
#include "utils/HexParser.h"

CachedReplicator::CachedReplicator()
    : m_name("")
    , m_publicKey("")
    , m_privateKey("")
{}

CachedReplicator::~CachedReplicator()
{}

std::string CachedReplicator::getName() const {
    return m_name;
}

void CachedReplicator::setName(const std::string& name) {
    m_name = name;
}

std::string CachedReplicator::getPublicKey() const {
    return m_publicKey;
}

void CachedReplicator::setPublicKey(const std::string& key) {
    m_publicKey = key;
}

std::string CachedReplicator::getPrivateKey() const {
    return m_privateKey;
}

void CachedReplicator::setPrivateKey(const std::string& key) {
    m_privateKey = key;
}