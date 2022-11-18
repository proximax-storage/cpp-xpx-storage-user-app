#include "OnChainClient.h"
#include <QDebug>
#include <boost/algorithm/string/predicate.hpp>

OnChainClient::OnChainClient(std::shared_ptr<StorageEngine> storage,
                             const std::string& privateKey,
                             const std::string& address,
                             const std::string& port,
                             QObject* parent)
    : QObject(parent)
    , mpStorageEngine(storage)
{
    qRegisterMetaType<OnChainClient::ChannelsType>("OnChainClient::ChannelsType");

    init(address, port, privateKey);

    connect(this, &OnChainClient::drivesPageLoaded, this, [this](const QUuid& id, const xpx_chain_sdk::drives_page::DrivesPage& drivesPage){
        if (drivesPage.pagination.totalPages == 1 ) {
            emit drivesLoaded({ drivesPage });
            return;
        }

        if (mLoadedDrives.contains(id)) {
            mLoadedDrives[id].push_back(drivesPage);
            if (drivesPage.pagination.pageNumber == drivesPage.pagination.totalPages) {
                emit drivesLoaded(mLoadedDrives[id]);
                mLoadedDrives.erase(id);
            }
        } else {
            mLoadedDrives.insert({ id, { drivesPage } });
        }
    });

    connect(this, &OnChainClient::downloadChannelsPageLoaded, this, [this](const QUuid& id, ChannelsType type, const xpx_chain_sdk::download_channels_page::DownloadChannelsPage& channelsPage){
        if (channelsPage.pagination.totalPages == 1 ) {
            emit downloadChannelsLoaded(type, { channelsPage });
            return;
        }

        if (mLoadedChannels.contains(id)) {
            mLoadedChannels[id].push_back(channelsPage);
            if (channelsPage.pagination.pageNumber == channelsPage.pagination.totalPages) {
                emit downloadChannelsLoaded(type, mLoadedChannels[id]);
                mLoadedChannels.erase(id);
            }
        } else {
            mLoadedChannels.insert({ id, { channelsPage } });
        }
    });
}

void OnChainClient::loadDrives(const xpx_chain_sdk::DrivesPageOptions& options) {
    mpBlockchainEngine->getDrives(options, [this, options](auto drivesPage, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            qWarning() << LOG_SOURCE << message.c_str() << " : " << code.c_str();
            return;
        }

        QUuid id = QUuid::createUuid();
        // 1 page
        emit drivesPageLoaded(id, drivesPage);

        qDebug() << LOG_SOURCE << " drivesPageLoaded: " << " id: " << id << " pageNumber:" << drivesPage.pagination.pageNumber;

        for (uint64_t pageNumber = 2; pageNumber <= drivesPage.pagination.totalPages; pageNumber++) {
            xpx_chain_sdk::PaginationOrderingOptions paginationOptions;
            paginationOptions.pageNumber = pageNumber;
            xpx_chain_sdk::DrivesPageOptions newOptions;
            newOptions.paginationOrderingOptions = paginationOptions;
            mpBlockchainEngine->getDrives(newOptions, [this, id](auto drivesPage, auto isSuccess, auto message, auto code) {
                if (!isSuccess) {
                    qWarning() << LOG_SOURCE << message.c_str() << " : " << code.c_str();
                    return;
                }

                emit drivesPageLoaded(id, drivesPage);

                qDebug() << LOG_SOURCE << " drivesPageLoaded: " << " id: " << id << " pageNumber:" << drivesPage.pagination.pageNumber;
            });
        }
    });
}

