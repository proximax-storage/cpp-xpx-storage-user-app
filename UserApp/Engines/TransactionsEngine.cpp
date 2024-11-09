#include "TransactionsEngine.h"
#include "Utils.h"

#include <qdebug.h>
#include <QFile>
#include <QFileInfoList>
#include <QDirIterator>
#include <drive/ModificationStatus.h>
#include <boost/algorithm/string/predicate.hpp>
#include <random>

TransactionsEngine::TransactionsEngine(std::shared_ptr<xpx_chain_sdk::IClient> client,
                                       std::shared_ptr<xpx_chain_sdk::Account> account,
                                       BlockchainEngine* blockchainEngine,
                                       QObject* parent)
    : QObject(parent)
    , mpChainClient(client)
    , mpBlockchainEngine(blockchainEngine)
    , mpChainAccount(account)
    , mSandbox(CLIENT_SANDBOX_FOLDER)
{}

void TransactionsEngine::addDownloadChannel(const std::string& channelAlias,
                                            const std::vector<std::array<uint8_t, 32>> &listOfAllowedPublicKeys,
                                            const std::array<uint8_t, 32> &rawDrivePubKey,
                                            const uint64_t &prepaidSize,
                                            const uint64_t &feedbacksNumber,
                                            const std::optional<xpx_chain_sdk::NetworkDuration>& deadline,
                                            const std::function<void(std::string hash)>& callback,
                                            const std::function<bool(const QString& transactionFee)>& confirmationCallback) {
    QString drivePubKey = rawHashToHex(rawDrivePubKey);

    xpx_chain_sdk::Key driveKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(drivePubKey.toStdString().c_str(), drivePubKey.size(), driveKey);

    std::vector<xpx_chain_sdk::Key> listOfAllowedPks;
    for (std::array<uint8_t, 32> key : listOfAllowedPublicKeys)
    {
        const QString keyHex = rawHashToHex(key);

        xpx_chain_sdk::Key newKey;
        xpx_chain_sdk::ParseHexStringIntoContainer(keyHex.toStdString().c_str(), keyHex.size(), newKey);
        listOfAllowedPks.emplace_back(newKey);
    }

    // TODO: fix empty list of keys
    auto downloadTransaction = xpx_chain_sdk::CreateDownloadTransaction(driveKey, prepaidSize, feedbacksNumber, listOfAllowedPks.size(), listOfAllowedPks,
                                                                        std::nullopt, deadline, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(downloadTransaction.get());

    if (confirmationCallback && !confirmationCallback(prettyBalance(downloadTransaction->maxFee()))) {
        qDebug () << "TransactionsEngine::addDownloadChannel: download channel creation was canceled by the user!";
        return;
    }

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> downloadNotifier;

    std::string hash = rawHashToHex(downloadTransaction->hash()).toStdString();
    qInfo() << "TransactionsEngine::addDownloadChannel. Drive id: " << drivePubKey << " transaction id: " << hash;

    statusNotifier.set([this, hash, downloadNotifierId = downloadNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (boost::iequals(notification.hash, hash)) {
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
        if (boost::iequals(notification.meta.hash, hash)) {
            qInfo() << LOG_SOURCE << "confirmed downloadTransaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit createDownloadChannelConfirmed(channelAlias, rawHashFromHex(hash.c_str()), rawDrivePubKey);
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { downloadNotifier },
                                                               [this, data = downloadTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });

    callback(hash);
}

void TransactionsEngine::closeDownloadChannel(const std::array<uint8_t, 32> &channelId, const std::optional<xpx_chain_sdk::NetworkDuration>& deadline) {
    QString channelIdHex = rawHashToHex(channelId);
    if( channelId.empty()) {
        qWarning() << "TransactionsEngine::closeDownloadChannel. Bad channelId: empty!";
        return;
    }

    xpx_chain_sdk::Key channelIdKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(channelIdHex.toStdString().c_str(), channelIdHex.size(), channelIdKey);

    const xpx_chain_sdk::Amount feedbackFeeAmount = 10;
    auto finishDownloadTransaction = xpx_chain_sdk::CreateFinishDownloadTransaction(channelIdKey, feedbackFeeAmount, std::nullopt, deadline, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(finishDownloadTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> finishDownloadNotifier;

    const std::string hash = rawHashToHex(finishDownloadTransaction->hash()).toStdString();
    qInfo() << "TransactionsEngine::closeDownloadChannel. Channel id: " << channelIdHex << " transaction id: " << hash;

    statusNotifier.set([this, hash, channelId, finishDownloadNotifierId = finishDownloadNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (boost::iequals(notification.hash, hash)) {
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
        if (boost::iequals(notification.meta.hash, hash)) {
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

void TransactionsEngine::downloadPayment(const std::array<uint8_t, 32> &channelId, uint64_t prepaidSize, const std::optional<xpx_chain_sdk::NetworkDuration>& deadline) {
    const QString channelIdHex = rawHashToHex(channelId);

    xpx_chain_sdk::Key channelIdKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(channelIdHex.toStdString().c_str(), channelIdHex.size(), channelIdKey);

    const xpx_chain_sdk::Amount feedbackFeeAmount = 10;

    auto downloadPaymentTransaction = xpx_chain_sdk::CreateDownloadPaymentTransaction(channelIdKey, prepaidSize, feedbackFeeAmount,
                                                                                      std::nullopt, deadline, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(downloadPaymentTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> downloadPaymentNotifier;

    const std::string hash = rawHashToHex(downloadPaymentTransaction->hash()).toStdString();
    qInfo() << "TransactionsEngine::downloadPayment. Channel id: " << channelIdHex << " prepaid size: " << prepaidSize << " transaction id: " << hash;

    statusNotifier.set([this, hash, downloadPaymentNotifierId = downloadPaymentNotifier.getId(), channelId](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (boost::iequals(notification.hash, hash)) {
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
        if (boost::iequals(notification.meta.hash, hash)) {
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

void TransactionsEngine::storagePayment(const std::array<uint8_t, 32> &driveId, const uint64_t& amount, const std::optional<xpx_chain_sdk::NetworkDuration>& deadline) {
    QString drivePubKey = rawHashToHex(driveId);

    xpx_chain_sdk::Key driveKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(drivePubKey.toStdString().c_str(), drivePubKey.size(), driveKey);

    auto storagePaymentTransaction = xpx_chain_sdk::CreateStoragePaymentTransaction(driveKey, amount, std::nullopt, deadline, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(storagePaymentTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> storagePaymentNotifier;

    const std::string hash = rawHashToHex(storagePaymentTransaction->hash()).toStdString();
    qInfo() << "TransactionsEngine::storagePayment. Drive key: " << drivePubKey << " amount: " << amount << " transaction id: " << hash;

    statusNotifier.set([this, hash, storagePaymentNotifierId = storagePaymentNotifier.getId(), driveId](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (boost::iequals(notification.hash, hash)) {
            qWarning() << "TransactionsEngine::storagePayment. Failed storage payment transaction: " << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), storagePaymentNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit storagePaymentFailed(driveId, notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << LOG_SOURCE << errorCode.message().c_str(); });

    storagePaymentNotifier.set([this, hash, statusNotifierId = statusNotifier.getId(), driveId](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (boost::iequals(notification.meta.hash, hash)) {
            qInfo() << "TransactionsEngine::storagePayment. Confirmed storage payment transaction, hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit storagePaymentConfirmed(driveId);
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { storagePaymentNotifier },
                                                               [this, data = storagePaymentTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}

void TransactionsEngine::addDrive(const uint64_t& driveSize,
                                  const ushort replicatorsCount,
                                  const std::optional<xpx_chain_sdk::NetworkDuration>& deadline,
                                  const std::function<void(std::string hash)>& callback,
                                  const std::function<bool(const QString& transactionFee)>& confirmationCallback) {
    xpx_chain_sdk::Amount verificationFeeAmount = 100;
    auto prepareDriveTransaction = xpx_chain_sdk::CreatePrepareBcDriveTransaction(driveSize, verificationFeeAmount, replicatorsCount, std::nullopt, deadline, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(prepareDriveTransaction.get());

    if (!confirmationCallback(prettyBalance(prepareDriveTransaction->maxFee()))) {
        qDebug () << "TransactionsEngine::addDrive: drive creation was canceled by the user!";
        return;
    }

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> prepareDriveNotifier;

    std::string hash = rawHashToHex(prepareDriveTransaction->hash()).toStdString();
    qInfo() << "TransactionsEngine::addDrive. Drive size: " << driveSize << " replicators count: " << replicatorsCount << " transaction id: " << hash << " trx fee: " << prettyBalance(prepareDriveTransaction->maxFee());

    statusNotifier.set([this, hash, prepareDriveNotifierId = prepareDriveNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (boost::iequals(notification.hash, hash)) {
            qWarning() << LOG_SOURCE << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), prepareDriveNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit createDriveFailed(rawHashFromHex(notification.hash.c_str()), notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << LOG_SOURCE << errorCode.message().c_str();
    });

    prepareDriveNotifier.set([this, hash, statusNotifierId = statusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (boost::iequals(notification.meta.hash, hash)) {
            qInfo() << LOG_SOURCE << "confirmed PrepareDrive transaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit createDriveConfirmed(rawHashFromHex(notification.meta.hash.c_str()));
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { prepareDriveNotifier },
                                                               [this, data = prepareDriveTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });

    callback(hash);
}

void TransactionsEngine::closeDrive(const std::array<uint8_t, 32>& rawDrivePubKey,
                                    const std::optional<xpx_chain_sdk::NetworkDuration>& deadline,
                                    const std::function<bool(const QString& transactionFee)>& callback) {
    QString drivePubKey = rawHashToHex(rawDrivePubKey);

    xpx_chain_sdk::Key driveKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(drivePubKey.toStdString().c_str(), drivePubKey.size(), driveKey);

    auto driveClosureTransaction = xpx_chain_sdk::CreateDriveClosureTransaction(driveKey, std::nullopt, deadline, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(driveClosureTransaction.get());

    if (!callback(prettyBalance(driveClosureTransaction->maxFee()))) {
        qDebug () << "TransactionsEngine::addDrive: Drive closing was canceled by the user!";
        return;
    }

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> driveClosureNotifier;

    const std::string hash = rawHashToHex(driveClosureTransaction->hash()).toStdString();
    qInfo() << "TransactionsEngine::closeDrive. Drive key: " << drivePubKey << " transaction id: " << hash;

    statusNotifier.set([this, hash, drivePubKey, rawDrivePubKey, driveClosureNotifierId = driveClosureNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (boost::iequals(notification.hash, hash)) {
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
        if (boost::iequals(notification.meta.hash, hash)) {
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

void TransactionsEngine::cancelDataModification(const xpx_chain_sdk::Drive& drive,
                                                const std::optional<xpx_chain_sdk::NetworkDuration>& deadline,
                                                const std::function<bool(const QString& transactionFee)>& callback) {
    if (drive.data.activeDataModifications.empty()) {
        qWarning() << "TransactionsEngine::cancelDataModification. Not active modifications found! Drive key: " << drive.data.multisig;
        return;
    }

    const unsigned long lastModificationIndex = drive.data.activeDataModifications.size() - 1;
    const auto currentModification = drive.data.activeDataModifications[lastModificationIndex].dataModification;

    xpx_chain_sdk::Key rawDriveKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(drive.data.multisig.c_str(), drive.data.multisig.size(), rawDriveKey);

    xpx_chain_sdk::Hash256 modificationHash256;
    xpx_chain_sdk::ParseHexStringIntoContainer(currentModification.id.c_str(), currentModification.id.size(), modificationHash256);

    auto dataModificationCancelTransaction = xpx_chain_sdk::CreateDataModificationCancelTransaction(rawDriveKey, modificationHash256,
                                                                                                    std::nullopt, deadline, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(dataModificationCancelTransaction.get());

    if (!callback(prettyBalance(dataModificationCancelTransaction->maxFee()))) {
        qDebug () << "TransactionsEngine::cancelDataModification: Cancel was canceled by the user!";
        return;
    }

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> dataModificationCancelNotifier;

    const std::string hash = rawHashToHex(dataModificationCancelTransaction->hash()).toStdString();
    qInfo() << "TransactionsEngine::cancelDataModification. Started 'cancel modification': " << currentModification.id << " transaction id: " << hash;

    const auto driveKeyRaw = rawHashFromHex(drive.data.multisig.c_str());
    const QString currentModificationHex = QString::fromStdString(currentModification.id);
    statusNotifier.set([this, hash, driveKeyRaw, dataModificationCancelNotifierId = dataModificationCancelNotifier.getId(), currentModificationHex](
            const auto& id,
            const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (boost::iequals(notification.hash, hash)) {
            qWarning() << "TransactionsEngine::cancelDataModification " << " onCancelDataModification: " << notification.status.c_str() << " : " << notification.status.c_str() << " transactionId: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), dataModificationCancelNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit cancelModificationFailed(driveKeyRaw, currentModificationHex, notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {},
                                                       [](auto errorCode) {qCritical() << LOG_SOURCE << errorCode.message().c_str(); });

    const auto activeModifications = drive.data.activeDataModifications;
    const auto driveKey = drive.data.multisig;
    dataModificationCancelNotifier.set([this, activeModifications, hash, currentModification, driveKey, statusNotifierId = statusNotifier.getId()](
            const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (boost::iequals(notification.meta.hash, hash)) {
            qInfo() << "TransactionsEngine::cancelDataModification: " << "confirmed dataModificationCancelTransaction hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            auto iterator = std::find_if(activeModifications.begin(), activeModifications.end(), [&](const auto& modification) {
                return boost::iequals(modification.dataModification.id, currentModification.id);
            });

            if (iterator == activeModifications.end()) {
                qWarning() << "TransactionsEngine::cancelDataModification: data modification not found: " << hash << " drive id: " << driveKey;
            }
            else
            {
                std::filesystem::path pathToSandbox = getSettingsFolder().string() + "/" + driveKey + mSandbox;
                const auto preferredPathFormat = pathToSandbox.make_preferred().string();
                const QString pathToActionList = findFile(currentModification.id.c_str(), preferredPathFormat.c_str());
                if (pathToActionList.isEmpty()) {
                    qWarning() << "TransactionsEngine::cancelDataModification: action list not found: " << hash << " sandbox: " << preferredPathFormat.c_str();
                } else {
                    removeDriveModifications(pathToActionList, preferredPathFormat.c_str(), currentModification.downloadDataCdi);
                }
                emit cancelModificationConfirmed(rawHashFromHex(driveKey.c_str()), currentModification.id.c_str());
            }
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { dataModificationCancelNotifier },
                                                               [this, data = dataModificationCancelTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}

void TransactionsEngine::applyDataModification(const std::array<uint8_t, 32>& driveId,
                                               const sirius::drive::ActionList& actions,
                                               const std::vector<xpx_chain_sdk::Address>& addresses,
                                               const std::vector<std::string>& replicators,
                                               const std::optional<xpx_chain_sdk::NetworkDuration>& deadline,
                                               const std::function<bool(const QString& transactionFee)>& confirmationCallback) {
    qInfo() << "TransactionsEngine::applyDataModification. Drive key: " << rawHashToHex(driveId);
    if (!isValidHash(driveId)) {
        emit newError("TransactionsEngine::applyDataModification. Bad driveId: empty!");
        return;
    }

    auto callback = [this, driveId, actions, addresses, deadline, cb = confirmationCallback](auto totalModifyDataSize, auto infoHash) {
        if (!isValidHash(infoHash)) {
            emit newError("TransactionsEngine::applyDataModification. Invalid info hash: " + rawHashToHex(infoHash));
            return;
        }

        sendModification(driveId, infoHash, actions, totalModifyDataSize, addresses, deadline, cb);
    };

    const std::string pathToSandbox = getSettingsFolder().string() + "/" + rawHashToHex(driveId).toStdString() + mSandbox;
    qDebug() << "TransactionsEngine::applyDataModification. Path to sandbox: " << std::filesystem::path(pathToSandbox).make_preferred().string();

    emit addActions(actions, driveId, pathToSandbox, replicators, callback);
}

bool TransactionsEngine::isValidHash(const std::array<uint8_t, 32> &hash)
{
    bool result = true;
    if (hash.empty())
    {
        result = false;
    }

    int counter = 0;
    for (const auto& c : hash)
    {
        if (c == 0)
        {
            counter++;
        }
    }

    if (counter == hash.size())
    {
        result = false;
    }

    return result;
}

std::array<uint8_t, 32> TransactionsEngine::getLatestModificationId(const std::array<uint8_t, 32> &driveId)
{
    return mDataModifications[driveId][mDataModifications[driveId].size() - 1].id;
}

bool TransactionsEngine::isModificationsPresent(const std::array<uint8_t, 32> &driveId)
{
    return !mDataModifications[driveId].empty();
}

void TransactionsEngine::sendModification(const std::array<uint8_t, 32>& driveId,
                                          const std::array<uint8_t, 32>& infoHash,
                                          const sirius::drive::ActionList& actions,
                                          const uint64_t totalModifyDataSize,
                                          const std::vector<xpx_chain_sdk::Address>& replicators,
                                          const std::optional<xpx_chain_sdk::NetworkDuration>& deadline,
                                          const std::function<bool(const QString& transactionFee)>& callback)
{
    const QString actionListFileName = rawHashToHex(infoHash);
    qInfo() << "TransactionsEngine::sendModification. action list downloadDataCDI: " << actionListFileName;

    const QString downloadDataCDIHex = rawHashToHex(infoHash);

    xpx_chain_sdk::Hash256 downloadDataCdi;
    xpx_chain_sdk::ParseHexStringIntoContainer(downloadDataCDIHex.toStdString().c_str(), downloadDataCDIHex.size(), downloadDataCdi);

    const QString   driveKeyHex = rawHashToHex(driveId);
    qInfo() << "TransactionsEngine::sendModification. Drive key: " << driveKeyHex << " Download data CDI: " << downloadDataCDIHex;

    const std::string pathToSandbox = getSettingsFolder().string() + "/" + driveKeyHex.toStdString() + mSandbox;
    const QString pathToActionList = findFile(actionListFileName, pathToSandbox.c_str());
    if (pathToActionList.isEmpty()) {
        emit newError("TransactionsEngine::sendModification. File actionList.bin not found: " + pathToActionList + " . Drive key: " + driveKeyHex);
        emit dataModificationFailed(driveId, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, " action list not found");
        return;
    } else {
        qInfo() << "TransactionsEngine::sendModification. actionList.bin found: " << pathToActionList;
    }

    for (const auto & action : actions) {
        if (action.m_actionId == sirius::drive::action_list_id::upload) {
            const QString path = QDir::toNativeSeparators(action.m_param1.c_str());
            qInfo() << "TransactionsEngine::sendModification. path for upload: " << path;

            if (!QFile::exists(path)) {
                qWarning() << "TransactionsEngine::sendModification. local file not found: " << path;
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

    qInfo() << "TransactionsEngine::sendModification. Upload Size: " << uploadSize << " total modify data size: " << totalModifyDataSize;

    xpx_chain_sdk::Key driveKeyRaw;
    xpx_chain_sdk::ParseHexStringIntoContainer(driveKeyHex.toStdString().c_str(), driveKeyHex.size(), driveKeyRaw);

    auto dataModificationTransaction = xpx_chain_sdk::CreateDataModificationTransaction(driveKeyRaw, downloadDataCdi, uploadSize, feedbackFeeAmount,
                                                                                        std::nullopt, deadline, mpChainClient->getConfig().NetworkId);
    mpChainAccount->signTransaction(dataModificationTransaction.get());

    if (callback && !callback(prettyBalance(dataModificationTransaction->maxFee()))) {
        qDebug () << "TransactionsEngine::sendModification: DataModification was canceled by the user!";
        return;
    }

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> dataModificationUnconfirmedNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> dataModificationConfirmedNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> dataModificationApprovalTransactionNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> replicatorsStatusNotifier;

    const std::string hash = rawHashToHex(dataModificationTransaction->hash()).toStdString();
    std::array<uint8_t, 32> modificationId = rawHashFromHex(hash.c_str());

    if (uploadSize == 0 || totalModifyDataSize == 0) {
        emit newError("Invalid size of upload data: 0. Modification canceled!");
        qWarning() << "Invalid size of upload data: 0. Modification canceled! Drive key: " << driveKeyHex << " modificationId: " << hash.c_str();
        emit dataModificationFailed(driveId, modificationId, " invalid size of upload data: 0");
        return;
    }

    if ( ! MAP_CONTAINS( mDataModifications,driveId)) {
        mDataModifications.insert({ driveId, {} });
    }

    ModificationEntity modificationEntity;
    modificationEntity.id = modificationId;

    mDataModifications[driveId].push_back(modificationEntity);

    emit modificationCreated(driveKeyHex, modificationId);

    statusNotifier.set([this, driveId, hash, modificationId, driveKeyHex, replicators,
                           dataModificationUnconfirmedNotifierId = dataModificationUnconfirmedNotifier.getId(),
                           confirmedNotifierId = dataModificationConfirmedNotifier.getId(),
                        approvalNotifierId = dataModificationApprovalTransactionNotifier.getId(),
                        statusNotifierId = replicatorsStatusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (boost::iequals(notification.hash, hash)) {
            removeConfirmedAddedNotifier(mpChainAccount->address(), confirmedNotifierId);
            removeUnconfirmedAddedNotifier(mpChainAccount->address(), dataModificationUnconfirmedNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            unsubscribeFromReplicators(replicators, approvalNotifierId, statusNotifierId);

            qWarning() << "TransactionsEngine::sendModification. drive key: " + driveKeyHex << " : " << notification.status.c_str() << " transactionId: " << notification.hash.c_str();
            emit dataModificationFailed(driveId, modificationId, notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {},
                                                       [](auto errorCode) {qCritical() << "TransactionsEngine::sendModification. " << errorCode.message().c_str(); });

    dataModificationUnconfirmedNotifier.set([this, hash] (
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (boost::iequals(notification.meta.hash, hash)) {
            qInfo() << "TransactionsEngine::sendModification. Data modification transaction added to unconfirmed pool: " << hash.c_str();
            removeUnconfirmedAddedNotifier(mpChainAccount->address(), id);
        }
    });

    dataModificationConfirmedNotifier.set([this, driveId, driveKeyHex, hash,
                                              statusNotifierId = statusNotifier.getId(),
                                              unconfirmedNotifierId = dataModificationUnconfirmedNotifier.getId(),
                                              modificationId] (
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (boost::iequals(notification.meta.hash, hash)) {
            qInfo() << "TransactionsEngine::sendModification. Data modification transaction confirmed, hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);
            emit dataModificationConfirmed(driveId, modificationId);

            for (auto& modificationEntity : mDataModifications[driveId]) {
                if (modificationEntity.id == modificationId) {
                    modificationEntity.isConfirmed = true;
                    if (modificationEntity.isApproved) {
                        //cancelDataModification(driveId);
                    }

                    return;
                }
            }

            qWarning() << "TransactionsEngine::sendModification. Data modification not found: " << hash << " drive id: " << driveKeyHex;
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { dataModificationConfirmedNotifier }, {},
                                                       [](auto errorCode) {qCritical() << LOG_SOURCE << errorCode.message().c_str(); });

    replicatorsStatusNotifier.set([](const auto&, const xpx_chain_sdk::TransactionStatusNotification& n) {
        qInfo() << "TransactionsEngine::sendModification. (replicators) transaction status notification is received : " << n.status.c_str() << n.hash.c_str();
    });

    dataModificationApprovalTransactionNotifier.set([
                                                        this, pathToSandbox, modificationId, driveId, pathToActionList, replicators, statusNotifierId = replicatorsStatusNotifier.getId(), infoHash](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (xpx_chain_sdk::TransactionType::Data_Modification_Approval != notification.data.type) {
            qInfo() << "TransactionsEngine::sendModification. other transactions confirmed (skip!) : " << rawHashToHex(modificationId);
            return;
        }

        qInfo() << "TransactionsEngine::sendModification. Confirmed DataModificationApproval transaction hash: " << notification.meta.hash.c_str();
        if ( ! MAP_CONTAINS( mDataModificationApprovals, driveId )) {
            mDataModificationApprovals.insert({ driveId, {} });
        }

        if ( MAP_CONTAINS( mDataModificationApprovals[driveId], modificationId )) {
            qWarning() << "TransactionsEngine::sendModification. ModificationId already exists (skip!) : " << rawHashToHex(modificationId);
            return;
        }

        qInfo() << "TransactionsEngine::sendModification. Confirmed data modification approval transaction, hash: " << notification.meta.hash;

        mpBlockchainEngine->getTransactionInfo(xpx_chain_sdk::Confirmed,
                                               notification.meta.hash,
                                               [this, infoHash, pathToActionList, pathToSandbox, driveId, modificationId, notification,
                                                id, replicators, statusNotifierId](auto transaction, auto isSuccess, auto message, auto code) {
            if (!isSuccess) {
                emit dataModificationApprovalFailed(driveId, {}, -1);
                qWarning() << "TransactionsEngine::sendModification. message: " << message.c_str() << " code: " << code.c_str();
                return;
            }

            if (!transaction) {
                emit dataModificationApprovalFailed(driveId, {}, -1);
                qWarning() << "TransactionsEngine::sendModification. Bad pointer to dataModificationApprovalTransaction info";
                return;
            }

            auto dataModificationApprovalTransaction = dynamic_cast<xpx_chain_sdk::transactions_info::DataModificationApprovalTransaction*>(transaction.get());
            if (!dataModificationApprovalTransaction) {
                emit dataModificationApprovalFailed(driveId, {}, -1);
                qWarning() << "TransactionsEngine::sendModification. Bad pointer to dataModificationApprovalTransaction";
                return;
            }

            if ( ! boost::iequals(dataModificationApprovalTransaction->driveKey, rawHashToHex(driveId).toStdString())) {
                qInfo() << "TransactionsEngine::sendModification. DataModificationApprovalTransaction is received for another drive: " << dataModificationApprovalTransaction->driveKey.c_str();
                return;
            }

            if ( ! boost::iequals(dataModificationApprovalTransaction->dataModificationId, rawHashToHex(modificationId).toStdString())) {
                qInfo() << "TransactionsEngine::sendModification. Other dataModificationApprovalTransaction hash: " << rawHashToHex(modificationId);
                return;
            }

            switch (dataModificationApprovalTransaction->modificationStatus) {
                case (uint8_t)sirius::drive::ModificationStatus::SUCCESS:
                {
                    qInfo() << "TransactionsEngine::sendModification. Data modification approval success. Code: " << dataModificationApprovalTransaction->modificationStatus;
                    qInfo() << "TransactionsEngine::sendModification. Confirmed dataModificationApprovalTransaction hash: "
                            << notification.meta.hash.c_str() << " modificationId: "
                            << rawHashToHex(modificationId) << " fileStructureCdi: "
                            << dataModificationApprovalTransaction->fileStructureCdi.c_str();

                    mDataModificationApprovals[driveId].insert(modificationId);

                    emit dataModificationApprovalConfirmed(driveId, dataModificationApprovalTransaction->fileStructureCdi, false);
                    break;
                }

                case (uint8_t)sirius::drive::ModificationStatus::ACTION_LIST_IS_ABSENT:
                {
                    emit dataModificationApprovalFailed(driveId, dataModificationApprovalTransaction->fileStructureCdi, dataModificationApprovalTransaction->modificationStatus);
                    qWarning() << "TransactionsEngine::sendModification. Data modification approval failed: ACTION_LIST_IS_ABSENT";
                    break;
                }

                case (uint8_t)sirius::drive::ModificationStatus::DOWNLOAD_FAILED:
                {
                    emit dataModificationApprovalFailed(driveId, dataModificationApprovalTransaction->fileStructureCdi, dataModificationApprovalTransaction->modificationStatus);
                    qWarning() << "TransactionsEngine::sendModification. Data modification approval failed: DOWNLOAD_FAILED";
                    break;
                }

                case (uint8_t)sirius::drive::ModificationStatus::INVALID_ACTION_LIST:
                {
                    emit dataModificationApprovalFailed(driveId, dataModificationApprovalTransaction->fileStructureCdi, dataModificationApprovalTransaction->modificationStatus);
                    qWarning() << "TransactionsEngine::sendModification. Data modification approval failed: INVALID_ACTION_LIST";
                    break;
                }

                case (uint8_t)sirius::drive::ModificationStatus::NOT_ENOUGH_SPACE:
                {
                    emit dataModificationApprovalFailed(driveId, dataModificationApprovalTransaction->fileStructureCdi, dataModificationApprovalTransaction->modificationStatus);
                    qWarning() << "TransactionsEngine::sendModification. Data modification approval failed: NOT_ENOUGH_SPACE";
                    break;
                }
            }

            unsubscribeFromReplicators(replicators, id, statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);
            removeDriveModifications(pathToActionList, pathToSandbox.c_str(), rawHashToHex(infoHash).toStdString());
        });
    });

    subscribeOnReplicators(replicators, dataModificationApprovalTransactionNotifier, replicatorsStatusNotifier);

    auto approvalNotifierId = dataModificationApprovalTransactionNotifier.getId();
    auto statusNotifierId = replicatorsStatusNotifier.getId();
    mpChainClient->notifications()->addUnconfirmedAddedNotifiers(mpChainAccount->address(), { dataModificationUnconfirmedNotifier },
                                                                [this, data = dataModificationTransaction->binary()]() { announceTransaction(data); },
                                                                [this, hash, replicators, approvalNotifierId, statusNotifierId](auto error) {
        onError(hash, error);
        unsubscribeFromReplicators(replicators, approvalNotifierId, statusNotifierId); });
}

void TransactionsEngine::replicatorOnBoarding(const QString& replicatorPrivateKey, const QString& nodeBootPrivateKey, const uint64_t capacityMB, const std::optional<xpx_chain_sdk::NetworkDuration>& deadline) {
    auto replicatorAccount = std::make_shared<xpx_chain_sdk::Account>([privateKey = replicatorPrivateKey.toStdString()](auto reason, auto param) {
        xpx_chain_sdk::Key key;
        ParseHexStringIntoContainer(privateKey.c_str(), privateKey.size(), key);
        return xpx_chain_sdk::PrivateKey(key.data(), key.size());
    }, mpChainClient->getConfig().NetworkId);

    auto nodeAccount = std::make_shared<xpx_chain_sdk::Account>([privateKey = nodeBootPrivateKey.toStdString()](auto reason, auto param) {
        xpx_chain_sdk::Key key;
        ParseHexStringIntoContainer(privateKey.c_str(), privateKey.size(), key);
        return xpx_chain_sdk::PrivateKey(key.data(), key.size());
    }, mpChainClient->getConfig().NetworkId);

	std::random_device   dev;
	std::seed_seq        seed({dev(), dev(), dev(), dev()});
	std::mt19937         rng(seed);
	auto message = xpx_chain_sdk::Hash256();
	std::generate( message.begin(), message.end(), [&rng]{
		return std::uniform_int_distribution<std::uint32_t>(0,0xff) ( rng );
	});
	xpx_chain_sdk::Ed25519 builder;
	builder.add(xpx_chain_sdk::RawBuffer(message));
	auto messageSignature = builder.sign(nodeAccount->keyPair());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> onBoardingNotifier;

    auto replicatorOnBoardingTransaction = xpx_chain_sdk::CreateReplicatorOnboardingTransaction(xpx_chain_sdk::Amount(capacityMB), nodeAccount->publicKey(), message, messageSignature,
                                                                                                std::nullopt, deadline, mpChainClient->getConfig().NetworkId);
    replicatorAccount->signTransaction(replicatorOnBoardingTransaction.get());

    const std::string onBoardingTransactionHash = rawHashToHex(replicatorOnBoardingTransaction->hash()).toStdString();
    statusNotifier.set([this, replicatorAccount, replicatorPrivateKey, confirmedAddedNotifierId = onBoardingNotifier.getId(), onBoardingTransactionHash](
            const auto& id,
            const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (boost::iequals(notification.hash, onBoardingTransactionHash)) {
            removeConfirmedAddedNotifier(replicatorAccount->address(), confirmedAddedNotifierId);
            removeStatusNotifier(replicatorAccount->address(), id);

            auto replicatorPublicKey = rawHashToHex(replicatorAccount->publicKey());
            qWarning() << LOG_SOURCE << "replicator key: " + replicatorPublicKey << " : " << notification.status.c_str() << " transactionId: " << notification.hash.c_str();
            emit replicatorOnBoardingFailed(replicatorPublicKey, replicatorPrivateKey, notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(replicatorAccount->address(), { statusNotifier }, {},
                                                       [](auto errorCode) {qCritical() << LOG_SOURCE << errorCode.message().c_str(); });

    onBoardingNotifier.set([this, onBoardingTransactionHash, replicatorAccount, statusNotifierId = statusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (boost::iequals(notification.meta.hash, onBoardingTransactionHash)) {
            qInfo() << LOG_SOURCE << "confirmed replicatorOnBoardingTransaction hash: " << onBoardingTransactionHash.c_str();

            removeConfirmedAddedNotifier(replicatorAccount->address(), id);
            removeStatusNotifier(replicatorAccount->address(), statusNotifierId);

            emit replicatorOnBoardingConfirmed(rawHashToHex(replicatorAccount->publicKey()));
        }
    });

    const std::string hash = rawHashToHex(replicatorOnBoardingTransaction->hash()).toStdString();
    qInfo() << "TransactionsEngine::replicatorOnBoarding. capacity(MB): " << capacityMB << " transaction id: " << hash;

    mpChainClient->notifications()->addConfirmedAddedNotifiers(replicatorAccount->address(), { onBoardingNotifier },
                                                               [this, data = replicatorOnBoardingTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}

void TransactionsEngine::replicatorOffBoarding(const std::array<uint8_t, 32> &driveId, const QString& replicatorPrivateKey, const std::optional<xpx_chain_sdk::NetworkDuration>& deadline) {
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

    auto offBoardingTransaction = xpx_chain_sdk::CreateReplicatorOffboardingTransaction(rawDriveKey, std::nullopt, deadline, mpChainClient->getConfig().NetworkId);
    auto replicatorAccount = std::make_shared<xpx_chain_sdk::Account>([privateKey = replicatorPrivateKey.toStdString()](auto reason, auto param) {
        xpx_chain_sdk::Key key;
        ParseHexStringIntoContainer(privateKey.c_str(), privateKey.size(), key);
        return xpx_chain_sdk::PrivateKey(key.data(), key.size());
    }, mpChainClient->getConfig().NetworkId);

    replicatorAccount->signTransaction(offBoardingTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> offBoardingNotifier;

    const std::string hash = rawHashToHex(offBoardingTransaction->hash()).toStdString();
    qInfo() << "TransactionsEngine::replicatorOffBoarding. Drive id: " << driveIdHex << " replicator private key: " << replicatorPrivateKey << " transaction id: " << hash;

    statusNotifier.set([this, hash, replicatorAccount, offBoardingNotifierId = offBoardingNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (boost::iequals(notification.hash, hash)) {
            removeConfirmedAddedNotifier(replicatorAccount->address(), offBoardingNotifierId);
            removeStatusNotifier(replicatorAccount->address(), id);

            auto replicatorPublicKey = rawHashToHex(replicatorAccount->publicKey());
            qWarning() << LOG_SOURCE << "replicator key: " + replicatorPublicKey << " : " << notification.status.c_str() << " transactionId: " << notification.hash.c_str();
            emit replicatorOffBoardingFailed(replicatorPublicKey, notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(replicatorAccount->address(), { statusNotifier }, {},
                                                       [](auto errorCode) {qCritical() << LOG_SOURCE << errorCode.message().c_str(); });

    offBoardingNotifier.set([this, hash, replicatorAccount, statusNotifierId = statusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (boost::iequals(notification.meta.hash, hash)) {
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
    QDirIterator it(QDir::toNativeSeparators(directory), QDirIterator::Subdirectories);

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

void TransactionsEngine::removeDriveModifications(const QString& pathToActionList, const QString& pathToSandbox, const std::string& dataCdi) {
    sirius::drive::ActionList actionList;
    actionList.deserialize(std::filesystem::path(pathToActionList.toStdString()).make_preferred());

    for (const auto& action : actionList) {
        if (action.m_actionId == sirius::drive::action_list_id::upload) {

            emit removeTorrent({ rawHashFromHex(action.m_param1.c_str()) });

            const std::string pathToTorrentFile = pathToSandbox.toStdString() + "/" + action.m_param1 + ".torrent";
            removeFile(pathToTorrentFile.c_str());
        }
    }

    removeFile(pathToActionList);
    removeFile(pathToActionList + ".torrent");
    emit removeFromTorrentMap(dataCdi);
}

void TransactionsEngine::removeFile(const QString& path) {
    QFile file(path);
    if (file.exists()) {
        if (!file.remove()) {
            qWarning () << "TransactionsEngine::removeFile. File has NOT been removed: " << path;
        }
    }
}

void TransactionsEngine::subscribeOnReplicators(
        const std::vector<xpx_chain_sdk::Address>& addresses,
        const xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification>& notifier,
        const xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification>& statusNotifier)
{
    for (const auto& address : addresses)
    {
        mpChainClient->notifications()->addStatusNotifiers(address, { statusNotifier }, {}, [](auto errorCode) { qCritical() << "TransactionsEngine::subscribeOnReplicators: " << errorCode.message().c_str(); });
        mpChainClient->notifications()->addConfirmedAddedNotifiers(address, { notifier }, {}, [](auto errorCode) { qCritical() << "TransactionsEngine::subscribeOnReplicators: " << errorCode.message().c_str(); });
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

void TransactionsEngine::deployContract( const std::array<uint8_t, 32>& driveId, const ContractDeploymentData& data,
                                         const std::vector<xpx_chain_sdk::Address>& replicators,
                                         const std::optional<xpx_chain_sdk::NetworkDuration>& deadline) {
    sendContractDeployment(driveId, data, replicators, deadline);
}

void TransactionsEngine::runManualCall( const ContractManualCallData& manualCallData,
                                        const std::vector<xpx_chain_sdk::Address>& replicators,
                                        const std::optional<xpx_chain_sdk::NetworkDuration>& deadline) {
    sendManualCall(manualCallData, replicators, deadline);
}

void TransactionsEngine::sendContractDeployment( const std::array<uint8_t, 32>& driveId,
                                                 const ContractDeploymentData& data,
                                                 const std::vector<xpx_chain_sdk::Address>& replicators,
                                                 const std::optional<xpx_chain_sdk::NetworkDuration>& deadline) {
    auto driveKey = reinterpret_cast<const xpx_chain_sdk::Key&>(driveId);

    xpx_chain_sdk::Key assignee;
    xpx_chain_sdk::ParseHexStringIntoContainer( data.m_assignee.c_str(), data.m_assignee.size(), assignee );

    xpx_chain_sdk::MosaicContainer servicePayments;
    for ( const auto&[mosaicId, amount]: data.m_servicePayments ) {
        servicePayments.insert( xpx_chain_sdk::Mosaic( std::stoull( mosaicId ), std::stoull( amount )));
    }

    auto transaction = xpx_chain_sdk::CreateDeployContractTransaction( driveKey,
                                                                       data.m_file,
                                                                       data.m_function,
                                                                       {data.m_parameters.begin(),
                                                                        data.m_parameters.end()},
                                                                       data.m_executionCallPayment,
                                                                       data.m_downloadCallPayment,
                                                                       servicePayments,
                                                                       data.m_automaticExecutionFileName,
                                                                       data.m_automaticExecutionFunctionName,
                                                                       data.m_automaticExecutionCallPayment,
                                                                       data.m_automaticDownloadCallPayment,
                                                                       data.m_automaticExecutionsNumber,
                                                                       assignee,
                                                                       std::nullopt, deadline,
                                                                       mpChainClient->getConfig().NetworkId );

    mpChainAccount->signTransaction( transaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> deployContractUnconfirmedNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> deployContractConfirmedNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> deployContractApprovalTransactionNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> replicatorsStatusNotifier;

    const std::string hashHex = rawHashToHex( transaction->hash()).toStdString();

    auto driveKeyHex = rawHashToHex( driveId ).toStdString();

    emit deployContractInitiated( driveId, transaction->hash());

    statusNotifier.set( [this, driveId, hashHex, contractId = transaction->hash(), driveKeyHex, replicators,
                                unconfirmedNotifierId = deployContractUnconfirmedNotifier.getId(),
                                confirmedNotifierId = deployContractConfirmedNotifier.getId(),
                                approvalNotifierId = deployContractApprovalTransactionNotifier.getId(),
                                statusNotifierId = replicatorsStatusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionStatusNotification& notification ) {
        if ( boost::iequals( notification.hash, hashHex )) {
            removeConfirmedAddedNotifier( mpChainAccount->address(), confirmedNotifierId );
            removeUnconfirmedAddedNotifier( mpChainAccount->address(), unconfirmedNotifierId );
            removeStatusNotifier( mpChainAccount->address(), id );

            unsubscribeFromReplicators( replicators, approvalNotifierId, statusNotifierId );

            qWarning() << "TransactionsEngine::deployContract. drive key: " << driveKeyHex << " : "
                       << notification.status.c_str() << " transactionId: " << notification.hash.c_str();
            emit deployContractFailed( driveId, contractId );
        }
    } );

    mpChainClient->notifications()->addStatusNotifiers( mpChainAccount->address(), {statusNotifier}, {},
                                                        []( auto errorCode ) {
                                                            qCritical() << "TransactionsEngine::deployContract. "
                                                                        << errorCode.message().c_str();
                                                        } );

    deployContractUnconfirmedNotifier.set( [this, hashHex](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification ) {
        if ( boost::iequals( notification.meta.hash, hashHex )) {
            qInfo() << "TransactionsEngine::deployContract. Deploy contract transaction added to unconfirmed pool: "
                    << hashHex.c_str();
            removeUnconfirmedAddedNotifier( mpChainAccount->address(), id );
        }
    } );

    deployContractConfirmedNotifier.set( [this, driveId, driveKeyHex, contractId = transaction->hash(), hashHex,
                                                 statusNotifierId = statusNotifier.getId(),
                                                 unconfirmedNotifierId = deployContractUnconfirmedNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification ) {
        if ( boost::iequals( notification.meta.hash, hashHex )) {
            qInfo() << "TransactionsEngine::sendModification. Data modification transaction confirmed, hash: "
                    << hashHex.c_str();

            removeStatusNotifier( mpChainAccount->address(), statusNotifierId );
            removeConfirmedAddedNotifier( mpChainAccount->address(), id );
            emit deployContractConfirmed( driveId, contractId );
        }
    } );

    mpChainClient->notifications()->addConfirmedAddedNotifiers( mpChainAccount->address(),
                                                                {deployContractConfirmedNotifier}, {},
                                                                []( auto errorCode ) {
                                                                    qCritical() << LOG_SOURCE
                                                                                << errorCode.message().c_str();
                                                                } );

    replicatorsStatusNotifier.set( []( const auto&, const xpx_chain_sdk::TransactionStatusNotification& n ) {
        qInfo() << "TransactionsEngine::deployContract. (replicators) transaction status notification is received : "
                << n.status.c_str() << n.hash.c_str();
    } );

    deployContractApprovalTransactionNotifier.set( [this,
                                                           driveId,
                                                           replicators,
                                                           contractId = transaction->hash(),
                                                           transactionHash = hashHex,
                                                           statusNotifierId = replicatorsStatusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification ) {
        if ( xpx_chain_sdk::TransactionType::Successful_End_Batch_Execution == notification.data.type ) {
            mpBlockchainEngine->getTransactionInfo( xpx_chain_sdk::Confirmed,
                                                    notification.meta.hash,
                                                    [this, driveId, notification,
                                                            contractId, transactionHash,
                                                            id, replicators, statusNotifierId]( auto transaction,
                                                                                                auto isSuccess,
                                                                                                auto message,
                                                                                                auto code ) {
                                                        if ( !isSuccess ) {
                                                            qWarning()
                                                                    << "TransactionsEngine::deployContract. message: "
                                                                    << message.c_str() << " code: " << code.c_str();
                                                            return;
                                                        }

                                                        if ( !transaction ) {
                                                            qWarning()
                                                                    << "TransactionsEngine::deployContract. Bad pointer to successfulEndBatchExecutionTransaction info";
                                                            return;
                                                        }

                                                        auto successfulEndBatchExecutionTransaction = reinterpret_cast<xpx_chain_sdk::transactions_info::SuccessfulEndBatchExecutionTransaction*>(transaction.get());

                                                        if ( !boost::iequals(
                                                                successfulEndBatchExecutionTransaction->contractKey,
                                                                transactionHash )) {
                                                            return;
                                                        }

                                                        auto callIt = std::find_if(
                                                                successfulEndBatchExecutionTransaction->callDigests.begin(),
                                                                successfulEndBatchExecutionTransaction->callDigests.end(),
                                                                [=]( const auto& item ) {
                                                                    return boost::iequals( item.callId,
                                                                                           transactionHash );
                                                                } );

                                                        if ( callIt ==
                                                             successfulEndBatchExecutionTransaction->callDigests.end()) {
                                                            return;
                                                        }

                                                        if ( callIt->status == 0 ) {
                                                            emit deployContractApprovalConfirmed( driveId, contractId );
                                                        } else {
                                                            emit deployContractApprovalFailed( driveId, contractId );
                                                        }

                                                        qInfo()
                                                                << "TransactionsEngine::deployContract. Confirmed deployContractApproval";

                                                        unsubscribeFromReplicators( replicators, id, statusNotifierId );
                                                        removeConfirmedAddedNotifier( mpChainAccount->address(), id );
                                                    } );
        }
        if ( xpx_chain_sdk::TransactionType::Unsuccessful_End_Batch_Execution == notification.data.type ) {
            mpBlockchainEngine->getTransactionInfo( xpx_chain_sdk::Confirmed,
                                                    notification.meta.hash,
                                                    [this, driveId, notification,
                                                            contractId, transactionHash,
                                                            id, replicators, statusNotifierId]( auto transaction,
                                                                                                auto isSuccess,
                                                                                                auto message,
                                                                                                auto code ) {
                                                        if ( !isSuccess ) {
                                                            qWarning()
                                                                    << "TransactionsEngine::deployContract. message: "
                                                                    << message.c_str() << " code: " << code.c_str();
                                                            return;
                                                        }

                                                        if ( !transaction ) {
                                                            qWarning()
                                                                    << "TransactionsEngine::deployContract. Bad pointer to successfulEndBatchExecutionTransaction info";
                                                            return;
                                                        }

                                                        auto successfulEndBatchExecutionTransaction = reinterpret_cast<xpx_chain_sdk::transactions_info::SuccessfulEndBatchExecutionTransaction*>(transaction.get());

                                                        if ( !boost::iequals(
                                                                successfulEndBatchExecutionTransaction->contractKey,
                                                                transactionHash )) {
                                                            return;
                                                        }

                                                        auto callIt = std::find_if(
                                                                successfulEndBatchExecutionTransaction->callDigests.begin(),
                                                                successfulEndBatchExecutionTransaction->callDigests.end(),
                                                                [=]( const auto& item ) {
                                                                    return boost::iequals( item.callId,
                                                                                           transactionHash );
                                                                } );

                                                        if ( callIt ==
                                                             successfulEndBatchExecutionTransaction->callDigests.end()) {
                                                            return;
                                                        }

                                                        emit deployContractApprovalFailed( driveId, contractId );

                                                        qInfo()
                                                                << "TransactionsEngine::deployContract. Confirmed deployContractApproval";

                                                        unsubscribeFromReplicators( replicators, id, statusNotifierId );
                                                        removeConfirmedAddedNotifier( mpChainAccount->address(), id );
                                                    } );
        }

    } );

    subscribeOnReplicators( replicators, deployContractApprovalTransactionNotifier, replicatorsStatusNotifier );

    auto approvalNotifierId = deployContractApprovalTransactionNotifier.getId();
    auto statusNotifierId = replicatorsStatusNotifier.getId();
    mpChainClient->notifications()->addUnconfirmedAddedNotifiers( mpChainAccount->address(),
                                                                  {deployContractUnconfirmedNotifier},
                                                                  [this, data = transaction->binary()]() {
                                                                      announceTransaction( data );
                                                                  },
                                                                  [this, hashHex, replicators, approvalNotifierId, statusNotifierId](
                                                                          auto error ) {
                                                                      onError( hashHex, error );
                                                                      unsubscribeFromReplicators( replicators,
                                                                                                  approvalNotifierId,
                                                                                                  statusNotifierId );
                                                                  } );

}

void TransactionsEngine::sendManualCall( const ContractManualCallData& data,
                                         const std::vector<xpx_chain_sdk::Address>& replicators,
                                         const std::optional<xpx_chain_sdk::NetworkDuration>& deadline) {
    auto contractId = rawHashFromHex(QString::fromStdString(data.m_contractKey));
    auto contractKey = reinterpret_cast<const xpx_chain_sdk::Key&>(contractId);

    xpx_chain_sdk::MosaicContainer servicePayments;
    for ( const auto&[mosaicId, amount]: data.m_servicePayments ) {
        servicePayments.insert( xpx_chain_sdk::Mosaic( std::stoull( mosaicId ), std::stoull( amount )));
    }

    auto transaction = xpx_chain_sdk::CreateManualCallTransaction( contractKey,
                                                                   data.m_file,
                                                                   data.m_function,
                                                                   {data.m_parameters.begin(),
                                                                    data.m_parameters.end()},
                                                                   data.m_executionCallPayment,
                                                                   data.m_downloadCallPayment,
                                                                   servicePayments,
                                                                   std::nullopt, deadline,
                                                                   mpChainClient->getConfig().NetworkId );

    mpChainAccount->signTransaction( transaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> manualCallUnconfirmedNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> manualCallConfirmedNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> manualCallApprovalTransactionNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> replicatorsStatusNotifier;

    const std::string hashHex = rawHashToHex( transaction->hash()).toStdString();

    auto contractKeyHex = rawHashToHex( contractId ).toStdString();

    emit manualCallInitiated( contractId, transaction->hash());

    statusNotifier.set( [this, contractId, hashHex, callId = transaction->hash(), contractKeyHex, replicators,
                         unconfirmedNotifierId = manualCallUnconfirmedNotifier.getId(),
                         confirmedNotifierId = manualCallConfirmedNotifier.getId(),
                         approvalNotifierId = manualCallApprovalTransactionNotifier.getId(),
                         statusNotifierId = replicatorsStatusNotifier.getId()](
                                 const auto& id,
                                 const xpx_chain_sdk::TransactionStatusNotification& notification ) {
        if ( boost::iequals( notification.hash, hashHex )) {
            removeConfirmedAddedNotifier( mpChainAccount->address(), confirmedNotifierId );
            removeUnconfirmedAddedNotifier( mpChainAccount->address(), unconfirmedNotifierId );
            removeStatusNotifier( mpChainAccount->address(), id );

            unsubscribeFromReplicators( replicators, approvalNotifierId, statusNotifierId );

            qWarning() << "TransactionsEngine::manualCall. contract key: " << contractKeyHex << " : "
            << notification.status.c_str() << " transactionId: " << notification.hash.c_str();
            emit manualCallFailed( contractId, callId );
        }
    } );

    mpChainClient->notifications()->addStatusNotifiers( mpChainAccount->address(), {statusNotifier}, {},
                                                        []( auto errorCode ) {
        qCritical() << "TransactionsEngine::contractCall. "
        << errorCode.message().c_str();
    } );

    manualCallUnconfirmedNotifier.set( [this, hashHex](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification ) {
        if ( boost::iequals( notification.meta.hash, hashHex )) {
            qInfo() << "TransactionsEngine::manualCall. Manual call transaction added to unconfirmed pool: "
            << hashHex.c_str();
            removeUnconfirmedAddedNotifier( mpChainAccount->address(), id );
        }
    } );

    manualCallConfirmedNotifier.set( [this, contractId, contractKeyHex, callId = transaction->hash(), hashHex,
                                          statusNotifierId = statusNotifier.getId(),
                                          unconfirmedNotifierId = manualCallUnconfirmedNotifier.getId()](
                                                  const auto& id,
                                                  const xpx_chain_sdk::TransactionNotification& notification ) {
        if ( boost::iequals( notification.meta.hash, hashHex )) {
            qInfo() << "TransactionsEngine::sendModification. Manual call transaction confirmed, hash: "
            << hashHex.c_str();

            removeStatusNotifier( mpChainAccount->address(), statusNotifierId );
            removeConfirmedAddedNotifier( mpChainAccount->address(), id );
            emit manualCallConfirmed( contractId, callId );
        }
    } );

    mpChainClient->notifications()->addConfirmedAddedNotifiers( mpChainAccount->address(),
                                                                {manualCallConfirmedNotifier}, {},
                                                                []( auto errorCode ) {
        qCritical() << LOG_SOURCE
        << errorCode.message().c_str();
    } );

//    replicatorsStatusNotifier.set( []( const auto&, const xpx_chain_sdk::TransactionStatusNotification& n ) {
//        qInfo() << "TransactionsEngine::manualCall. (replicators) transaction status notification is received : "
//        << n.status.c_str() << n.hash.c_str();
//    } );
//
//    deployContractApprovalTransactionNotifier.set( [this,
//                                                    driveId,
//                                                    replicators,
//                                                    contractId = transaction->hash(),
//                                                    transactionHash = hashHex,
//                                                    statusNotifierId = replicatorsStatusNotifier.getId()](
//                                                            const auto& id,
//                                                            const xpx_chain_sdk::TransactionNotification& notification ) {
//        if ( xpx_chain_sdk::TransactionType::Successful_End_Batch_Execution == notification.data.type ) {
//            mpBlockchainEngine->getTransactionInfo( xpx_chain_sdk::Confirmed,
//                                                    notification.meta.hash,
//                                                    [this, driveId, notification,
//                                                     contractId, transactionHash,
//                                                     id, replicators, statusNotifierId]( auto transaction,
//                                                             auto isSuccess,
//                                                             auto message,
//                                                             auto code ) {
//                if ( !isSuccess ) {
//                    qWarning()
//                    << "TransactionsEngine::deployContract. message: "
//                    << message.c_str() << " code: " << code.c_str();
//                    return;
//                }
//
//                if ( !transaction ) {
//                    qWarning()
//                    << "TransactionsEngine::deployContract. Bad pointer to successfulEndBatchExecutionTransaction info";
//                    return;
//                }
//
//                auto successfulEndBatchExecutionTransaction = reinterpret_cast<xpx_chain_sdk::transactions_info::SuccessfulEndBatchExecutionTransaction*>(transaction.get());
//
//                if ( !boost::iequals(
//                        successfulEndBatchExecutionTransaction->contractKey,
//                        transactionHash )) {
//                    return;
//                }
//
//                auto callIt = std::find_if(
//                        successfulEndBatchExecutionTransaction->callDigests.begin(),
//                        successfulEndBatchExecutionTransaction->callDigests.end(),
//                        [=]( const auto& item ) {
//                            return boost::iequals( item.callId,
//                                                   transactionHash );
//                        } );
//
//                if ( callIt ==
//                successfulEndBatchExecutionTransaction->callDigests.end()) {
//                    return;
//                }
//
//                if ( callIt->status == 0 ) {
//                    emit deployContractApprovalConfirmed( driveId, contractId );
//                } else {
//                    emit deployContractApprovalFailed( driveId, contractId );
//                }
//
//                qInfo()
//                << "TransactionsEngine::deployContract. Confirmed deployContractApproval";
//
//                unsubscribeFromReplicators( replicators, id, statusNotifierId );
//                removeConfirmedAddedNotifier( mpChainAccount->address(), id );
//            } );
//        }
//        if ( xpx_chain_sdk::TransactionType::Unsuccessful_End_Batch_Execution == notification.data.type ) {
//            mpBlockchainEngine->getTransactionInfo( xpx_chain_sdk::Confirmed,
//                                                    notification.meta.hash,
//                                                    [this, driveId, notification,
//                                                     contractId, transactionHash,
//                                                     id, replicators, statusNotifierId]( auto transaction,
//                                                             auto isSuccess,
//                                                             auto message,
//                                                             auto code ) {
//                if ( !isSuccess ) {
//                    qWarning()
//                    << "TransactionsEngine::deployContract. message: "
//                    << message.c_str() << " code: " << code.c_str();
//                    return;
//                }
//
//                if ( !transaction ) {
//                    qWarning()
//                    << "TransactionsEngine::deployContract. Bad pointer to successfulEndBatchExecutionTransaction info";
//                    return;
//                }
//
//                auto successfulEndBatchExecutionTransaction = reinterpret_cast<xpx_chain_sdk::transactions_info::SuccessfulEndBatchExecutionTransaction*>(transaction.get());
//
//                if ( !boost::iequals(
//                        successfulEndBatchExecutionTransaction->contractKey,
//                        transactionHash )) {
//                    return;
//                }
//
//                auto callIt = std::find_if(
//                        successfulEndBatchExecutionTransaction->callDigests.begin(),
//                        successfulEndBatchExecutionTransaction->callDigests.end(),
//                        [=]( const auto& item ) {
//                            return boost::iequals( item.callId,
//                                                   transactionHash );
//                        } );
//
//                if ( callIt ==
//                successfulEndBatchExecutionTransaction->callDigests.end()) {
//                    return;
//                }
//
//                emit deployContractApprovalFailed( driveId, contractId );
//
//                qInfo()
//                << "TransactionsEngine::deployContract. Confirmed deployContractApproval";
//
//                unsubscribeFromReplicators( replicators, id, statusNotifierId );
//                removeConfirmedAddedNotifier( mpChainAccount->address(), id );
//            } );
//        }
//
//    } );
//
//    subscribeOnReplicators( replicators, deployContractApprovalTransactionNotifier, replicatorsStatusNotifier );
//
    auto approvalNotifierId = manualCallApprovalTransactionNotifier.getId();
    auto statusNotifierId = replicatorsStatusNotifier.getId();
    mpChainClient->notifications()->addUnconfirmedAddedNotifiers( mpChainAccount->address(),
                                                                  {manualCallUnconfirmedNotifier},
                                                                  [this, data = transaction->binary()]() {
        announceTransaction( data );
        },
        [this, hashHex, replicators, approvalNotifierId, statusNotifierId](
                auto error ) {
        onError( hashHex, error );
        unsubscribeFromReplicators( replicators,
                                    approvalNotifierId,
                                    statusNotifierId );
    } );
}

void TransactionsEngine::streamStart(const std::array<uint8_t, 32>& rawDrivePubKey,
                                            const std::string& folderName,
                                            uint64_t expectedUploadSizeMegabytes,
                                            uint64_t feedbackFeeAmount,
                                            const std::optional<xpx_chain_sdk::NetworkDuration>& deadline,
                                            const std::function<void(std::string hash)>& callback) {
    const QString driveKeyHex = rawHashToHex(rawDrivePubKey);
    xpx_chain_sdk::Key driveKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(driveKeyHex.toStdString().c_str(), driveKeyHex.size(), driveKey);

    qInfo() << "TransactionsEngine::streamStart. Drive key: " << driveKeyHex
            << " folderName: " << folderName << " expectedUploadSizeMegabytes: "
            << expectedUploadSizeMegabytes << " feedbackFeeAmount: " << feedbackFeeAmount;

    const std::vector<uint8_t> rawFolderName(folderName.begin(), folderName.end());
    auto streamStartTransaction = xpx_chain_sdk::CreateStreamStartTransaction(driveKey, expectedUploadSizeMegabytes, rawFolderName.size(), feedbackFeeAmount, rawFolderName, std::nullopt, deadline, mpChainClient->getConfig().NetworkId);

    mpChainAccount->signTransaction(streamStartTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> streamStartNotifier;

    std::string hash = rawHashToHex(streamStartTransaction->hash()).toStdString();
    statusNotifier.set([this, hash, streamStartNotifierId = streamStartNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (boost::iequals(notification.hash, hash)) {
            qWarning() << "TransactionsEngine::streamStart::statusNotifier: " << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), streamStartNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit streamStartFailed(rawHashFromHex(hash.c_str()), notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << "TransactionsEngine::streamStart::addStatusNotifiers: " << errorCode.message().c_str();
    });

    streamStartNotifier.set([this, hash, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (boost::iequals(notification.meta.hash, hash)) {
            qInfo() << "confirmed streamTransaction, hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit streamStartConfirmed(rawHashFromHex(hash.c_str()));
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { streamStartNotifier },
                                                               [this, data = streamStartTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });

    callback(hash);
}

void TransactionsEngine::streamFinish(const std::array<uint8_t, 32>& rawDrivePubKey,
                                      const std::array<uint8_t, 32>& streamId,
                                      uint64_t actualUploadSizeMegabytes,
                                      const std::array<uint8_t, 32>& streamStructureCdi,
                                      const std::optional<xpx_chain_sdk::NetworkDuration>& deadline,
                                      const std::vector<xpx_chain_sdk::Address>& replicatorAddresses)
{
    const QString driveKeyHex = rawHashToHex(rawDrivePubKey);
    xpx_chain_sdk::Key driveKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(driveKeyHex.toStdString().c_str(), driveKeyHex.size(), driveKey);

    const QString streamIdHex = rawHashToHex(streamId);
    xpx_chain_sdk::Key streamIdKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(streamIdHex.toStdString().c_str(), streamIdHex.size(), streamIdKey);

    const QString streamStructureCdiHex = rawHashToHex(streamStructureCdi);
    xpx_chain_sdk::Key streamCdi;
    xpx_chain_sdk::ParseHexStringIntoContainer(streamStructureCdiHex.toStdString().c_str(), streamStructureCdiHex.size(), streamCdi);

    qInfo() << "TransactionsEngine::streamFinish. Drive key: " << driveKeyHex
            << " streamId: " << streamIdHex << " actualUploadSizeMegabytes: "
            << actualUploadSizeMegabytes << " streamCDI: " << streamStructureCdiHex;

    auto streamFinishTransaction = xpx_chain_sdk::CreateStreamFinishTransaction(driveKey, streamIdKey, actualUploadSizeMegabytes, streamCdi,
                                                                                std::nullopt, deadline, mpChainClient->getConfig().NetworkId);

    mpChainAccount->signTransaction(streamFinishTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> streamFinishNotifier;

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> replicatorsStatusNotifier;
    replicatorsStatusNotifier.set([](const auto&, const xpx_chain_sdk::TransactionStatusNotification& n)
    {
        qInfo() << "TransactionsEngine::streamFinish. (replicators) transaction status notification is received : " << n.status.c_str() << n.hash.c_str();
    });

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> dataModificationApprovalTransactionNotifier;

    const std::string hash = rawHashToHex(streamFinishTransaction->hash()).toStdString();
    statusNotifier.set([this, hash, streamId, replicatorAddresses,
                           replicatorsStatusNotifierId = replicatorsStatusNotifier.getId(),
                           streamFinishNotifierId = streamFinishNotifier.getId(),
                           approvalNotifierId = dataModificationApprovalTransactionNotifier.getId()] (const auto& id, const auto& notification)
    {
        if (boost::iequals(notification.hash, hash))
        {
            qWarning() << "TransactionsEngine::streamFinish::statusNotifier: " << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), streamFinishNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);
            unsubscribeFromReplicators(replicatorAddresses, approvalNotifierId, replicatorsStatusNotifierId);

            emit streamFinishFailed(streamId, notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << "TransactionsEngine::streamFinish::addStatusNotifiers: " << errorCode.message().c_str();
    });

    streamFinishNotifier.set([this, hash, streamId, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (boost::iequals(notification.meta.hash, hash))
        {
            qInfo() << "TransactionsEngine::streamFinish:: confirmed streamFinishTransaction, hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit streamFinishConfirmed(streamId);
        }
    });

    std::array<uint8_t, 32> modificationId = rawHashFromHex(hash.c_str());

    dataModificationApprovalTransactionNotifier.set([
            this, modificationId, rawDrivePubKey, replicatorAddresses, statusNotifierId = replicatorsStatusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification)
    {
        if (xpx_chain_sdk::TransactionType::Data_Modification_Approval != notification.data.type)
        {
            qInfo() << "TransactionsEngine::streamFinish. other transactions confirmed (skip!) : " << rawHashToHex(modificationId);
            return;
        }

        qInfo() << "TransactionsEngine::streamFinish. Confirmed DataModificationApproval transaction hash: " << notification.meta.hash.c_str();
        if ( ! MAP_CONTAINS( mDataModificationApprovals, rawDrivePubKey ))
        {
            mDataModificationApprovals.insert({ rawDrivePubKey, {} });
        }

        if ( MAP_CONTAINS( mDataModificationApprovals[rawDrivePubKey], modificationId ))
        {
            qWarning() << "TransactionsEngine::streamFinish. ModificationId already exists (skip!) : " << rawHashToHex(modificationId);
            return;
        }

        qInfo() << "TransactionsEngine::streamFinish. Confirmed data modification approval transaction, hash: " << notification.meta.hash;

        mpBlockchainEngine->getTransactionInfo(xpx_chain_sdk::Confirmed,
                                               notification.meta.hash,
                                               [this, rawDrivePubKey, modificationId, notification,
                                                       id, replicatorAddresses, statusNotifierId](auto transaction, auto isSuccess, auto message, auto code)
        {
            if (!isSuccess) {
                //emit dataModificationApprovalFailed(driveId, {}, -1);
                qWarning() << "TransactionsEngine::streamFinish. message: " << message.c_str() << " code: " << code.c_str();
                return;
            }

            if (!transaction) {
                //emit dataModificationApprovalFailed(driveId, {}, -1);
                qWarning() << "TransactionsEngine::streamFinish. Bad pointer to dataModificationApprovalTransaction info";
                return;
            }

            auto dataModificationApprovalTransaction = dynamic_cast<xpx_chain_sdk::transactions_info::DataModificationApprovalTransaction*>(transaction.get());
            if (!dataModificationApprovalTransaction) {
                //emit dataModificationApprovalFailed(driveId, {}, -1);
                qWarning() << "TransactionsEngine::streamFinish. Bad pointer to dataModificationApprovalTransaction";
                return;
            }

            if ( ! boost::iequals(dataModificationApprovalTransaction->driveKey, rawHashToHex(rawDrivePubKey).toStdString())) {
                qInfo() << "TransactionsEngine::streamFinish. DataModificationApprovalTransaction is received for another drive: " << dataModificationApprovalTransaction->driveKey.c_str();
                return;
            }

            switch (dataModificationApprovalTransaction->modificationStatus)
            {
                case (uint8_t)sirius::drive::ModificationStatus::SUCCESS:
                {
                    qInfo() << "TransactionsEngine::streamFinish. Data modification approval success. Code: " << dataModificationApprovalTransaction->modificationStatus;
                    qInfo() << "TransactionsEngine::streamFinish. Confirmed dataModificationApprovalTransaction hash: "
                            << notification.meta.hash.c_str() << " modificationId: "
                            << rawHashToHex(modificationId) << " fileStructureCdi: "
                            << dataModificationApprovalTransaction->fileStructureCdi.c_str();

                    mDataModificationApprovals[rawDrivePubKey].insert(modificationId);

                    emit dataModificationApprovalConfirmed(rawDrivePubKey, dataModificationApprovalTransaction->fileStructureCdi, true);
                    break;
                }

                case (uint8_t)sirius::drive::ModificationStatus::ACTION_LIST_IS_ABSENT:
                {
                    //emit dataModificationApprovalFailed(driveId, dataModificationApprovalTransaction->fileStructureCdi, dataModificationApprovalTransaction->modificationStatus);
                    qWarning() << "TransactionsEngine::sendModification. Data modification approval failed: ACTION_LIST_IS_ABSENT";
                    break;
                }

                case (uint8_t)sirius::drive::ModificationStatus::DOWNLOAD_FAILED:
                {
                    //emit dataModificationApprovalFailed(driveId, dataModificationApprovalTransaction->fileStructureCdi, dataModificationApprovalTransaction->modificationStatus);
                    qWarning() << "TransactionsEngine::streamFinish. Data modification approval failed: DOWNLOAD_FAILED";
                    break;
                }

                case (uint8_t)sirius::drive::ModificationStatus::INVALID_ACTION_LIST:
                {
                    //emit dataModificationApprovalFailed(driveId, dataModificationApprovalTransaction->fileStructureCdi, dataModificationApprovalTransaction->modificationStatus);
                    qWarning() << "TransactionsEngine::streamFinish. Data modification approval failed: INVALID_ACTION_LIST";
                    break;
                }

                case (uint8_t)sirius::drive::ModificationStatus::NOT_ENOUGH_SPACE:
                {
                    //emit dataModificationApprovalFailed(driveId, dataModificationApprovalTransaction->fileStructureCdi, dataModificationApprovalTransaction->modificationStatus);
                    qWarning() << "TransactionsEngine::streamFinish. Data modification approval failed: NOT_ENOUGH_SPACE";
                    break;
                }
            }

            unsubscribeFromReplicators(replicatorAddresses, id, statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);
            //removeDriveModifications(pathToActionList, pathToSandbox.c_str());
        });
    });

    subscribeOnReplicators(replicatorAddresses, dataModificationApprovalTransactionNotifier, replicatorsStatusNotifier);

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { streamFinishNotifier },
                                                               [this, data = streamFinishTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}

void TransactionsEngine::streamPayment(const std::array<uint8_t, 32>& rawDrivePubKey,
                                       const std::array<uint8_t, 32>& streamId,
                                       uint64_t additionalUploadSizeMegabytes,
                                       const std::optional<xpx_chain_sdk::NetworkDuration>& deadline) {
    const QString driveKeyHex = rawHashToHex(rawDrivePubKey);
    xpx_chain_sdk::Key driveKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(driveKeyHex.toStdString().c_str(), driveKeyHex.size(), driveKey);

    const QString streamIdHex = rawHashToHex(streamId);
    xpx_chain_sdk::Key streamIdKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(streamIdHex.toStdString().c_str(), streamIdHex.size(), streamIdKey);

    auto streamPaymentTransaction = xpx_chain_sdk::CreateStreamPaymentTransaction(driveKey, streamIdKey, additionalUploadSizeMegabytes,
                                                                                  std::nullopt, deadline, mpChainClient->getConfig().NetworkId);

    mpChainAccount->signTransaction(streamPaymentTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> streamPaymentNotifier;

    const std::string hash = rawHashToHex(streamPaymentTransaction->hash()).toStdString();
    qInfo() << "TransactionsEngine::streamPayment. Drive key: " << driveKeyHex << " streamId: " << streamIdHex << " additionalUploadSizeMegabytes: " << additionalUploadSizeMegabytes;

    statusNotifier.set([this, hash, streamPaymentNotifierId = streamPaymentNotifier.getId(), streamId](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        if (boost::iequals(notification.hash, hash)) {
            qWarning() << "TransactionsEngine::streamPayment::statusNotifier: " << notification.status.c_str() << " hash: " << notification.hash.c_str();

            removeConfirmedAddedNotifier(mpChainAccount->address(), streamPaymentNotifierId);
            removeStatusNotifier(mpChainAccount->address(), id);

            emit streamPaymentFailed(streamId, notification.status.c_str());
        }
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, {}, [](auto errorCode) {
        qCritical() << "TransactionsEngine::streamPayment::addStatusNotifiers: " << errorCode.message().c_str();
    });

    streamPaymentNotifier.set([this, hash, streamId, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (boost::iequals(notification.meta.hash, hash)) {
            qInfo() << "TransactionsEngine::streamPayment: confirmed streamPaymentTransaction, hash: " << hash.c_str();

            removeStatusNotifier(mpChainAccount->address(), statusNotifierId);
            removeConfirmedAddedNotifier(mpChainAccount->address(), id);

            emit streamPaymentConfirmed(streamId);
        }
    });

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { streamPaymentNotifier },
                                                               [this, data = streamPaymentTransaction->binary()]() { announceTransaction(data); },
                                                               [this, hash](auto error) { onError(hash, error); });
}
