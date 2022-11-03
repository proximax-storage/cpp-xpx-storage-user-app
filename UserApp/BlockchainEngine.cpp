#include "BlockchainEngine.h"

BlockchainEngine::BlockchainEngine(std::shared_ptr<xpx_chain_sdk::IClient> chainClient, QObject *parent)
    : QObject(parent)
    , mpChainClient(chainClient)
    , mpWorker(new Worker)
    , mpThread(new QThread)
{
    qRegisterMetaType<std::array<uint8_t,32>>("std::array<uint8_t,32>");
    qRegisterMetaType<sirius::drive::ActionList>("ActionList");
    qRegisterMetaType<sirius::Key>("sirius_Key");
}

void BlockchainEngine::init(int delay) {
    mpWorker->moveToThread(mpThread);
    connect(mpThread, &QThread::started, mpWorker, [w = mpWorker, delay]() { w->init(delay); }, Qt::QueuedConnection);
    connect(mpWorker, &Worker::done, this, &BlockchainEngine::callbackResolver, Qt::QueuedConnection);
    connect(mpThread, &QThread::finished, mpThread, &QThread::deleteLater);
    connect(mpThread, &QThread::finished, mpWorker, &Worker::deleteLater);
    connect(this, &BlockchainEngine::addResolver, this, &BlockchainEngine::onAddResolver, Qt::QueuedConnection);
    connect(this, &BlockchainEngine::removeResolver, this, &BlockchainEngine::onRemoveResolver, Qt::QueuedConnection);
    connect(this, &BlockchainEngine::runProcess, mpWorker, &Worker::process, Qt::QueuedConnection);
    mpThread->start();
}

void BlockchainEngine::announceNewTransaction(const std::vector<uint8_t> &binaryData,
                                              const std::function<void(bool, bool, std::string, std::string)> &callback) {
    auto task = [this, binaryData]() {
        try {
            auto result = mpChainClient->transactions()->announceNewTransaction(binaryData);
            return QVariant::fromValue(result);
        } catch (const xpx_chain_sdk::InvalidRequest& e) {
            return QVariant::fromValue(e.getErrorMessage());
        } catch (std::exception& e) {
            return QVariant::fromValue(std::string(e.what()));
        }
    };

    const QUuid id = QUuid::createUuid();
    auto resolver = [this, id, callback] (QVariant data) {
        emit removeResolver(id);
        typeResolver<bool>(data, callback);
    };

    emit addResolver(id, resolver);
    emit runProcess(id, task);
}

void BlockchainEngine::getNetworkInfo(const std::function<void(xpx_chain_sdk::NetworkInfo, bool, std::string, std::string)>& callback) {
    auto task = [this]() {
        try {
            auto info = mpChainClient->network()->getNetworkInfo();
            return QVariant::fromValue(info);
        } catch (const xpx_chain_sdk::InvalidRequest& e) {
            return QVariant::fromValue(e.getErrorMessage());
        } catch (std::exception& e) {
            return QVariant::fromValue(std::string(e.what()));
        }
    };

    const QUuid id = QUuid::createUuid();
    auto resolver = [this, id, callback] (QVariant data) {
        emit removeResolver(id);
        typeResolver<xpx_chain_sdk::NetworkInfo>(data, callback);
    };

    emit addResolver(id, resolver);
    emit runProcess(id, task);
}

void BlockchainEngine::getAccountInfo(const std::string &accountPublicKey,
                                      const std::function<void(xpx_chain_sdk::AccountInfo, bool, std::string, std::string)> &callback) {
    auto task = [this, accountPublicKey]() {
        try {
            auto info = mpChainClient->account()->getAccountInfo(accountPublicKey);
            return QVariant::fromValue(info);
        } catch (const xpx_chain_sdk::InvalidRequest& e) {
            return QVariant::fromValue(e.getErrorMessage());
        } catch (std::exception& e) {
            return QVariant::fromValue(std::string(e.what()));
        }
    };

    const QUuid id = QUuid::createUuid();
    auto resolver = [this, id, callback] (QVariant data) {
        emit removeResolver(id);
        typeResolver<xpx_chain_sdk::AccountInfo>(data, callback);
    };

    emit addResolver(id, resolver);
    emit runProcess(id, task);
}

