#include "TransactionsEngine.h"
#include "Utils.h"

#include <qdebug.h>
#include <QFile>
#include <QFileInfoList>
#include <QDirIterator>

TransactionsEngine::TransactionsEngine(std::shared_ptr<xpx_chain_sdk::IClient> client,
                                       std::shared_ptr<xpx_chain_sdk::Account> account,
                                       BlockchainEngine* blockchainEngine,
                                       QObject* parent)
    : QObject(parent)
    , mpChainClient(client)
    , mpBlockchainEngine(blockchainEngine)
    , mpChainAccount(account)
{}

std::string TransactionsEngine::addDownloadChannel(const std::string& channelAlias,
                                                   const std::vector<std::array<uint8_t, 32>> &listOfAllowedPublicKeys,
                                                   const std::array<uint8_t, 32> &rawDrivePubKey, const uint64_t &prepaidSize,
                                                   const uint64_t &feedbacksNumber) {
    QString drivePubKey = rawHashToHex(rawDrivePubKey);

    xpx_chain_sdk::Key driveKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(drivePubKey.toStdString().c_str(), drivePubKey.size(), driveKey);

    std::vector<xpx_chain_sdk::Key> listOfAllowedPks;
    for (std::array<uint8_t, 32> key : listOfAllowedPublicKeys) {
        const QString keyHex = rawHashToHex(key);

        xpx_chain_sdk::Key newKey;
        xpx_chain_sdk::ParseHexStringIntoContainer(keyHex.toStdString().c_str(), keyHex.size(), newKey);
        listOfAllowedPks.emplace_back(newKey);
    }

    // TODO: fix empty list of keys
    auto downloadTransaction = xpx_chain_sdk::CreateDownloadTransaction(driveKey, prepaidSize, feedbacksNumber, listOfAllowedPks.size(), listOfAllowedPks,
                                                                        std::nullopt, std::nullopt, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(downloadTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> downloadNotifier;

    std::string hash = rawHashToHex(downloadTransaction->hash()).toStdString();
    statusNotifier.set([this, hash, downloadNotifierId = downloadNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (notification.hash == hash) {
            qWarning() << LOG_SOURCE << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), downloadNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit createDownloadChannelFailed(notification.hash.c_str(), notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << LOG_SOURCE << errorCode.message().c_str();
    });

    downloadNotifier.set([this, channelAlias, hash, rawDrivePubKey, statusNotifierId = statusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << LOG_SOURCE << "confirmed downloadTransaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            //emit balancesUpdated();
            emit createDownloadChannelConfirmed(channelAlias, rawHashFromHex(hash.c_str()), rawDrivePubKey);
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { downloadNotifier },
            [this, data = downloadTransaction->binary()]() { announceTransaction(data); },
            [this, hash](auto error) { onError(hash, error); });

    return hash;
}

void TransactionsEngine::closeDownloadChannel(const std::array<uint8_t, 32> &channelId) {
    QString channelIdHex = rawHashToHex(channelId);
    if( channelId.empty()) {
        qWarning() << LOG_SOURCE << "bad channelId: empty!";
        return;
    }

    xpx_chain_sdk::Key channelIdKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(channelIdHex.toStdString().c_str(), channelIdHex.size(), channelIdKey);

    const xpx_chain_sdk::Amount feedbackFeeAmount = 10;
    auto finishDownloadTransaction = xpx_chain_sdk::CreateFinishDownloadTransaction(channelIdKey, feedbackFeeAmount, std::nullopt, std::nullopt, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(finishDownloadTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> finishDownloadNotifier;

    const std::string hash = rawHashToHex(finishDownloadTransaction->hash()).toStdString();
    statusNotifier.set([this, hash, channelId, finishDownloadNotifierId = finishDownloadNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (notification.hash == hash) {
            qWarning() << LOG_SOURCE << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), finishDownloadNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit closeDownloadChannelFailed(channelId, notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << LOG_SOURCE << errorCode.message().c_str();
    });

    finishDownloadNotifier.set([this, hash, channelId, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << LOG_SOURCE << "confirmed finishDownloadTransaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit closeDownloadChannelConfirmed(channelId);
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { finishDownloadNotifier },
                                                               [this, data = finishDownloadTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}

void TransactionsEngine::downloadPayment(const std::array<uint8_t, 32> &channelId, uint64_t prepaidSize) {
    const QString channelIdHex = rawHashToHex(channelId);

    xpx_chain_sdk::Key channelIdKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(channelIdHex.toStdString().c_str(), channelIdHex.size(), channelIdKey);

    const xpx_chain_sdk::Amount feedbackFeeAmount = 10;

    auto downloadPaymentTransaction = xpx_chain_sdk::CreateDownloadPaymentTransaction(channelIdKey, prepaidSize, feedbackFeeAmount,
                                                                                      std::nullopt, std::nullopt, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(downloadPaymentTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> downloadPaymentNotifier;

    const std::string hash = rawHashToHex(downloadPaymentTransaction->hash()).toStdString();
    statusNotifier.set([this, hash, downloadPaymentNotifierId = downloadPaymentNotifier.getId(), channelId](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (notification.hash == hash) {
            qWarning() << LOG_SOURCE << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), downloadPaymentNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit downloadPaymentFailed(channelId, notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << LOG_SOURCE << errorCode.message().c_str();
    });

    downloadPaymentNotifier.set([this, hash, channelId, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << LOG_SOURCE << "confirmed downloadPaymentTransaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit downloadPaymentConfirmed(channelId);
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { downloadPaymentNotifier },
                                                               [this, data = downloadPaymentTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}

void TransactionsEngine::storagePayment(const std::array<uint8_t, 32> &driveId, const uint64_t& amount) {
    QString drivePubKey = rawHashToHex(driveId);

    xpx_chain_sdk::Key driveKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(drivePubKey.toStdString().c_str(), drivePubKey.size(), driveKey);

    auto storagePaymentTransaction = xpx_chain_sdk::CreateStoragePaymentTransaction(driveKey, amount, std::nullopt, std::nullopt, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(storagePaymentTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> storagePaymentNotifier;

    statusNotifier.set([this, storagePaymentNotifierId = storagePaymentNotifier.getId(), driveId](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        qWarning() << LOG_SOURCE << notification.status.c_str() << " hash: " << notification.hash.c_str();

        removeConfirmedAddedNotifier(mpChainAccount->address(), storagePaymentNotifierId);
        removeStatusNotifier(mpChainAccount->address(), id);

        emit storagePaymentFailed(driveId, notification.status.c_str());
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << LOG_SOURCE << errorCode.message().c_str(); });

    const std::string hash = rawHashToHex(storagePaymentTransaction->hash()).toStdString();
    storagePaymentNotifier.set([this, hash, statusNotifierId = statusNotifier.getId(), driveId](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << LOG_SOURCE << "confirmed storagePaymentTransaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit storagePaymentConfirmed(driveId);
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { storagePaymentNotifier },
                                                               [this, data = storagePaymentTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}

std::string TransactionsEngine::addDrive(const std::string& driveAlias, const uint64_t& driveSize, const ushort replicatorsCount) {
    xpx_chain_sdk::Amount verificationFeeAmount = 100;
    auto prepareDriveTransaction = xpx_chain_sdk::CreatePrepareBcDriveTransaction(driveSize, verificationFeeAmount, replicatorsCount, std::nullopt, std::nullopt, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(prepareDriveTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> prepareDriveNotifier;

    std::string hash = rawHashToHex(prepareDriveTransaction->hash()).toStdString();
    statusNotifier.set([this, hash, driveAlias, prepareDriveNotifierId = prepareDriveNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (notification.hash == hash) {
            qWarning() << LOG_SOURCE << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), prepareDriveNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit createDriveFailed(driveAlias, rawHashFromHex(notification.hash.c_str()), notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << LOG_SOURCE << errorCode.message().c_str();
    });

    prepareDriveNotifier.set([this, hash, driveAlias, statusNotifierId = statusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << LOG_SOURCE << "confirmed PrepareDrive transaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit createDriveConfirmed(driveAlias, rawHashFromHex(notification.meta.hash.c_str()));
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { prepareDriveNotifier },
                                                               [this, data = prepareDriveTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
    return hash;
}

void TransactionsEngine::closeDrive(const std::array<uint8_t, 32>& rawDrivePubKey) {
    QString drivePubKey = rawHashToHex(rawDrivePubKey);

    xpx_chain_sdk::Key driveKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(drivePubKey.toStdString().c_str(), drivePubKey.size(), driveKey);

    auto driveClosureTransaction = xpx_chain_sdk::CreateDriveClosureTransaction(driveKey, std::nullopt, std::nullopt, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(driveClosureTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> driveClosureNotifier;

    const std::string hash = rawHashToHex(driveClosureTransaction->hash()).toStdString();
    statusNotifier.set([this, hash, drivePubKey, rawDrivePubKey, driveClosureNotifierId = driveClosureNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (notification.hash == hash) {
            qWarning() << LOG_SOURCE << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), driveClosureNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit closeDriveFailed(rawDrivePubKey, notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << LOG_SOURCE << errorCode.message().c_str();
    });

    driveClosureNotifier.set([this, hash, rawDrivePubKey, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << LOG_SOURCE << "confirmed driveClosureTransaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit closeDriveConfirmed(rawDrivePubKey);
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { driveClosureNotifier },
                                                               [this, data = driveClosureTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}

void TransactionsEngine::cancelDataModification(const std::array<uint8_t, 32> &driveKey) {
    mpBlockchainEngine->getDriveById(rawHashToHex(driveKey).toStdString(), [this, driveKey](xpx_chain_sdk::Drive drive, auto isSuccess, auto message, auto code){
        if (!isSuccess) {
            qWarning() << LOG_SOURCE << "message: " << message.c_str() << " code: " << code.c_str();
            return;
        }

        if (drive.data.activeDataModifications.empty() ) {
            qInfo() << LOG_SOURCE << "not active modifications found!";
            return;
        }

        QString drivePubKey = rawHashToHex(driveKey);

        xpx_chain_sdk::Key rawDriveKey;
        xpx_chain_sdk::ParseHexStringIntoContainer(drivePubKey.toStdString().c_str(), drivePubKey.size(), rawDriveKey);

        auto lastModification = drive.data.activeDataModifications.size() - 1;
        const QString modificationHex = drive.data.activeDataModifications[lastModification].dataModification.id.c_str();

        xpx_chain_sdk::Hash256 modificationHash256;
        xpx_chain_sdk::ParseHexStringIntoContainer(modificationHex.toStdString().c_str(), modificationHex.size(), modificationHash256);

        auto dataModificationCancelTransaction = xpx_chain_sdk::CreateDataModificationCancelTransaction(rawDriveKey, modificationHash256,
                                                                                                        std::nullopt, std::nullopt, mpChainClient->getConfig().NetworkId);
        mpChainAccount->signTransaction(dataModificationCancelTransaction.get());

        xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
        xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> dataModificationCancelNotifier;

        statusNotifier.set([this, driveKey, dataModificationCancelNotifierId = dataModificationCancelNotifier.getId(), modificationHex](
                const auto& id,
                const xpx_chain_sdk::TransactionStatusNotification& notification) {
            qWarning() << LOG_SOURCE << " onCancelDataModification: " << notification.status.c_str() << " : " << notification.status.c_str() << " transactionId: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), dataModificationCancelNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit cancelModificationFailed(driveKey,modificationHex);
        });

        mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {},
                                                           [](auto errorCode) {qCritical() << LOG_SOURCE << errorCode.message().c_str(); });

        const std::string hash = rawHashToHex(dataModificationCancelTransaction->hash()).toStdString();
        dataModificationCancelNotifier.set([this, hash, statusNotifierId = statusNotifier.getId(), driveKey, modificationHex](
                const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
            if (notification.meta.hash == hash) {
                qInfo() << LOG_SOURCE << "confirmed dataModificationCancelTransaction hash: " << hash.c_str();

                removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
                removeConfirmedAddedNotifier(mpChainAccount->address(), id);

                emit cancelModificationConfirmed(driveKey, modificationHex);
            }
        });

        mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { dataModificationCancelNotifier },
                                                                   [this, data = dataModificationCancelTransaction->binary()]() { announceTransaction(data); },
                                                                   [this, hash](auto error) { onError(hash, error); });
    });
}

void TransactionsEngine::applyDataModification(const std::array<uint8_t, 32>& driveId,
                                               const sirius::drive::ActionList& actions,
                                               const std::string& sandboxFolder,
                                               const std::vector<xpx_chain_sdk::Address>& replicators) {
    if (!isValidHash(driveId)) {
        qWarning() << LOG_SOURCE << "bad driveId: empty!";
        return;
    }

    auto callback = [this, driveId, actions, sandboxFolder, replicators](auto totalModifyDataSize, auto infoHash) {
        if (!isValidHash(infoHash)) {
            qWarning() << LOG_SOURCE << "invalid info hash: " << rawHashToHex(infoHash);
            return;
        }

        sendModification(driveId, infoHash, actions, totalModifyDataSize, sandboxFolder, replicators);
    };

    emit addActions(actions, driveId, sandboxFolder, callback);
}

bool TransactionsEngine::isValidHash(const std::array<uint8_t, 32> &hash) {
    bool result = true;
    if (hash.empty()) {
        result = false;
    }

    int counter = 0;
    for (const auto& c : hash) {
        if (c == 0) {
            counter++;
        }
    }

    if (counter == hash.size()) {
        result = false;
    }

    return result;
}

void TransactionsEngine::sendModification(const std::array<uint8_t, 32>& driveId,
                                          const std::array<uint8_t, 32>& infoHash,
                                          const sirius::drive::ActionList& actions,
                                          const uint64_t totalModifyDataSize,
                                          const std::string& sandboxFolder,
                                          const std::vector<xpx_chain_sdk::Address>& replicators) {
    const QString actionListFileName = rawHashToHex(infoHash);
    qInfo() << LOG_SOURCE << "action list downloadDataCDI: " << actionListFileName;

    const QString downloadDataCDIHex = rawHashToHex(infoHash);

    xpx_chain_sdk::Hash256 downloadDataCdi;
    xpx_chain_sdk::ParseHexStringIntoContainer(downloadDataCDIHex.toStdString().c_str(), downloadDataCDIHex.size(), downloadDataCdi);

    const QString pathToActionList = findFile(actionListFileName, sandboxFolder.c_str());
    if (pathToActionList.isEmpty()) {
        qWarning() << LOG_SOURCE << " actionList.bin not found: " << pathToActionList;
        return;
    } else {
        qInfo() << LOG_SOURCE << " actionList.bin found: " << pathToActionList;
    }

    for (int i = 0; i < actions.size(); i++) {
        if (actions[i].m_actionId == sirius::drive::action_list_id::upload) {
            const QString path = actions[i].m_param1.c_str();
            qInfo() << LOG_SOURCE << "path for upload: " << path;

            if (!QFile::exists(path)) {
                qWarning() << LOG_SOURCE << "local file not found: " << path;
            }
        }
    }

    // TODO pass from dialog
    const xpx_chain_sdk::Amount feedbackFeeAmount = 10;

    uint64_t uploadSize = 0;
    uint64_t bytesPerMegabyte = 1024 * 1024;
    if (totalModifyDataSize > 0) {
        // integer ceil
        uploadSize = (totalModifyDataSize - 1) / (bytesPerMegabyte) + 1;
    }

    qInfo() << LOG_SOURCE << "Upload Size: " << uploadSize << " total modify data size: " << totalModifyDataSize;

    const QString driveKeyHex = rawHashToHex(driveId);

    xpx_chain_sdk::Key driveKeyRaw;
    xpx_chain_sdk::ParseHexStringIntoContainer(driveKeyHex.toStdString().c_str(), driveKeyHex.size(), driveKeyRaw);

    auto dataModificationTransaction = xpx_chain_sdk::CreateDataModificationTransaction(driveKeyRaw, downloadDataCdi, uploadSize, feedbackFeeAmount,
                                                                                        std::nullopt, std::nullopt, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(dataModificationTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> dataModificationNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> dataModificationApprovalTransactionNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> replicatorsStatusNotifier;

    const std::string hash = rawHashToHex(dataModificationTransaction->hash()).toStdString();
    std::array<uint8_t, 32> modificationId = rawHashFromHex(hash.c_str());
    emit modificationCreated(driveKeyHex, modificationId);

    statusNotifier.set([this, driveId, hash, modificationId, driveKeyHex, replicators,
                        dataModificationNotifierId = dataModificationNotifier.getId(),
                        approvalNotifierId = dataModificationApprovalTransactionNotifier.getId(),
                        statusNotifierId = replicatorsStatusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (notification.hash == hash) {
            removeUnconfirmedAddedNotifier(mpChainAccount->address(), dataModificationNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            unsubscribeFromReplicators(replicators, approvalNotifierId, statusNotifierId);

            qWarning() << LOG_SOURCE << "drive key: " + driveKeyHex << " : " << notification.status.c_str() << " transactionId: " << notification.hash.c_str();
            emit dataModificationFailed(driveId,modificationId);
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {},
                                                       [](auto errorCode) {qCritical() << LOG_SOURCE << errorCode.message().c_str(); });

    dataModificationNotifier.set([this, driveId, hash, statusNotifierId = statusNotifier.getId(), modificationId] (
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << LOG_SOURCE << "confirmed dataModificationTransaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeUnconfirmedAddedNotifier(mpChainAccount->address(), id);

            emit dataModificationConfirmed(driveId,modificationId);
        }
    });

    replicatorsStatusNotifier.set([](const auto&, const xpx_chain_sdk::TransactionStatusNotification& n) {
        qInfo() << LOG_SOURCE << "(replicators) transaction status notification is received : " << n.status.c_str() << n.hash.c_str();
    });

    dataModificationApprovalTransactionNotifier.set([
            this, modificationId, driveId, pathToActionList, replicators, statusNotifierId = replicatorsStatusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (xpx_chain_sdk::TransactionType::Data_Modification_Approval != notification.data.type) {
            qInfo() << LOG_SOURCE << "other transactions confirmed (skip!) : " << rawHashToHex(modificationId);
            return;
        }

        qInfo() << LOG_SOURCE << "confirmed DataModificationApproval transaction hash: " << notification.meta.hash.c_str();
        if (!mDataModificationApprovals.contains(driveId)) {
            mDataModificationApprovals.insert({ driveId, {} });
        }

        if (mDataModificationApprovals[driveId].contains(modificationId)) {
            qWarning() << LOG_SOURCE << "modificationId already exists (skip!) : " << rawHashToHex(modificationId);
            return;
        }

        // TODO: fix libtorrent logic to avoid this
        std::this_thread::sleep_for(std::chrono::seconds(30));

        mpBlockchainEngine->getTransactionInfo(xpx_chain_sdk::Confirmed,
                                               notification.meta.hash,
                                               [this, driveId, modificationId, notification,
                                                id, replicators, statusNotifierId](auto transaction, auto isSuccess, auto message, auto code) {
            if (!isSuccess) {
                qWarning() << LOG_SOURCE << "message: " << message.c_str() << " code: " << code.c_str();
                return;
            }

            if (!transaction) {
                qWarning() << LOG_SOURCE << "bad pointer to dataModificationApprovalTransaction info";
                return;
            }

            auto dataModificationApprovalTransaction = reinterpret_cast<xpx_chain_sdk::transactions_info::DataModificationApprovalTransaction*>(transaction.get());
            if (!dataModificationApprovalTransaction) {
                qWarning() << LOG_SOURCE << "bad pointer to dataModificationApprovalTransaction";
                return;
            }

            if (dataModificationApprovalTransaction->driveKey != rawHashToHex(driveId).toStdString()) {
                qInfo() << LOG_SOURCE << "dataModificationApprovalTransaction is received for another drive: " << dataModificationApprovalTransaction->driveKey.c_str();
                return;
            }

            if (dataModificationApprovalTransaction->dataModificationId != rawHashToHex(modificationId).toStdString()) {
                qInfo() << LOG_SOURCE << "other dataModificationApprovalTransaction hash: " << rawHashToHex(modificationId);
                return;
            }

            qInfo() << LOG_SOURCE << "confirmed dataModificationApprovalTransaction hash: "
                    << notification.meta.hash.c_str() << " modificationId: "
                    << rawHashToHex(modificationId) << " fileStructureCdi: "
                    << dataModificationApprovalTransaction->fileStructureCdi.c_str();

            mDataModificationApprovals[driveId].insert(modificationId);

            // TODO: auto update drive structure (temporary for demo)
            emit dataModificationApprovalConfirmed(driveId, dataModificationApprovalTransaction->fileStructureCdi);

            unsubscribeFromReplicators(replicators, id, statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);
            // TODO:
            //removeDriveModifications(pathToActionList);
        });
    });

    subscribeOnReplicators(replicators, dataModificationApprovalTransactionNotifier, replicatorsStatusNotifier);

    auto approvalNotifierId = dataModificationApprovalTransactionNotifier.getId();
    auto statusNotifierId = replicatorsStatusNotifier.getId();
    mpChainClient->notifications()->addUnconfirmedAddedNotifiers(mpChainAccount->address(), { dataModificationNotifier },
                                                                [this, data = dataModificationTransaction->binary()]() { announceTransaction(data); },
                                                                [this, hash, replicators, approvalNotifierId, statusNotifierId](auto error) {
        onError(hash, error);
        unsubscribeFromReplicators(replicators, approvalNotifierId, statusNotifierId); });
}

void TransactionsEngine::replicatorOnBoarding(const QString& replicatorPrivateKey, const uint64_t capacityMB) {
    auto replicatorAccount = std::make_shared<xpx_chain_sdk::Account>([privateKey = replicatorPrivateKey.toStdString()](auto reason, auto param) {
        xpx_chain_sdk::Key key;
        ParseHexStringIntoContainer(privateKey.c_str(), privateKey.size(), key);
        return xpx_chain_sdk::PrivateKey(key.data(), key.size());
    }, mpChainClient->getConfig().NetworkId);

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> onBoardingNotifier;

    statusNotifier.set([this, replicatorAccount, confirmedAddedNotifierId = onBoardingNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionStatusNotification& notification) {
        removeConfirmedAddedNotifier(replicatorAccount->address(), confirmedAddedNotifierId);
        removeStatusNotifier(replicatorAccount->address(), id);

        auto replicatorPublicKey = rawHashToHex(replicatorAccount->publicKey());
        qWarning() << LOG_SOURCE << "replicator key: " + replicatorPublicKey << " : " << notification.status.c_str() << " transactionId: " << notification.hash.c_str();
        emit replicatorOnBoardingFailed(replicatorPublicKey);
    });

    mpChainClient->notifications()->addStatusNotifiers(replicatorAccount->address(), { statusNotifier }, {},
                                                       [](auto errorCode) {qCritical() << LOG_SOURCE << errorCode.message().c_str(); });

    auto replicatorOnBoardingTransaction = xpx_chain_sdk::CreateReplicatorOnboardingTransaction(xpx_chain_sdk::Amount(capacityMB),
                                                                                                std::nullopt, std::nullopt, mpChainClient->getConfig().NetworkId);
    replicatorAccount->signTransaction(replicatorOnBoardingTransaction.get());

    const std::string onBoardingTransactionHash = rawHashToHex(replicatorOnBoardingTransaction->hash()).toStdString();
    onBoardingNotifier.set([this, onBoardingTransactionHash, replicatorAccount, statusNotifierId = statusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == onBoardingTransactionHash) {
            qInfo() << LOG_SOURCE << "confirmed replicatorOnBoardingTransaction hash: " << onBoardingTransactionHash.c_str();

            removeConfirmedAddedNotifier(replicatorAccount->address(), id);
            removeStatusNotifier(replicatorAccount->address(), statusNotifierId);

            emit replicatorOnBoardingConfirmed(rawHashToHex(replicatorAccount->publicKey()));
        }
    });

    const std::string hash = rawHashToHex(replicatorOnBoardingTransaction->hash()).toStdString();
    mpChainClient->notifications()->addConfirmedAddedNotifiers(replicatorAccount->address(), { onBoardingNotifier },
                                                               [this, data = replicatorOnBoardingTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}

void TransactionsEngine::replicatorOffBoarding(const std::array<uint8_t, 32> &driveId, const QString& replicatorPrivateKey) {
    auto driveIdHex = rawHashToHex(driveId).toStdString();
    if (!isValidHash(driveId)) {
        qWarning() << LOG_SOURCE << "invalid hash: " << driveIdHex.c_str();
        return;
    }

    if (replicatorPrivateKey.isEmpty()) {
        qWarning() << LOG_SOURCE << "invalid replicator private key: " << driveIdHex.c_str();
        return;
    }

    xpx_chain_sdk::Key rawDriveKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(driveIdHex.c_str(), driveIdHex.size(), rawDriveKey);

    auto offBoardingTransaction = xpx_chain_sdk::CreateReplicatorOffboardingTransaction(rawDriveKey, std::nullopt, std::nullopt, mpChainClient->getConfig().NetworkId);
    auto replicatorAccount = std::make_shared<xpx_chain_sdk::Account>([privateKey = replicatorPrivateKey.toStdString()](auto reason, auto param) {
        xpx_chain_sdk::Key key;
        ParseHexStringIntoContainer(privateKey.c_str(), privateKey.size(), key);
        return xpx_chain_sdk::PrivateKey(key.data(), key.size());
    }, mpChainClient->getConfig().NetworkId);

    replicatorAccount->signTransaction(offBoardingTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> offBoardingNotifier;

    statusNotifier.set([this, replicatorAccount, offBoardingNotifierId = offBoardingNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionStatusNotification& notification) {
        removeConfirmedAddedNotifier(replicatorAccount->address(), offBoardingNotifierId);
        removeStatusNotifier(replicatorAccount->address(), id);

        auto replicatorPublicKey = rawHashToHex(replicatorAccount->publicKey());
        qWarning() << LOG_SOURCE << "replicator key: " + replicatorPublicKey << " : " << notification.status.c_str() << " transactionId: " << notification.hash.c_str();
        emit replicatorOffBoardingFailed(replicatorPublicKey);
    });

    mpChainClient->notifications()->addStatusNotifiers(replicatorAccount->address(), { statusNotifier }, {},
                                                       [](auto errorCode) {qCritical() << LOG_SOURCE << errorCode.message().c_str(); });

    // Success
    const std::string hash = rawHashToHex(offBoardingTransaction->hash()).toStdString();

    offBoardingNotifier.set([this, hash, replicatorAccount, statusNotifierId = statusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << LOG_SOURCE << "confirmed offBoardingTransaction hash: " << hash.c_str();

            removeStatusNotifier(replicatorAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(replicatorAccount->address(), id);

            auto replicatorPublicKey = rawHashToHex(replicatorAccount->publicKey());
            emit replicatorOffBoardingConfirmed(replicatorPublicKey);
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(replicatorAccount->address(), { offBoardingNotifier },
                                                               [this, data = offBoardingTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}

void TransactionsEngine::removeConfirmedAddedNotifier(const xpx_chain_sdk::Address& address,
                                                      const std::string &id,
                                                      std::function<void()> onSuccess,
                                                      std::function<void(boost::beast::error_code)> onError) {
    mpChainClient->notifications()->removeConfirmedAddedNotifiers(address, onSuccess, onError ? onError : [](auto code) {
        qWarning() << LOG_SOURCE << code.message().c_str();
    }, { id });
}

void TransactionsEngine::removeUnconfirmedAddedNotifier(const xpx_chain_sdk::Address &address,
                                                        const std::string &id,
                                                        std::function<void()> onSuccess,
                                                        std::function<void(boost::beast::error_code)> onError) {
    mpChainClient->notifications()->removeUnconfirmedAddedNotifiers(address, onSuccess, onError ? onError : [](auto code) {
        qWarning() << LOG_SOURCE << code.message().c_str();
    }, { id });
}

void TransactionsEngine::removeStatusNotifier(const xpx_chain_sdk::Address& address,
                                              const std::string &id,
                                              std::function<void()> onSuccess,
                                              std::function<void(boost::beast::error_code)> onError) {
    mpChainClient->notifications()->removeStatusNotifiers(address, onSuccess, onError ? onError : [](auto code) {
        qWarning() << LOG_SOURCE << code.message().c_str();
    }, { id });
}

void TransactionsEngine::announceTransaction(const std::vector<uint8_t> &data) {
    mpBlockchainEngine->announceNewTransaction(data, [](auto isAnnounced, auto isSuccess, auto message, auto code){
        if (!isSuccess) {
            qWarning() << LOG_SOURCE << message.c_str() << " : " << code.c_str();
        }
    });
}

void TransactionsEngine::onError(const std::string& transactionId, boost::beast::error_code errorCode) {
    qCritical() << LOG_SOURCE << errorCode.message().c_str() << " hash: " << transactionId.c_str();
}

QString TransactionsEngine::findFile(const QString& fileName, const QString& directory) {
    QFileInfoList hitList;
    QDirIterator it(directory, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString filename = it.next();
        QFileInfo file(filename);

        if (file.isDir()) {
            continue;
        }

        if (file.fileName().toLower() == fileName.toLower()) {
            hitList.append(file);
        }
    }

    return hitList.empty() ? "" : hitList[0].absoluteFilePath();
}

void TransactionsEngine::subscribeOnReplicators(
        const std::vector<xpx_chain_sdk::Address>& addresses,
        const xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification>& notifier,
        const xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification>& statusNotifier) {
    for (const auto& address : addresses) {
        mpChainClient->notifications()->addStatusNotifiers(address, { statusNotifier }, {}, [](auto errorCode) { qCritical() << LOG_SOURCE << errorCode.message().c_str(); });
        mpChainClient->notifications()->addConfirmedAddedNotifiers(address, { notifier }, {}, [](auto errorCode) { qCritical() << LOG_SOURCE << errorCode.message().c_str(); });
    }
}

void TransactionsEngine::unsubscribeFromReplicators(const std::vector<xpx_chain_sdk::Address>& addresses,
                                                    const std::string& notifierId,
                                                    const std::string& statusNotifierId) {
    for (const auto& address : addresses) {
        removeStatusNotifier(address, statusNotifierId);
        removeConfirmedAddedNotifier(address, notifierId);
    }
}