void OnChainClient::loadDownloadChannels(const xpx_chain_sdk::DownloadChannelsPageOptions& options) {
    mpBlockchainEngine->getDownloadChannels(options, [this, options](auto channelsPage, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            qWarning() << LOG_SOURCE << message.c_str() << " : " << code.c_str();
            return;
        }

        QUuid id = QUuid::createUuid();
        // 1 page
        bool isSponsored = options.toMap().empty();
        bool isMyOwn = options.consumerKey.has_value() && options.toMap().size() == 1;
        if (isMyOwn) {
            loadMyOwnChannels(id, channelsPage);
        } else {
            loadSponsoredChannels(id, channelsPage);
        }

        for (uint64_t pageNumber = 2; pageNumber <= channelsPage.pagination.totalPages; pageNumber++) {
            xpx_chain_sdk::PaginationOrderingOptions paginationOptions;
            paginationOptions.pageNumber = pageNumber;
            xpx_chain_sdk::DownloadChannelsPageOptions newOptions;
            newOptions.paginationOrderingOptions = paginationOptions;
            mpBlockchainEngine->getDownloadChannels(newOptions, [this, isMyOwn, isSponsored, id](auto channelsPage, auto isSuccess, auto message, auto code) {
                if (!isSuccess) {
                    qWarning() << LOG_SOURCE << message.c_str() << " : " << code.c_str();
                    return;
                }

                if (isMyOwn) {
                    loadMyOwnChannels(id, channelsPage);
                } else if(isSponsored) {
                    loadSponsoredChannels(id, channelsPage);
                } else {
                    emit downloadChannelsPageLoaded(id, ChannelsType::Others, channelsPage);
                }
            });
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

std::string OnChainClient::addDrive(const std::string &driveAlias, const uint64_t &driveSize, ushort replicatorsCount) {
    return mpTransactionsEngine->addDrive(driveAlias, driveSize, replicatorsCount);
}

void OnChainClient::closeDrive(const std::array<uint8_t, 32> &driveKey) {
    mpTransactionsEngine->closeDrive(driveKey);
}

void OnChainClient::cancelDataModification(const std::array<uint8_t, 32> &driveId) {
    mpTransactionsEngine->cancelDataModification(driveId);
}

void OnChainClient::applyDataModification(const std::array<uint8_t, 32> &driveId,
                                          const sirius::drive::ActionList &actions,
                                          const std::string &sandboxFolder) {
    mpBlockchainEngine->getDriveById(rawHashToHex(driveId).toStdString(),
                                     [this, driveId, actions, sandboxFolder]
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

        mpTransactionsEngine->applyDataModification(driveId, actions, sandboxFolder, addresses);
    });
}

void OnChainClient::downloadPayment(const std::array<uint8_t, 32> &channelId, uint64_t amount) {
    mpTransactionsEngine->downloadPayment(channelId, amount);
}

void OnChainClient::storagePayment(const std::array<uint8_t, 32> &driveId, const uint64_t &amount) {
    mpTransactionsEngine->storagePayment(driveId, amount);
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

    connect(mpTransactionsEngine.get(), &TransactionsEngine::downloadPaymentConfirmed, this, [this](auto channelId) {
        emit downloadPaymentTransactionConfirmed(channelId);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::downloadPaymentFailed, this, [this](auto channelId, auto errorText) {
        emit downloadPaymentTransactionFailed(channelId, errorText);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::storagePaymentConfirmed, this, [this](auto driveId) {
        emit storagePaymentTransactionConfirmed(driveId);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::storagePaymentFailed, this, [this](auto driveId, auto errorText) {
        emit storagePaymentTransactionFailed(driveId, errorText);
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

    connect(mpTransactionsEngine.get(), &TransactionsEngine::cancelModificationConfirmed, this, [this](auto driveId, auto modificationId) {
        emit cancelModificationTransactionConfirmed(driveId, modificationId);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::cancelModificationFailed, this, [this](auto modificationId) {
        emit cancelModificationTransactionFailed(modificationId);
    });

    connect(mpTransactionsEngine.get(), &TransactionsEngine::dataModificationApprovalConfirmed, this,
            [this](auto driveId, auto fileStructureCdi) {
                emit dataModificationApprovalTransactionConfirmed(driveId, fileStructureCdi);
                auto callback = [this](const std::string& driveId, const std::array<uint8_t, 32>& fsTreeHash, const sirius::drive::FsTree& fsTree) {
                    emit fsTreeDownloaded(driveId, fsTreeHash, fsTree);
                };

                mpStorageEngine->downloadFsTree(rawHashToHex(driveId).toStdString(),
                                                "0000000000000000000000000000000000000000000000000000000000000000",
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
            emit initializedFailed(message.c_str());
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

        mpBlockchainEngine->getBlockByHeight(1, [this, config, privateKey, info](auto block, auto isSuccess, auto message, auto code) {
            if (!isSuccess) {
                qWarning() << LOG_SOURCE << "message: " << message.c_str() << " code: " << code.c_str();
                emit initializedFailed(message.c_str());
                return;
            }

            config->GenerationHash = block.meta.generationHash;
            initAccount(privateKey);
            mpTransactionsEngine = std::make_shared<TransactionsEngine>(mpChainClient, mpChainAccount, mpBlockchainEngine);
            initConnects();
            emit initializedSuccessfully(info.name.c_str());
        });
    });
}

void OnChainClient::loadMyOwnChannels(const QUuid& id, xpx_chain_sdk::download_channels_page::DownloadChannelsPage channelsPage) {
    qDebug() << LOG_SOURCE << " loadMyOwnChannels: " << " id: " << id << " pageNumber:" << channelsPage.pagination.pageNumber;
    std::vector<xpx_chain_sdk::DownloadChannel> channels;
    for (const auto& channel : channelsPage.data.channels) {
        if (channel.data.finished) {
            continue;
        }

        for (const auto& key : channel.data.listOfPublicKeys) {
            if (boost::iequals( key, rawHashToHex(mpChainAccount->publicKey()).toStdString())) {
                channels.push_back(channel);
                break;
            }
        }
    }

    channelsPage.data.channels = channels;
    emit downloadChannelsPageLoaded(id, ChannelsType::MyOwn, channelsPage);
}

void OnChainClient::loadSponsoredChannels(const QUuid& id, xpx_chain_sdk::download_channels_page::DownloadChannelsPage channelsPage) {
    qDebug() << LOG_SOURCE << " loadSponsoredChannels: " << " id: " << id << " pageNumber:" << channelsPage.pagination.pageNumber;

    std::vector<xpx_chain_sdk::DownloadChannel> channels;
    for (const auto& channel : channelsPage.data.channels) {
        bool isMyOwn = boost::iequals(channel.data.consumer, rawHashToHex(mpChainAccount->publicKey()).toStdString());
        if (channel.data.finished || isMyOwn) {
            continue;
        }

        for (const auto& key : channel.data.listOfPublicKeys) {
            if (boost::iequals( key, rawHashToHex(mpChainAccount->publicKey()).toStdString())) {
                channels.push_back(channel);
                break;
            }
        }
    }

    channelsPage.data.channels = channels;
    emit downloadChannelsPageLoaded(id, ChannelsType::Sponsored, channelsPage);
}