void BlockchainEngine::getBlockchainHeight(const std::function<void(uint64_t, bool, std::string, std::string)> &callback) {
    auto task = [this]() {
        try {
            auto height = mpChainClient->blockchain()->getBlockchainHeight();
            return QVariant::fromValue(height);
        } catch (const xpx_chain_sdk::InvalidRequest& e) {
            return QVariant::fromValue(e.getErrorMessage());
        } catch (std::exception& e) {
            return QVariant::fromValue(std::string(e.what()));
        }
    };

    const QUuid id = QUuid::createUuid();
    auto resolver = [this, id, callback] (QVariant data) {
        emit removeResolver(id);
        typeResolver<uint64_t>(data, callback);
    };

    emit addResolver(id, resolver);
    emit runProcess(id, task);
}

void BlockchainEngine::getDownloadChannelById(const std::string &channelPublicKey,
                                              const std::function<void(xpx_chain_sdk::DownloadChannel, bool, std::string, std::string)> &callback) {
    auto task = [this, channelPublicKey]() {
        try {
            auto channel = mpChainClient->storage()->getDownloadChannelById(channelPublicKey);
            return QVariant::fromValue(channel);
        } catch (const xpx_chain_sdk::InvalidRequest& e) {
            return QVariant::fromValue(e.getErrorMessage());
        } catch (std::exception& e) {
            return QVariant::fromValue(std::string(e.what()));
        }
    };

    const QUuid id = QUuid::createUuid();
    auto resolver = [this, id, callback] (QVariant data) {
        emit removeResolver(id);
        typeResolver<xpx_chain_sdk::DownloadChannel>(data, callback);
    };

    emit addResolver(id, resolver);
    emit runProcess(id, task);
}

void BlockchainEngine::getDownloadChannels(const std::function<void(xpx_chain_sdk::download_channels_page::DownloadChannelsPage,
                                           bool, std::string,std::string)> &callback) {
    auto task = [this]() {
        try {
            auto channelsPage = mpChainClient->storage()->getDownloadChannels();
            return QVariant::fromValue(channelsPage);
        } catch (const xpx_chain_sdk::InvalidRequest& e) {
            return QVariant::fromValue(e.getErrorMessage());
        } catch (std::exception& e) {
            return QVariant::fromValue(std::string(e.what()));
        }
    };

    const QUuid id = QUuid::createUuid();
    auto resolver = [this, id, callback] (QVariant data) {
        emit removeResolver(id);
        typeResolver<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>(data, callback);
    };

    emit addResolver(id, resolver);
    emit runProcess(id, task);
}

void BlockchainEngine::getBlockByHeight(uint64_t height, const std::function<void(xpx_chain_sdk::Block, bool, std::string, std::string)> &callback) {
    auto task = [this, height]() {
        try {
            auto block = mpChainClient->blockchain()->getBlockByHeight(height);
            return QVariant::fromValue(block);
        } catch (const xpx_chain_sdk::InvalidRequest& e) {
            return QVariant::fromValue(e.getErrorMessage());
        } catch (std::exception& e) {
            return QVariant::fromValue(std::string(e.what()));
        }
    };

    const QUuid id = QUuid::createUuid();
    auto resolver = [this, id, callback] (QVariant data) {
        emit removeResolver(id);
        typeResolver<xpx_chain_sdk::Block>(data, callback);
    };

    emit addResolver(id, resolver);
    emit runProcess(id, task);
}

