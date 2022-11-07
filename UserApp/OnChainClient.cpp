#include "OnChainClient.h"
#include <QDebug>

OnChainClient::OnChainClient(std::shared_ptr<StorageEngine> storage,
                             const std::string& privateKey,
                             const std::string& address,
                             const std::string& port,
                             QObject* parent)
    : QObject(parent)
    , mpStorageEngine(storage)
{
    init(address, port, privateKey);
}

void OnChainClient::loadDrives() {
    mpBlockchainEngine->getDrives([this](auto drivesPage, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            qWarning() << LOG_SOURCE << message.c_str() << " : " << code.c_str();
            return;
        }

        auto publicKey = rawHashToHex(mpChainAccount->publicKey());

        QStringList loadedDrives;
        for (const auto &drive: drivesPage.data.drives) {
            if (drive.data.owner == publicKey.toStdString()) {
                loadedDrives.push_back(drive.data.multisig.c_str());
            }
        }

        if (!loadedDrives.empty()) {
            emit drivesLoaded(loadedDrives);
        }
    });
}

void OnChainClient::loadDownloadChannels(const QString& drivePubKey) {
    mpBlockchainEngine->getDownloadChannels([this, drivePubKey](auto channelsPage, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            qWarning() << LOG_SOURCE << message.c_str() << " : " << code.c_str();
            return;
        }

        QStringList loadedChannels;
        for (const auto &channel: channelsPage.data.channels) {
            if (channel.data.drive == drivePubKey.toStdString()) {
                loadedChannels.push_back(channel.data.id.c_str());
            }
        }

        if (!loadedChannels.empty()) {
            emit downloadChannelsLoaded(loadedChannels);
        }
    });
}

std::shared_ptr<BlockchainEngine> OnChainClient::getBlockchainEngine() {
    return mpBlockchainEngine;
}

std::string OnChainClient::addDownloadChannel(const std::string& channelAlias,
                                       const std::vector<std::array<uint8_t, 32>> &listOfAllowedPublicKeys,
                                       const std::array<uint8_t, 32> &drivePubKey, const uint64_t &prepaidSize,
                                       const uint64_t &feedbacksNumber) {
    return mpTransactionsEngine->addDownloadChannel(channelAlias, listOfAllowedPublicKeys, drivePubKey, prepaidSize, feedbacksNumber);
}

void OnChainClient::closeDownloadChannel(const std::array<uint8_t, 32> &channelId) {
    mpTransactionsEngine->closeDownloadChannel(channelId);
}

void OnChainClient::addDrive(const std::string &driveAlias, const uint64_t &driveSize, ushort replicatorsCount) {
    mpTransactionsEngine->addDrive(driveAlias, driveSize, replicatorsCount);
}

void OnChainClient::closeDrive(const std::array<uint8_t, 32> &driveKey) {
    mpTransactionsEngine->closeDrive(driveKey);
}

void OnChainClient::applyDataModification(const std::array<uint8_t, 32> &driveId,
                                          const sirius::drive::ActionList &actions,
                                          const std::array<uint8_t, 32> &channelId,
                                          const std::string &sandboxFolder) {
    mpBlockchainEngine->getDriveById(rawHashToHex(driveId).toStdString(),
                                     [this, driveId, actions, channelId, sandboxFolder]
                                     (auto drive, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            qWarning() << LOG_SOURCE << "message: " << message.c_str() << " code: " << code.c_str();
            return;
        }

        if (drive.data.replicators.empty()) {
            qWarning() << LOG_SOURCE << "empty replicators list received for the drive: " << drive.data.multisig.c_str();
            return;
        }

        std::vector<xpx_chain_sdk::Address> addresses;
        addresses.reserve(drive.data.replicators.size());
        for (const auto& replicator : drive.data.replicators) {
            xpx_chain_sdk::Key key;
            xpx_chain_sdk::ParseHexStringIntoContainer(replicator.c_str(), replicator.size(), key);
            addresses.emplace_back(key);
        }

        mpTransactionsEngine->applyDataModification(driveId, actions, channelId, sandboxFolder, addresses);
    });
}

