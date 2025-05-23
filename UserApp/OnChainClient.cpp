#include "OnChainClient.h"
#include "Utils.h"
#include <QDebug>
#include <boost/algorithm/string/predicate.hpp>

OnChainClient::OnChainClient(StorageEngine* storage, QObject* parent)
    : QObject(parent)
    , mpStorageEngine(storage)
    , mpDialogSignalsEmitter(new DialogSignals(this))
{
    qRegisterMetaType<OnChainClient::ChannelsType>("OnChainClient::ChannelsType");
}

void OnChainClient::start(const QString& privateKey,
                          const QString& address,
                          const QString& port,
                          double feeMultiplier) {
    init(address, port, feeMultiplier, privateKey);

    connect(this, &OnChainClient::drivesPageLoaded, this, [this](const QUuid& id, const xpx_chain_sdk::drives_page::DrivesPage& drivesPage){
        if (drivesPage.pagination.totalPages <= 1 ) {
            emit drivesLoaded({ drivesPage });
            return;
        }

        if ( MAP_CONTAINS( mLoadedDrives, id )) {
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
        if (channelsPage.pagination.totalPages <= 1 ) {
            emit downloadChannelsLoaded(type, { channelsPage });
            return;
        }

        if ( MAP_CONTAINS( mLoadedChannels, id )) {
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

        qDebug() <<"OnChainClient::loadDrives. Drives Page Loaded: " << " id: " << id << " page Number:" << drivesPage.pagination.pageNumber;

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

                qDebug() << "OnChainClient::loadDrives. Drives Page Loaded: " << " id: " << id << " page Number:" << drivesPage.pagination.pageNumber;
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

BlockchainEngine* OnChainClient::getBlockchainEngine() {
    return mpBlockchainEngine;
}

void OnChainClient::addDownloadChannel(const QString& channelAlias,
                                       const std::vector<std::array<uint8_t, 32>> &listOfAllowedPublicKeys,
                                       const std::array<uint8_t, 32> &drivePubKey,
                                       const uint64_t &prepaidSize,
                                       const uint64_t &feedbacksNumber,
                                       const std::function<void(QString hash)>& callback,
                                       const std::function<bool(const QString& transactionFee)>& confirmationCallback) {
    auto deadlineCallback = [this, alias = channelAlias, keys = listOfAllowedPublicKeys,
                                                   pKey = drivePubKey, prepaid = prepaidSize,
                                                   feedback = feedbacksNumber, cb = callback, ccb = confirmationCallback]
                                                    (std::optional<xpx_chain_sdk::NetworkDuration> deadline) {
        mpTransactionsEngine->addDownloadChannel(alias, keys, pKey, prepaid, feedback, deadline, cb, ccb);
    };

    mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
}

void OnChainClient::closeDownloadChannel(const std::array<uint8_t, 32> &channelId) {
    auto deadlineCallback = [this, channelId](std::optional<xpx_chain_sdk::NetworkDuration> deadline) {
        mpTransactionsEngine->closeDownloadChannel(channelId, deadline);
    };

    mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
}

void OnChainClient::addDrive(const uint64_t &driveSize, ushort replicatorsCount,
                             const std::function<void(QString hash)>& callback,
                             const std::function<bool(const QString& transactionFee)>& confirmationCallback) {
    auto deadlineCallback = [this, size = driveSize, count = replicatorsCount, cb = callback, confCb = confirmationCallback]
                                                   (std::optional<xpx_chain_sdk::NetworkDuration> deadline)
    {
        mpTransactionsEngine->addDrive(size, count, deadline, cb, confCb);
    };

    mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
}

void OnChainClient::closeDrive(const std::array<uint8_t, 32> &driveKey, const std::function<bool(const QString& transactionFee)>& callback) {
    auto deadlineCallback = [this, driveKey, cb = callback](std::optional<xpx_chain_sdk::NetworkDuration> deadline) {
        mpTransactionsEngine->closeDrive(driveKey, deadline, cb);
    };

    mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
}

void OnChainClient::cancelDataModification(const std::array<uint8_t, 32> &driveId, const std::function<bool(const QString& transactionFee)>& callback) {
    mpBlockchainEngine->getDriveById(rawHashToHex(driveId).toStdString(),
                                     [this, cb = callback](auto drive, auto isSuccess, auto message, auto code)
    {
        if (!isSuccess) {
            qWarning() << "OnChainClient::cancelDataModification. Drive key: " << drive.data.multisig.c_str() << " Message: " << message.c_str() << " code: " << code.c_str();
            emit newError(ErrorType::Network, message.c_str());
            return;
        }

        auto deadlineCallback = [this, drive, ccb = cb](std::optional<xpx_chain_sdk::NetworkDuration> deadline) {
            mpTransactionsEngine->cancelDataModification(drive, deadline, ccb);
        };

        mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
    });
}

void OnChainClient::applyDataModification(const std::array<uint8_t, 32> &driveId,
                                          const sirius::drive::ActionList &actions,
                                          const std::function<bool(const QString& transactionFee)>& callback)
{
    mpBlockchainEngine->getDriveById(rawHashToHex(driveId).toStdString(),
                                     [this, driveId, actions, cb = callback]
                                     (auto drive, auto isSuccess, auto message, auto code)
    {
        if (!isSuccess)
        {
            qWarning() << "OnChainClient::applyDataModification. Message: " << message.c_str() << " code: " << code.c_str();
            emit newError(ErrorType::Network, message.c_str());
            return;
        }

        if (drive.data.replicators.empty())
        {
            emit dataModificationTransactionFailed(driveId, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, " received empty replicators list");
            emit newError(ErrorType::InvalidData, "OnChainClient::applyDataModification. Empty replicators list received for the drive: " + QString::fromStdString(drive.data.multisig));
            return;
        }

        std::vector<xpx_chain_sdk::Address> addresses;
        addresses.reserve(drive.data.replicators.size());
        for (const auto& replicator : drive.data.replicators)
        {
            xpx_chain_sdk::Key key;
            xpx_chain_sdk::ParseHexStringIntoContainer(replicator.c_str(), replicator.size(), key);
            addresses.emplace_back(key);
        }

        auto deadlineCallback = [this, ccb = cb, driveId, actions, addresses, replicators = drive.data.replicators](std::optional<xpx_chain_sdk::NetworkDuration> deadline)
        {
            mpTransactionsEngine->applyDataModification(driveId, actions, addresses, convertToQStringVector(replicators), deadline, ccb);
        };

        mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
    });
}

void OnChainClient::downloadPayment(const std::array<uint8_t, 32> &channelId, uint64_t amount)
{
    auto deadlineCallback = [this, channelId, amount](std::optional<xpx_chain_sdk::NetworkDuration> deadline)
    {
        mpTransactionsEngine->downloadPayment(channelId, amount, deadline);
    };

    mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
}

void OnChainClient::storagePayment(const std::array<uint8_t, 32> &driveId, const uint64_t &amount)
{
    auto deadlineCallback = [this, driveId, amount](std::optional<xpx_chain_sdk::NetworkDuration> deadline)
    {
        mpTransactionsEngine->storagePayment(driveId, amount, deadline);
    };

    mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
}

void OnChainClient::replicatorOnBoarding(const QString &replicatorPrivateKey, const QString& nodeBootPrivateKey, uint64_t capacityMB, bool isExists) {
    auto keyPair = sirius::crypto::KeyPair::FromPrivate(sirius::crypto::PrivateKey::FromString(replicatorPrivateKey.toStdString()));
    auto publicKey = rawHashToHex(keyPair.publicKey().array());
    if (isExists) {
        mpBlockchainEngine->getReplicatorById(publicKey.toStdString(),
            [this, isExists, publicKey, replicatorPrivateKey](auto drivesPage, auto isSuccess, auto message, auto code){
            if (!isSuccess) {
                qWarning() << "nChainClient::replicatorOnBoarding. Replicator marked as exists, but not found!. Id: " << publicKey;

                QString notification;
                notification.append("Replicator marked as exists: ");
                notification.append("'");
                notification.append(publicKey);
                notification.append("'");
                notification.append(" but not found!");
                emit newNotification(notification);
                return;
            }

            qInfo() << "OnChainClient::replicatorOnBoarding. Callback. Replicator already exists: " << publicKey;
            emit replicatorOnBoardingTransactionConfirmed(publicKey, isExists);
        });
    } else {

        auto deadlineCallback = [this, replicatorPrivateKey, nodeBootPrivateKey, capacityMB](std::optional<xpx_chain_sdk::NetworkDuration> deadline) {
            mpTransactionsEngine->replicatorOnBoarding(replicatorPrivateKey, nodeBootPrivateKey, capacityMB, deadline);
        };

        mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
    }
}

void OnChainClient::replicatorOffBoarding(const std::array<uint8_t, 32> &driveId, const QString &replicatorPrivateKey) {
    auto deadlineCallback = [this, driveId, replicatorPrivateKey](std::optional<xpx_chain_sdk::NetworkDuration> deadline) {
        mpTransactionsEngine->replicatorOffBoarding(driveId, replicatorPrivateKey, deadline);
    };

    mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
}

void OnChainClient::calculateUsedSpaceOfReplicator(const QString& publicKey, std::function<void(uint64_t index, uint64_t usedSpace)> callback) {
    mpBlockchainEngine->getReplicatorById(publicKey.toStdString(), [this, callback] (auto replicator, auto isSuccess, auto message, auto code ) {
        if (!isSuccess) {
            qWarning() << "OnChainClient::calculateUsedSpaceOfReplicator. Error: " << message.c_str() << " : " << code.c_str();
            callback(0, 0);
            return;
        }

        if (replicator.data.drivesInfo.empty()) {
            callback(0, 0);
        }

        for (uint64_t i = 0; i < replicator.data.drivesInfo.size(); i++) {
            mpBlockchainEngine->getDriveById(replicator.data.drivesInfo[i].drive, [callback, i](auto drive, auto isSuccess, auto message, auto code ){
                if (!isSuccess) {
                    qWarning() << "OnChainClient::calculateUsedSpaceOfReplicator:: drive info. Error: " << message.c_str() << " Code: " << code.c_str();
                    callback(i, 0);
                    return;
                }

                // megabytes
                callback(i ,drive.data.size);
            });
        }
    });
}

StorageEngine* OnChainClient::getStorageEngine()
{
    return mpStorageEngine;
}

DialogSignals* OnChainClient::getDialogSignalsEmitter()
{
    return mpDialogSignalsEmitter;
}

void OnChainClient::initConnects() {
    connect(mpTransactionsEngine, &TransactionsEngine::newError, this, [this](auto errorText) {
        emit newError(ErrorType::InvalidData, errorText);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::createDownloadChannelConfirmed, this, [this](auto alias, auto channelId, auto driveKey) {
        emit downloadChannelOpenTransactionConfirmed(alias, rawHashToHex(channelId), rawHashToHex(driveKey));
    });

    connect(mpTransactionsEngine, &TransactionsEngine::createDownloadChannelFailed, this, [this](auto channelId, auto errorText) {
        emit downloadChannelOpenTransactionFailed(channelId, errorText);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::closeDownloadChannelConfirmed, this, [this](auto channelId) {
        emit downloadChannelCloseTransactionConfirmed(channelId);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::closeDownloadChannelFailed, this, [this](auto channelId, auto errorText) {
        emit downloadChannelCloseTransactionFailed(channelId, errorText);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::createDriveConfirmed, this, [this](auto driveId) {
        emit prepareDriveTransactionConfirmed(driveId);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::createDriveFailed, this, [this](auto driveKey, auto errorText) {
        emit prepareDriveTransactionFailed(driveKey, errorText);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::closeDriveConfirmed, this, [this](auto driveId) {
        emit closeDriveTransactionConfirmed(driveId);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::closeDriveFailed, this, [this](auto driveKey, auto errorText) {
        emit closeDriveTransactionFailed(driveKey, errorText);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::downloadPaymentConfirmed, this, [this](auto channelId) {
        emit downloadPaymentTransactionConfirmed(channelId);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::downloadPaymentFailed, this, [this](auto channelId, auto errorText) {
        emit downloadPaymentTransactionFailed(channelId, errorText);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::storagePaymentConfirmed, this, [this](auto driveId) {
        emit storagePaymentTransactionConfirmed(driveId);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::storagePaymentFailed, this, [this](auto driveId, auto errorText) {
        emit storagePaymentTransactionFailed(driveId, errorText);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::addActions, this, [this](auto actionList, auto driveId, auto sandboxFolder, auto replicators, auto callback) {
        uint64_t modificationsSize = 0;
        auto hash = mpStorageEngine->addActions(actionList, driveId, sandboxFolder, modificationsSize, replicators);
        callback(modificationsSize, hash.array());
    }, Qt::QueuedConnection);

    connect(mpTransactionsEngine, &TransactionsEngine::dataModificationConfirmed, this, [this](auto driveId,auto modificationId) {
        emit dataModificationTransactionConfirmed(driveId, modificationId);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::dataModificationFailed, this, [this](auto driveId, auto modificationId, auto status) {
        emit dataModificationTransactionFailed(driveId, modificationId, status);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::cancelModificationConfirmed, this, [this](auto driveId, auto modificationId) {
        emit cancelModificationTransactionConfirmed(driveId, modificationId);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::cancelModificationFailed, this, [this](auto driveId, auto modificationId, auto error) {
        emit cancelModificationTransactionFailed(driveId, modificationId, error);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::replicatorOnBoardingConfirmed, this, [this](auto replicatorPublicKey, auto isExists) {
        emit replicatorOnBoardingTransactionConfirmed(replicatorPublicKey, isExists);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::replicatorOnBoardingFailed, this, [this](auto replicatorPublicKey, auto replicatorPrivateKey, auto error) {
        emit replicatorOnBoardingTransactionFailed(replicatorPublicKey, replicatorPrivateKey, error);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::replicatorOffBoardingConfirmed, this, [this](auto replicatorPublicKey) {
        emit replicatorOffBoardingTransactionConfirmed(replicatorPublicKey);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::replicatorOffBoardingFailed, this, [this](auto replicatorPublicKey, auto error) {
        emit replicatorOffBoardingTransactionFailed(replicatorPublicKey, error);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::deployContractConfirmed, this, [this](auto driveId, auto contractId) {
        emit deployContractTransactionConfirmed(driveId, contractId);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::deployContractFailed, this, [this](auto driveId, auto contractId) {
        emit deployContractTransactionFailed(driveId, contractId);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::deployContractApprovalConfirmed, this, [this](auto driveId, auto contractId) {
        emit deployContractTransactionApprovalConfirmed(driveId, contractId);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::deployContractApprovalFailed, this, [this](auto driveId, auto contractId) {
        emit deployContractTransactionApprovalFailed(driveId, contractId);
    });

    connect( mpTransactionsEngine, &TransactionsEngine::manualCallConfirmed, this,
             [this]( std::array<uint8_t, 32> contractId, std::array<uint8_t, 32> callId ) {
                 emit manualCallTransactionConfirmed( contractId, callId );
             } );

    connect( mpTransactionsEngine, &TransactionsEngine::manualCallFailed, this,
             [this]( std::array<uint8_t, 32> contractId, std::array<uint8_t, 32> callId ) {
                 emit manualCallTransactionFailed( contractId, callId );
             } );

    connect(mpTransactionsEngine, &TransactionsEngine::dataModificationApprovalConfirmed, this,
            [this](auto driveId, auto fileStructureCdi, auto isStream) {
                const QString driveKey = rawHashToHex(driveId);
                qInfo() << "TransactionsEngine::dataModificationApprovalConfirmed. Drive key: " << driveKey << " fileStructureCdi: " << fileStructureCdi;
                if (!isStream)
                {
                    emit dataModificationApprovalTransactionConfirmed(driveId, fileStructureCdi);
                }

                emit downloadFsTreeDirect(driveKey, fileStructureCdi);
    }, Qt::QueuedConnection);

    connect(mpTransactionsEngine, &TransactionsEngine::dataModificationApprovalFailed, this,  [this](auto driveId, auto fileStructureCdi, auto code) {
        emit dataModificationApprovalTransactionFailed(driveId, fileStructureCdi, code);
    }, Qt::QueuedConnection);

    connect(mpTransactionsEngine, &TransactionsEngine::removeTorrent, this, [this](auto torrentId) {
        mpStorageEngine->removeTorrentSync(torrentId);
    }, Qt::QueuedConnection);

    connect(mpTransactionsEngine, &TransactionsEngine::removeFromTorrentMap, this, [this](auto downloadDataCdi) {
            mpStorageEngine->removeTorrentSync(rawHashFromHex(downloadDataCdi));
    }, Qt::QueuedConnection);

    connect(mpTransactionsEngine, &TransactionsEngine::streamStartConfirmed, this, [this](auto streamId) {
        emit streamStartTransactionConfirmed(streamId);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::streamStartFailed, this, [this](auto streamId, auto errorText) {
        emit streamStartTransactionFailed(streamId, errorText);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::streamFinishConfirmed, this, [this](auto streamId) {
        emit streamFinishTransactionConfirmed(streamId);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::streamFinishFailed, this, [this](auto streamId, auto errorText) {
        emit streamFinishTransactionFailed(streamId, errorText);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::streamPaymentConfirmed, this, [this](auto streamId) {
        emit streamPaymentTransactionConfirmed(streamId);
    });

    connect(mpTransactionsEngine, &TransactionsEngine::streamPaymentFailed, this, [this](auto streamId, auto errorText) {
        emit streamPaymentTransactionFailed(streamId, errorText);
    });
}

void OnChainClient::initAccount(const QString &privateKey) {
    mpChainAccount = std::make_shared<xpx_chain_sdk::Account>([privateKey](
            xpx_chain_sdk::PrivateKeySupplierReason reason,
            xpx_chain_sdk::PrivateKeySupplierParam param)
    {
        xpx_chain_sdk::Key key;
        ParseHexStringIntoContainer(privateKey.toStdString().c_str(), privateKey.size(), key);
        return xpx_chain_sdk::PrivateKey(key.data(), key.size());
    }, mpChainClient->getConfig().NetworkId);
}

void OnChainClient::init(const QString& address,
                         const QString& port,
                         const double feeMultiplier,
                         const QString& privateKey) {
    xpx_chain_sdk::Config& config = xpx_chain_sdk::GetConfig();

    QString resolvedHost = address + ":" + port;
    if (!isResolvedToIpAddress(resolvedHost)) {
        emit newError(ErrorType::NetworkInit, "Cannot resolve host(1): " + resolvedHost);
        return;
    }

    qDebug() << "OnChainClient::init: REST bootstrap ip address: " << resolvedHost;

    auto hostRaw = resolvedHost.split(":");
    if (hostRaw.size() == 2) {
        resolvedHost = hostRaw[0];
    } else {
        emit newError(ErrorType::NetworkInit, "Cannot resolve host(2): " + resolvedHost);
        return;
    }

    config.nodeAddress = resolvedHost.toStdString();
    config.port = port.toStdString();
    config.TransactionFeeMultiplier = feeMultiplier;

    mpChainClient = xpx_chain_sdk::getClient(config);
    mpBlockchainEngine = new BlockchainEngine(mpChainClient, this);
    connect(this, &OnChainClient::connectedToServer, this, [this, &config, privateKey](){ onConnected(config, privateKey); });

    mpChainClient->notifications()->connect([this](auto message, auto code) {
        if (code == 0)
        {
            // Separate thread.
            emit connectedToServer();
        }
        else
        {
            const QString error = QString::fromStdString("Network error: " + message + ". Code: " + std::to_string(code));
            qCritical () << "OnChainClient::init. " << error;
            emit newError(ErrorType::NetworkInit, error);
        }
    });
}

void OnChainClient::loadMyOwnChannels(const QUuid& id, xpx_chain_sdk::download_channels_page::DownloadChannelsPage channelsPage) {
    qDebug() << "OnChainClient::loadMyOwnChannels. id: " << id << " pageNumber:" << channelsPage.pagination.pageNumber;
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
    qDebug() << "OnChainClient::loadSponsoredChannels. id: " << id << " pageNumber:" << channelsPage.pagination.pageNumber;

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

void OnChainClient::onConnected(xpx_chain_sdk::Config& config, const QString& privateKey) {
    qInfo () << "OnChainClient::init. Connected to server: " << config.nodeAddress.c_str() << ":" << config.port.c_str();

    mpBlockchainEngine->init(80); // 1000 milliseconds / 15 request per second
    mpBlockchainEngine->getNetworkInfo([this, &config, privateKey](auto info, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            qWarning() << "OnChainClient::init. getNetworkInfo. Message: " << message.c_str() << " code: " << code.c_str();
            emit newError(ErrorType::Network, message.c_str());
            return;
        }

        if (boost::iequals("mijin", info.name)) {
            config.NetworkId = xpx_chain_sdk::NetworkIdentifier::Mijin;
        } else if (boost::iequals("mijinTest", info.name)) {
            config.NetworkId = xpx_chain_sdk::NetworkIdentifier::Mijin_Test;
        } else if (boost::iequals("private", info.name)) {
            config.NetworkId = xpx_chain_sdk::NetworkIdentifier::Private;
        } else if (boost::iequals("privateTest", info.name)) {
            config.NetworkId = xpx_chain_sdk::NetworkIdentifier::Private_Test;
        } else if (boost::iequals("public", info.name)) {
            config.NetworkId = xpx_chain_sdk::NetworkIdentifier::Public;
            info.name = "Sirius Mainnet";
        } else if (boost::iequals("publicTest", info.name)) {
            config.NetworkId = xpx_chain_sdk::NetworkIdentifier::Public_Test;
            info.name = "Sirius Testnet 2";
        } else {
            config.NetworkId = xpx_chain_sdk::NetworkIdentifier::Zero;
        }

        mpBlockchainEngine->getBlockByHeight(1, [this, &config, privateKey, info](auto block, auto isSuccess, auto message, auto code) {
            if (!isSuccess) {
                qWarning() << "OnChainClient::init. getBlockByHeight. Message: " << message.c_str() << " code: " << code.c_str();
                emit newError(ErrorType::Network, message.c_str());
                return;
            }

            config.GenerationHash = block.meta.generationHash;
            initAccount(privateKey);
            mpTransactionsEngine = new TransactionsEngine(mpChainClient, mpChainAccount, mpBlockchainEngine, this);
            initConnects();
            emit networkInitialized(info.name.c_str());

            xpx_chain_sdk::Notifier<xpx_chain_sdk::Block> blockNotifier([this](auto, auto) {
                emit updateBalance();
            });

            mpChainClient->notifications()->addBlockNotifiers({ blockNotifier }, [](){}, [](auto){});
        });
                                       });
}

void OnChainClient::deployContract( const std::array<uint8_t, 32>& driveKey, const ContractDeploymentData& data ) {
    mpBlockchainEngine->getDriveById(rawHashToHex(driveKey).toStdString(),
                                     [this, driveKey, data]
                                     (auto drive, auto isSuccess, auto message, auto code) {
        if (!isSuccess) {
            qWarning() << "OnChainClient::deployContract. Message: " << message.c_str() << " code: " << code.c_str();
            emit newError(ErrorType::Network, message.c_str());
            return;
        }

        if (drive.data.replicators.empty()) {
            emit newError(ErrorType::InvalidData, "OnChainClient::deployContract. Empty replicators list received for the drive: " + QString::fromStdString(drive.data.multisig));
            return;
        }

        std::vector<xpx_chain_sdk::Address> addresses;
        addresses.reserve(drive.data.replicators.size());
        for (const auto& replicator : drive.data.replicators) {
            xpx_chain_sdk::Key key;
            xpx_chain_sdk::ParseHexStringIntoContainer(replicator.c_str(), replicator.size(), key);
            addresses.emplace_back(key);
        }

        auto deadlineCallback = [this, driveKey, data, addresses](std::optional<xpx_chain_sdk::NetworkDuration> deadline) {
            mpTransactionsEngine->deployContract(driveKey, data, addresses, deadline);
        };

        mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
    });
}

void OnChainClient::runContract(const ContractManualCallData& data ) {
    auto deadlineCallback = [this, data](std::optional<xpx_chain_sdk::NetworkDuration> deadline) {
        mpTransactionsEngine->runManualCall(data, {}, deadline);
    };

    mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
}

void OnChainClient::streamStart(const std::array<uint8_t, 32> &rawDrivePubKey,
                                const QString &folderName,
                                uint64_t expectedUploadSizeMegabytes,
                                uint64_t feedbackFeeAmount,
                                const std::function<void(QString hash)>& callback) {
    auto deadlineCallback = [this, rawDrivePubKey, folderName, expectedUploadSizeMegabytes, feedbackFeeAmount, callback](std::optional<xpx_chain_sdk::NetworkDuration> deadline) {
        mpTransactionsEngine->streamStart(rawDrivePubKey, folderName, expectedUploadSizeMegabytes, feedbackFeeAmount, deadline, callback);
    };

    mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
}

void OnChainClient::streamFinish(const std::array<uint8_t, 32> &rawDrivePubKey,
                                 const std::array<uint8_t, 32> &streamId,
                                 uint64_t actualUploadSizeMegabytes,
                                 const std::array<uint8_t, 32> &streamStructureCdi)
{
    mpBlockchainEngine->getDriveById(rawHashToHex(rawDrivePubKey).toStdString(),
    [this, rawDrivePubKey, streamId, actualUploadSizeMegabytes, streamStructureCdi]
    (auto drive, auto isSuccess, auto message, auto code)
    {
        if (!isSuccess)
        {
            qWarning() << "OnChainClient::streamFinish. Message: " << message.c_str() << " code: " << code.c_str();
            emit newError(ErrorType::Network, message.c_str());
            return;
        }

        if (drive.data.replicators.empty())
        {
            emit dataModificationTransactionFailed(rawDrivePubKey, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, " received empty replicators list");
            emit newError(ErrorType::InvalidData, "OnChainClient::streamFinish. Empty replicators list received for the drive: " + QString::fromStdString(drive.data.multisig));
            return;
        }

        std::vector<xpx_chain_sdk::Address> addresses;
        addresses.reserve(drive.data.replicators.size());
        for (const auto& replicator : drive.data.replicators)
        {
            xpx_chain_sdk::Key key;
            xpx_chain_sdk::ParseHexStringIntoContainer(replicator.c_str(), replicator.size(), key);
            addresses.emplace_back(key);
        }

        auto deadlineCallback = [this, rawDrivePubKey, streamId, actualUploadSizeMegabytes, streamStructureCdi, addresses](auto deadline)
        {
            mpTransactionsEngine->streamFinish(rawDrivePubKey, streamId, actualUploadSizeMegabytes, streamStructureCdi, deadline, addresses);
        };

        mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
    });
}

void OnChainClient::streamPayment(const std::array<uint8_t, 32> &rawDrivePubKey,
                                  const std::array<uint8_t, 32> &streamId,
                                  uint64_t additionalUploadSizeMegabytes) {
    auto deadlineCallback = [this, rawDrivePubKey, streamId, additionalUploadSizeMegabytes](std::optional<xpx_chain_sdk::NetworkDuration> deadline) {
        mpTransactionsEngine->streamPayment(rawDrivePubKey, streamId, additionalUploadSizeMegabytes, deadline);
    };

    mpBlockchainEngine->getTransactionDeadline(deadlineCallback);
}
