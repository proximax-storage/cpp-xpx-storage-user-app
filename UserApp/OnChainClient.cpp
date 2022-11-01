#include "OnChainClient.h"
#include <QDebug>

OnChainClient::OnChainClient(const std::string& privateKey, const std::string& address, const std::string& port,  QObject* parent)
    : QObject(parent)
{
    // API node
    auto config = std::shared_ptr<xpx_chain_sdk::Config>(&xpx_chain_sdk::GetConfig());
    config->nodeAddress = address;
    config->port = port;

    auto tmpChainSdkClient = xpx_chain_sdk::getClient(config);
    auto networkName = tmpChainSdkClient->network()->getNetworkInfo().name;
    config->GenerationHash = tmpChainSdkClient->blockchain()->getBlockByHeight(1).meta.generationHash;
    tmpChainSdkClient.reset();

    if ("mijin" == networkName) {
        config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Mijin;
    } else if ("mijinTest" == networkName) {
        config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Mijin_Test;
    } else if ("private" == networkName) {
        config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Private;
    } else if ("privateTest" == networkName) {
        config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Private_Test;
    } else if ("public" == networkName) {
        config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Public;
    } else if ("publicTest" == networkName) {
        config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Public_Test;
    } else {
        config->NetworkId = xpx_chain_sdk::NetworkIdentifier::Zero;
    }

    mpChainClient = xpx_chain_sdk::getClient(config);
    mpChainAccount = std::make_shared<xpx_chain_sdk::Account>([privateKey](
            xpx_chain_sdk::PrivateKeySupplierReason reason,
            xpx_chain_sdk::PrivateKeySupplierParam param)
    {
        xpx_chain_sdk::Key key;
        ParseHexStringIntoContainer(privateKey.c_str(), privateKey.size(), key);
        return xpx_chain_sdk::PrivateKey(key.data(), key.size());
    }, mpChainClient->getConfig()->NetworkId);

    mpBlockchainEngine = std::make_shared<BlockchainEngine>(mpChainClient);
    mpBlockchainEngine->init(80); // 1000 milliseconds / 15 request per second

    mpTransactionsEngine = std::make_shared<TransactionsEngine>(mpChainClient, mpChainAccount, mpBlockchainEngine);

    connect(mpTransactionsEngine.get(), &TransactionsEngine::createDownloadTransactionConfirmed, this, [this](auto alias, auto channelId, auto driveKey) {
        emit downloadChannelOpenTransactionConfirmed(alias, channelId, driveKey);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::createDownloadTransactionFailed, this, [this](auto channelId, auto errorText) {
        emit downloadChannelOpenTransactionFailed(channelId, errorText);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::finishDownloadTransactionConfirmed, this, [this](auto channelId) {
        emit downloadChannelCloseTransactionConfirmed(channelId);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::finishDownloadTransactionFailed, this, [this](auto errorText) {
        emit downloadChannelCloseTransactionFailed(errorText);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::createDriveTransactionConfirmed, this, [this](auto alias, auto driveId) {
        emit prepareDriveTransactionConfirmed(alias, driveId);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::createDriveTransactionFailed, this, [this](auto alias, auto driveKey, auto errorText) {
        emit prepareDriveTransactionFailed(alias, driveKey, errorText);
    });
}

void OnChainClient::loadDrives() {
    mpBlockchainEngine->getDrives([this](auto drivesPage, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            qWarning() << message.c_str() << " : " << code.c_str();
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
            qWarning() << message.c_str() << " : " << code.c_str();
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

void OnChainClient::addDownloadChannel(const std::string& channelAlias,
                                       const std::vector<std::array<uint8_t, 32>> &listOfAllowedPublicKeys,
                                       const std::array<uint8_t, 32> &drivePubKey, const uint64_t &prepaidSize,
                                       const uint64_t &feedbacksNumber) {
    mpTransactionsEngine->addDownloadChannel(channelAlias, listOfAllowedPublicKeys, drivePubKey, prepaidSize, feedbacksNumber);
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