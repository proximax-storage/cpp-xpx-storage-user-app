#include "OnChainClient.h"

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
}

void OnChainClient::loadDrives() {
    mpBlockchainEngine->getDrives([this](auto drivesPage, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            // logs
            return;
        }

        auto publicKey = TransactionsEngine::rawHashToHex(mpChainAccount->publicKey());

        QStringList loadedDrives;
        for (const auto &drive: drivesPage.data.drives) {
            if (drive.data.owner == publicKey.toStdString()) {
                loadedDrives.push_back(drive.data.multisig.c_str());
            }
        }

        emit drivesLoaded(loadedDrives);
    });
}

void OnChainClient::loadDownloadChannels(const QString& drivePubKey) {
    mpBlockchainEngine->getDownloadChannels([this, drivePubKey](auto channelsPage, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            // logs
            return;
        }

        QStringList loadedChannels;
        for (const auto &channel: channelsPage.data.channels) {
            if (channel.data.drive == drivePubKey.toStdString()) {
                loadedChannels.push_back(channel.data.id.c_str());
            }
        }

        emit downloadChannelsLoaded(loadedChannels);
    });
}

std::shared_ptr<BlockchainEngine> OnChainClient::getBlockchainEngine() {
    return mpBlockchainEngine;
}