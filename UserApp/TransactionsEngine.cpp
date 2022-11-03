#include "TransactionsEngine.h"
#include "Utils.h"

#include <qdebug.h>
#include <QFile>
#include <QFileInfoList>
#include <QDirIterator>

TransactionsEngine::TransactionsEngine(std::shared_ptr<xpx_chain_sdk::IClient> client,
                                       std::shared_ptr<xpx_chain_sdk::Account> account,
                                       std::shared_ptr<BlockchainEngine> blockchainEngine)
    : mpChainClient(client)
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
                                                                        std::nullopt, std::nullopt, mpChainClient->getConfig()->NetworkId);
    mpChainAccount->signTransaction(downloadTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> downloadNotifier;

    std::string hash = rawHashToHex(downloadTransaction->hash()).toStdString();
    statusNotifier.set([this, hash, downloadNotifierId = downloadNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (notification.hash == hash) {
            qWarning() << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), downloadNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit createDownloadChannelFailed(notification.hash.c_str(), notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << errorCode.message().c_str();
    });

    downloadNotifier.set([this, channelAlias, hash, rawDrivePubKey, statusNotifierId = statusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << "confirmed downloadTransaction hash: " << hash.c_str();

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
        qWarning() << "bad channelId: empty!";
        return;
    }

    xpx_chain_sdk::Key channelIdKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(channelIdHex.toStdString().c_str(), channelIdHex.size(), channelIdKey);

    const xpx_chain_sdk::Amount feedbackFeeAmount = 10;
    auto finishDownloadTransaction = xpx_chain_sdk::CreateFinishDownloadTransaction(channelIdKey, feedbackFeeAmount, std::nullopt, std::nullopt, mpChainClient->getConfig()->NetworkId);
    mpChainAccount->signTransaction(finishDownloadTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> finishDownloadNotifier;

    const std::string hash = rawHashToHex(finishDownloadTransaction->hash()).toStdString();
    statusNotifier.set([this, hash, finishDownloadNotifierId = finishDownloadNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (notification.hash == hash) {
            qWarning() << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), finishDownloadNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit closeDownloadChannelFailed(notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << errorCode.message().c_str();
    });

    finishDownloadNotifier.set([this, hash, channelId, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << "confirmed finishDownloadTransaction hash: " << hash.c_str();

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
                                                                                      std::nullopt, std::nullopt, mpChainClient->getConfig()->NetworkId);
    mpChainAccount->signTransaction(downloadPaymentTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> downloadPaymentNotifier;

    const std::string hash = rawHashToHex(downloadPaymentTransaction->hash()).toStdString();
    statusNotifier.set([this, hash, downloadPaymentNotifierId = downloadPaymentNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (notification.hash == hash) {
            qWarning() << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), downloadPaymentNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit downloadPaymentFailed(notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [this](auto errorCode) {
        qCritical() << errorCode.message().c_str();
    });

    downloadPaymentNotifier.set([this, hash, channelId, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << "confirmed downloadPaymentTransaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit downloadPaymentConfirmed(channelId);
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { downloadPaymentNotifier },
                                                               [this, data = downloadPaymentTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}

void TransactionsEngine::addDrive(const std::string& driveAlias, const uint64_t& driveSize, const ushort replicatorsCount) {
    xpx_chain_sdk::Amount verificationFeeAmount = 100;
    auto prepareDriveTransaction = xpx_chain_sdk::CreatePrepareBcDriveTransaction(driveSize, verificationFeeAmount, replicatorsCount, std::nullopt, std::nullopt, mpChainClient->getConfig()->NetworkId);
    mpChainAccount->signTransaction(prepareDriveTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> prepareDriveNotifier;

    const std::string hash = rawHashToHex(prepareDriveTransaction->hash()).toStdString();
    statusNotifier.set([this, hash, driveAlias, prepareDriveNotifierId = prepareDriveNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (notification.hash == hash) {
            qWarning() << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), prepareDriveNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit createDriveFailed(driveAlias, rawHashFromHex(notification.hash.c_str()), notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << errorCode.message().c_str();
    });

    prepareDriveNotifier.set([this, hash, driveAlias, statusNotifierId = statusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << "confirmed PrepareDrive transaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit createDriveConfirmed(driveAlias, rawHashFromHex(notification.meta.hash.c_str()));
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { prepareDriveNotifier },
                                                               [this, data = prepareDriveTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}

void TransactionsEngine::closeDrive(const std::array<uint8_t, 32>& rawDrivePubKey) {
    QString drivePubKey = rawHashToHex(rawDrivePubKey);

    xpx_chain_sdk::Key driveKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(drivePubKey.toStdString().c_str(), drivePubKey.size(), driveKey);

    auto driveClosureTransaction = xpx_chain_sdk::CreateDriveClosureTransaction(driveKey, std::nullopt, std::nullopt, mpChainClient->getConfig()->NetworkId);
    mpChainAccount->signTransaction(driveClosureTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> driveClosureNotifier;

    const std::string hash = rawHashToHex(driveClosureTransaction->hash()).toStdString();
    statusNotifier.set([this, hash, drivePubKey, driveClosureNotifierId = driveClosureNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (notification.hash == hash) {
            qWarning() << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), driveClosureNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit closeDriveFailed(notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << errorCode.message().c_str();
    });

    driveClosureNotifier.set([this, hash, rawDrivePubKey, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << "confirmed driveClosureTransaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit closeDriveConfirmed(rawDrivePubKey);
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { driveClosureNotifier },
                                                               [this, data = driveClosureTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}

void TransactionsEngine::applyDataModification(const std::array<uint8_t, 32>& driveKey,
                                               const sirius::drive::ActionList& actions,
                                               const std::array<uint8_t, 32> &channelId,
                                               const std::string& sandboxFolder) {
    if (!isValidHash(driveKey)) {
        qWarning() << "bad driveId: empty!";
        return;
    }

    if (!isValidHash(channelId)) {
        qWarning() << "bad channelId: empty!";
        return;
    }

    auto callback = [this, driveKey, channelId, actions, sandboxFolder](uint64_t totalModifyDataSize, const std::array<uint8_t, 32>& infoHash) {
        if (!isValidHash(infoHash)) {
            qWarning() << "invalid info hash: " << rawHashToHex(infoHash);
            return;
        }

        sendModification(driveKey, channelId, infoHash, actions, totalModifyDataSize, sandboxFolder);
    };

    // mPathToModifications modify_drive_data
    //emit addActions(actions, driveKey, sandboxFolder, callback);
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
                                          const std::array<uint8_t, 32>& channelId,
                                          const std::array<uint8_t, 32>& infoHash,
                                          const sirius::drive::ActionList& actions,
                                          const uint64_t totalModifyDataSize,
                                          const std::string& sandboxFolder) {
    const QString actionListFileName = rawHashToHex(infoHash);
    qInfo() << "action list downloadDataCDI: " << actionListFileName;

    const QString downloadDataCDIHex = rawHashToHex(infoHash);

    xpx_chain_sdk::Hash256 downloadDataCdi;
    xpx_chain_sdk::ParseHexStringIntoContainer(downloadDataCDIHex.toStdString().c_str(), downloadDataCDIHex.size(), downloadDataCdi);

    const QString pathToActionList = findFile(actionListFileName, sandboxFolder.c_str());
    if (pathToActionList.isEmpty()) {
        qWarning() << " actionList.bin not found: " << pathToActionList;
        return;
    } else {
        qInfo() << " actionList.bin found: " << pathToActionList;
    }

    for (int i = 0; i < actions.size(); i++) {
        if (actions[i].m_actionId == sirius::drive::action_list_id::upload) {
            const QString path = actions[i].m_param1.c_str();
            qInfo() << "path for upload: " << path;

            if (!QFile::exists(path)) {
                qWarning() << "local file not found: " << path;
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

    qInfo() << "Upload Size: " << uploadSize << " total modify data size: " << totalModifyDataSize;

    const QString driveKeyHex = rawHashToHex(driveId);

    xpx_chain_sdk::Key driveKeyRaw;
    xpx_chain_sdk::ParseHexStringIntoContainer(driveKeyHex.toStdString().c_str(), driveKeyHex.size(), driveKeyRaw);

    auto dataModificationTransaction = xpx_chain_sdk::CreateDataModificationTransaction(driveKeyRaw, downloadDataCdi, uploadSize, feedbackFeeAmount,
                                                                                        std::nullopt, std::nullopt, mpChainClient->getConfig()->NetworkId);
    mpChainAccount->signTransaction(dataModificationTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> dataModificationNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> dataModificationApprovalTransactionNotifier;

    const std::string hash = rawHashToHex(dataModificationTransaction->hash()).toStdString();
    std::array<uint8_t, 32> modificationId = rawHashFromHex(hash.c_str());

    statusNotifier.set([this, hash, modificationId, driveKeyHex, dataModificationNotifierId = dataModificationNotifier.getId(), approvalId = dataModificationApprovalTransactionNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (notification.hash == hash) {
            removeConfirmedAddedNotifier(mpChainAccount->address(), dataModificationNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            //unsubscribeFromReplicators(rawDrivePubKey, approvalId);

            qWarning() << "drive key: " + driveKeyHex << " : " << notification.status.c_str() << " transactionId: " << notification.hash.c_str();
            emit dataModificationFailed(modificationId);
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {},
                                                       [](auto errorCode) {qCritical() << errorCode.message().c_str(); });

    dataModificationNotifier.set([this, hash, statusNotifierId = statusNotifier.getId(), modificationId] (
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << "confirmed dataModificationTransaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit dataModificationConfirmed(modificationId);
        }
    });

    dataModificationApprovalTransactionNotifier.set([
            this, modificationId, driveId, channelId, pathToActionList](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (xpx_chain_sdk::TransactionType::Data_Modification_Approval != notification.data.type) {
            qInfo() << "other transactions confirmed (skip!) : " << rawHashToHex(modificationId);
            return;
        }

        qInfo() << "confirmed DataModificationApproval transaction hash: " << notification.meta.hash.c_str();
        if (!mDataModificationApprovals.contains(driveId)) {
            mDataModificationApprovals.insert({ driveId, {} });
        }

        if (mDataModificationApprovals[driveId].contains(modificationId)) {
            qWarning() << "modificationId already exists (skip!) : " << rawHashToHex(modificationId);
            return;
        }

        mpBlockchainEngine->getTransactionInfo(xpx_chain_sdk::Confirmed,
                                               notification.meta.hash,
                                               [this, driveId, modificationId, notification, id, channelId](auto transaction, auto isSuccess, auto message, auto code) {
            if (!isSuccess) {
                qWarning() << "message: " << message.c_str() << " code: " << code.c_str();
                return;
            }

            if (!transaction) {
                qWarning() << "bad pointer to dataModificationApprovalTransaction info";
                return;
            }

            auto dataModificationApprovalTransaction = reinterpret_cast<xpx_chain_sdk::transactions_info::DataModificationApprovalTransaction*>(transaction.get());
            if (!dataModificationApprovalTransaction) {
                qWarning() << "bad pointer to dataModificationApprovalTransaction";
                return;
            }

            if (dataModificationApprovalTransaction->driveKey != rawHashToHex(driveId).toStdString()) {
                qInfo() << "dataModificationApprovalTransaction is received for another drive: " << dataModificationApprovalTransaction->driveKey.c_str();
                return;
            }

            if (dataModificationApprovalTransaction->dataModificationId != rawHashToHex(modificationId).toStdString()) {
                qInfo() << "other dataModificationApprovalTransaction hash: " << rawHashToHex(modificationId);
                return;
            }

            // TODO: fix libtorrent logic to avoid this
            std::this_thread::sleep_for(std::chrono::seconds(30));

            qInfo() << "confirmed dataModificationApprovalTransaction hash: "
                    << notification.meta.hash.c_str() << " modificationId: "
                    << rawHashToHex(modificationId) << " fileStructureCdi: "
                    << dataModificationApprovalTransaction->fileStructureCdi.c_str();

            mDataModificationApprovals[driveId].insert(modificationId);

            //emit balancesUpdated();
            // TODO: auto update drive structure (temporary for demo)
            emit dataModificationApprovalConfirmed(driveId, channelId, dataModificationApprovalTransaction->fileStructureCdi);

            //unsubscribeFromReplicators(driveKey, id);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);
            //removeDriveModifications(pathToActionList);
        });
    });

    //subscribeOnReplicators(driveKey, dataModificationApprovalTransactionNotifier);

    auto approvalId = dataModificationApprovalTransactionNotifier.getId();
    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { dataModificationNotifier },
                                                               [this, data = dataModificationTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash, approvalId](auto error) { onError(hash, error); /*unsubscribeFromReplicators(rawDrivePubKey, approvalId);*/ });
}

void TransactionsEngine::removeConfirmedAddedNotifier(const xpx_chain_sdk::Address& address,
                                                      const std::string &id,
                                                      std::function<void()> onSuccess,
                                                      std::function<void(boost::beast::error_code)> onError) {
    mpChainClient->notifications()->removeConfirmedAddedNotifiers(address, onSuccess, onError ? onError : [](auto code) {
        qWarning() << code.message().c_str();
    }, { id });
}

void TransactionsEngine::removeStatusNotifier(const xpx_chain_sdk::Address& address,
                                              const std::string &id,
                                              std::function<void()> onSuccess,
                                              std::function<void(boost::beast::error_code)> onError) {
    mpChainClient->notifications()->removeStatusNotifiers(address, onSuccess, onError ? onError : [](auto code) {
        qWarning() << code.message().c_str();
    }, { id });
}

void TransactionsEngine::announceTransaction(const std::vector<uint8_t> &data) {
    mpBlockchainEngine->announceNewTransaction(data, [](auto isAnnounced, auto isSuccess, auto message, auto code){
        if (!isSuccess) {
            qWarning() << message.c_str() << " : " << code.c_str();
        }
    });
}

void TransactionsEngine::onError(const std::string& transactionId, boost::beast::error_code errorCode) {
    qCritical() << errorCode.message().c_str() << " hash: " << transactionId.c_str();
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