void OnChainClient::initConnects() {
    connect(mpTransactionsEngine.get(), &TransactionsEngine::createDownloadChannelConfirmed, this, [this](auto alias, auto channelId, auto driveKey) {
        emit downloadChannelOpenTransactionConfirmed(alias, channelId, driveKey);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::createDownloadChannelFailed, this, [this](auto channelId, auto errorText) {
        emit downloadChannelOpenTransactionFailed(channelId, errorText);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::closeDownloadChannelConfirmed, this, [this](auto channelId) {
        emit downloadChannelCloseTransactionConfirmed(channelId);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::closeDownloadChannelFailed, this, [this](auto errorText) {
        emit downloadChannelCloseTransactionFailed(errorText);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::createDriveConfirmed, this, [this](auto alias, auto driveId) {
        emit prepareDriveTransactionConfirmed(alias, driveId);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::createDriveFailed, this, [this](auto alias, auto driveKey, auto errorText) {
        emit prepareDriveTransactionFailed(alias, driveKey, errorText);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::closeDriveConfirmed, this, [this](auto driveId) {
        emit closeDriveTransactionConfirmed(driveId);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::closeDriveFailed, this, [this](auto errorText) {
        emit closeDriveTransactionFailed(errorText);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::addActions, this, [this](auto actionList, auto driveId, auto sandboxFolder, auto callback) {
        uint64_t modificationsSize = 0;
        auto hash = mpStorageEngine->addActions(actionList, driveId,  sandboxFolder, modificationsSize);
        callback(modificationsSize, hash.array());
    }, Qt::QueuedConnection);

    connect(mpTransactionsEngine.get(), &TransactionsEngine::dataModificationConfirmed, this, [this](auto modificationId) {
        emit dataModificationTransactionConfirmed(modificationId);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::dataModificationFailed, this, [this](auto modificationId) {
        emit dataModificationTransactionFailed(modificationId);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::dataModificationApprovalConfirmed, this,
            [this](auto driveId, auto channelId, auto fileStructureCdi) {
                auto callback = [this](const std::string& driveId, const std::array<uint8_t, 32>& fsTreeHash, const sirius::drive::FsTree& fsTree) {
                    emit fsTreeDownloaded(driveId, fsTreeHash, fsTree);
                };

                mpStorageEngine->downloadFsTree(rawHashToHex(driveId).toStdString(),
                                                rawHashToHex(channelId).toStdString(),
                                                rawHashFromHex(fileStructureCdi.c_str()), callback);
            }, Qt::QueuedConnection);
}

void OnChainClient::initAccount(const std::string &privateKey) {
    mpChainAccount = std::make_shared<xpx_chain_sdk::Account>([privateKey](
            xpx_chain_sdk::PrivateKeySupplierReason reason,
            xpx_chain_sdk::PrivateKeySupplierParam param)
    {
        xpx_chain_sdk::Key key;
        ParseHexStringIntoContainer(privateKey.c_str(), privateKey.size(), key);
        return xpx_chain_sdk::PrivateKey(key.data(), key.size());
    }, mpChainClient->getConfig()->NetworkId);
}

void OnChainClient::init(const std::string& address,
                         const std::string& port,
                         const std::string& privateKey) {
    auto config = std::shared_ptr<xpx_chain_sdk::Config>(&xpx_chain_sdk::GetConfig());
    config->nodeAddress = address;
    config->port = port;

    mpChainClient = xpx_chain_sdk::getClient(config);
    mpBlockchainEngine = std::make_shared<BlockchainEngine>(mpChainClient);
    mpBlockchainEngine->init(80); // 1000 milliseconds / 15 request per second
    mpBlockchainEngine->getNetworkInfo([this, config, privateKey](auto info, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            qWarning() << LOG_SOURCE << __FILE__ << "message: " << message.c_str() << " code: " << code.c_str();
            return;
        }

        if ("mijin" == info.name) {
            config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Mijin;
        } else if ("mijinTest" == info.name) {
            config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Mijin_Test;
        } else if ("private" == info.name) {
            config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Private;
        } else if ("privateTest" == info.name) {
            config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Private_Test;
        } else if ("public" == info.name) {
            config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Public;
        } else if ("publicTest" == info.name) {
            config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Public_Test;
        } else {
            config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Zero;
        }

        mpBlockchainEngine->getBlockByHeight(1, [this, config, privateKey](auto block, auto isSuccess, auto message, auto code) {
            if (!isSuccess) {
                qWarning() << LOG_SOURCE << "message: " << message.c_str() << " code: " << code.c_str();
                return;
            }

            config->GenerationHash = block.meta.generationHash;
            initAccount(privateKey);
            mpTransactionsEngine = std::make_shared<TransactionsEngine>(mpChainClient, mpChainAccount, mpBlockchainEngine);
            initConnects();
        });
    });
}