void BlockchainEngine::getDriveById(const std::string &drivePublicKey,
                                    const std::function<void(xpx_chain_sdk::Drive, bool, std::string, std::string)> &callback) {
    auto task = [this, drivePublicKey]() {
        try {
            auto block = mpChainClient->storage()->getDriveById(drivePublicKey);
            return QVariant::fromValue(block);
        } catch (const xpx_chain_sdk::InvalidRequest& e) {
            return QVariant::fromValue(e.getErrorMessage());
        } catch (std::exception& e) {
            return QVariant::fromValue(std::string(e.what()));
        }
    };

    const QUuid id = QUuid::createUuid();
    auto resolver = [this, id, callback] (QVariant data) {
        emit removeResolver(id);
        typeResolver<xpx_chain_sdk::Drive>(data, callback);
    };

    emit addResolver(id, resolver);
    emit runProcess(id, task);
}

void BlockchainEngine::getDrives(const std::function<void(xpx_chain_sdk::drives_page::DrivesPage drivesPage, bool, std::string, std::string)> &callback) {
    auto task = [this]() {
        try {
            auto drives = mpChainClient->storage()->getDrives();
            return QVariant::fromValue(drives);
        } catch (const xpx_chain_sdk::InvalidRequest& e) {
            return QVariant::fromValue(e.getErrorMessage());
        } catch (std::exception& e) {
            return QVariant::fromValue(std::string(e.what()));
        }
    };

    const QUuid id = QUuid::createUuid();
    auto resolver = [this, id, callback] (QVariant data) {
        emit removeResolver(id);
        typeResolver<xpx_chain_sdk::drives_page::DrivesPage>(data, callback);
    };

    emit addResolver(id, resolver);
    emit runProcess(id, task);
}

void BlockchainEngine::getTransactionInfo(xpx_chain_sdk::TransactionGroup group,
                                          const std::string &transactionId,
                                          const std::function<void(std::shared_ptr<xpx_chain_sdk::transactions_info::BasicTransaction>, bool, std::string,std::string)> &callback) {
    auto task = [this, group, transactionId]() {
        try {
            auto info = mpChainClient->transactions()->getTransactionInfo(group, transactionId);
            return QVariant::fromValue(info);
        } catch (const xpx_chain_sdk::InvalidRequest& e) {
            return QVariant::fromValue(e.getErrorMessage());
        } catch (std::exception& e) {
            return QVariant::fromValue(std::string(e.what()));
        }
    };

    const QUuid id = QUuid::createUuid();
    auto resolver = [this, id, callback] (QVariant data) {
        emit removeResolver(id);
        typeResolver<std::shared_ptr<xpx_chain_sdk::transactions_info::BasicTransaction>>(data, callback);
    };

    emit addResolver(id, resolver);
    emit runProcess(id, task);
}

void BlockchainEngine::getReplicatorById(
        const std::string &replicatorPublicKey,
        const std::function<void(xpx_chain_sdk::Replicator, bool, std::string, std::string)>& callback)
{
    auto task = [this, replicatorPublicKey]() {
        try {
            auto replicator = mpChainClient->storage()->getReplicatorById(replicatorPublicKey);
            return QVariant::fromValue(replicator);
        } catch (const xpx_chain_sdk::InvalidRequest& e) {
            return QVariant::fromValue(e.getErrorMessage());
        } catch (std::exception& e) {
            return QVariant::fromValue(std::string(e.what()));
        }
    };

    const QUuid id = QUuid::createUuid();
    auto resolver = [this, id, callback] (QVariant data) {
        emit removeResolver(id);
        typeResolver<xpx_chain_sdk::Replicator>(data, callback);
    };

    emit addResolver(id, resolver);
    emit runProcess(id, task);
}

void BlockchainEngine::onAddResolver(const QUuid &id, const std::function<void(QVariant)>& resolver) {
    mResolvers.emplace(id, resolver);
}

void BlockchainEngine::onRemoveResolver(const QUuid &id) {
    mResolvers.erase(id);
}

void BlockchainEngine::callbackResolver(const QUuid& id, const QVariant& data) {
    mResolvers[id](data);
}

BlockchainEngine::~BlockchainEngine()
{
    if (mpThread) {
        mpThread->deleteLater();
    }

    if (mpWorker) {
        mpWorker->deleteLater();
    }
}
