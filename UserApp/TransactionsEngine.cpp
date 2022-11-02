#include "TransactionsEngine.h"
#include "Utils.h"

#include <qdebug.h>

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

    statusNotifier.set([this, downloadNotifierId = downloadNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        qWarning() << notification.status.c_str() << " hash: " << notification.hash.c_str();
        emit createDownloadTransactionFailed(notification.hash.c_str(), notification.status.c_str());

        mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [](auto code)
        {
            qWarning() << code.message().c_str();
        }, { downloadNotifierId });

        mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [](auto code) {
            qWarning() << code.message().c_str();
        }, { id });
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, [](){}, [this](auto errorCode) {
        qCritical() << errorCode.message().c_str();
    });

    std::string hash = rawHashToHex(downloadTransaction->hash()).toStdString();

    downloadNotifier.set([this, channelAlias, hash, rawDrivePubKey, statusNotifierId = statusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << "confirmed downloadTransaction hash: " << hash.c_str();
            mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [](auto code) {
                qWarning() << code.message().c_str();
            }, { statusNotifierId });

            mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [hash](auto code) {
                qWarning() << code.message().c_str();
            }, { id });

            //emit balancesUpdated();
            emit createDownloadTransactionConfirmed(channelAlias, rawHashFromHex(hash.c_str()), rawDrivePubKey);
        }
    });

    auto onSuccess = [this, data = downloadTransaction->binary()](){
        mpBlockchainEngine->announceNewTransaction(data, [](auto isAnnounced, auto isSuccess, auto message, auto code){
            if (!isSuccess) {
                qWarning() << message.c_str() << " : " << code.c_str();
            }
        });
    };

    auto onError = [hash](auto errorCode){
        qCritical() << errorCode.message().c_str() << " hash: " << hash.c_str();
    };

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { downloadNotifier }, onSuccess, onError);

    return hash;
}

void TransactionsEngine::closeDownloadChannel(const std::array<uint8_t, 32> &channelId) {
    QString channelIdHex = rawHashToHex(channelId);
    if( channelId.empty()) {
        qWarning() << "bad channelId: " << " : " << channelIdHex;
        return;
    }

    xpx_chain_sdk::Key channelIdKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(channelIdHex.toStdString().c_str(), channelIdHex.size(), channelIdKey);

    const xpx_chain_sdk::Amount feedbackFeeAmount = 10;
    auto finishDownloadTransaction = xpx_chain_sdk::CreateFinishDownloadTransaction(channelIdKey, feedbackFeeAmount, std::nullopt, std::nullopt, mpChainClient->getConfig()->NetworkId);
    mpChainAccount->signTransaction(finishDownloadTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> finishDownloadNotifier;

    statusNotifier.set([this, finishDownloadNotifierId = finishDownloadNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        qWarning() << notification.status.c_str() << " hash: " << notification.hash.c_str();
        emit finishDownloadTransactionFailed(notification.status.c_str());

        mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [](auto code) {
            qWarning() << code.message().c_str();
        }, { finishDownloadNotifierId });

        mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [](auto code) {
            qWarning() << code.message().c_str();
        }, { id });
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, [](){}, [](auto errorCode) {
        qCritical() << errorCode.message().c_str();
    });

    const std::string hash = rawHashToHex(finishDownloadTransaction->hash()).toStdString();

    finishDownloadNotifier.set([this, hash, channelId, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << "confirmed finishDownloadTransaction hash: " << hash.c_str();
            mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [](auto code) {
                qWarning() << code.message().c_str();
            } , { statusNotifierId });

            mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [hash](auto code) {
                qWarning() << code.message().c_str();
            }, { id });

            emit finishDownloadTransactionConfirmed(channelId);
        }
    });

    auto onSuccess = [this, data = finishDownloadTransaction->binary()](){
        mpBlockchainEngine->announceNewTransaction(data, [](auto isAnnounced, auto isSuccess, auto message, auto code){
            if (!isSuccess) {
                qWarning() << message.c_str() << " : " << code.c_str();
            }
        });
    };

    auto onError = [hash](auto errorCode){
        qCritical() << errorCode.message().c_str() << " hash: " << hash.c_str();
    };

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { finishDownloadNotifier }, onSuccess, onError);
}

void TransactionsEngine::downloadPayment(const std::array<uint8_t, 32> &channelId, uint64_t prepaidSize) {
    const QString channelIdHex = rawHashToHex(channelId);
    //emit log(Info, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), ": channelId: ", channelIdHex});

    xpx_chain_sdk::Key channelIdKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(channelIdHex.toStdString().c_str(), channelIdHex.size(), channelIdKey);

    const xpx_chain_sdk::Amount feedbackFeeAmount = 10;

    auto downloadPaymentTransaction = xpx_chain_sdk::CreateDownloadPaymentTransaction(channelIdKey, prepaidSize, feedbackFeeAmount,
                                                                                      std::nullopt, std::nullopt, mpChainClient->getConfig()->NetworkId);
    mpChainAccount->signTransaction(downloadPaymentTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> downloadPaymentNotifier;

    statusNotifier.set([this, downloadPaymentNotifierId = downloadPaymentNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        //emit log(Error, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), "onDownloadPayment : ", notification.status.c_str(), notification.hash.c_str()});
        //emit downloadPaymentFailed(notification.hash.c_str(), notification.status.c_str());

        mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [this](auto code) {
            qWarning() << code.message().c_str();
        }, { downloadPaymentNotifierId });

        mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [this](auto code) {
            qWarning() << code.message().c_str();
        }, { id });
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, [](){}, [this](auto errorCode) {
        qCritical() << errorCode.message().c_str();
    });

    const std::string hash = rawHashToHex(downloadPaymentTransaction->hash()).toStdString();

    downloadPaymentNotifier.set([this, hash, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            //emit log(Info, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), "confirmed downloadPaymentTransaction hash: ", hash.c_str()});

            mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [](auto code) {
                qWarning() << code.message().c_str();
            }, { statusNotifierId });

            mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [hash](auto code) {
                qWarning() << code.message().c_str();
            }, {id });

            //emit balancesUpdated();
            //emit downloadPaymentConfirmed();
        }
    });

    auto onSuccess = [this, data = downloadPaymentTransaction->binary()](){
        mpBlockchainEngine->announceNewTransaction(data, [](auto isAnnounced, auto isSuccess, auto message, auto code){
            if (!isSuccess) {
                qWarning() << message.c_str() << " : " << code.c_str();
            }
        });
    };

    auto onError = [hash](auto errorCode){
        qCritical() << errorCode.message().c_str() << " hash: " << hash.c_str();
    };

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { downloadPaymentNotifier }, onSuccess, onError);
}

void TransactionsEngine::addDrive(const std::string& driveAlias, const uint64_t& driveSize, const ushort replicatorsCount) {
    xpx_chain_sdk::Amount verificationFeeAmount = 100;
    auto prepareDriveTransaction = xpx_chain_sdk::CreatePrepareBcDriveTransaction(driveSize, verificationFeeAmount, replicatorsCount, std::nullopt, std::nullopt, mpChainClient->getConfig()->NetworkId);
    mpChainAccount->signTransaction(prepareDriveTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> prepareDriveNotifier;

    statusNotifier.set([this, driveAlias, prepareDriveNotifierId = prepareDriveNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        qWarning() << notification.status.c_str() << " hash: " << notification.hash.c_str();
        emit createDriveTransactionFailed(driveAlias, rawHashFromHex(notification.hash.c_str()), notification.status.c_str());

        mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [this](auto code) {
            qWarning() << code.message().c_str();
        }, { prepareDriveNotifierId });

        mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [this](auto code) {
            qWarning() << code.message().c_str();
        }, { id });
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, [](){}, [](auto errorCode) {
        qCritical() << errorCode.message().c_str();
    });

    const std::string hash = rawHashToHex(prepareDriveTransaction->hash()).toStdString();
    //const auto fsTree = mDrivesFsTrees[prepareDriveTransaction->hash()];

    prepareDriveNotifier.set([this, hash, driveAlias, statusNotifierId = statusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << "confirmed PrepareDrive transaction hash: " << hash.c_str();
            mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [](auto code) {
                qWarning() << code.message().c_str();
            }, { statusNotifierId });

            mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [](auto code) {
                qWarning() << code.message().c_str();
            }, { id });

            auto rawDrivePubKey = rawHashFromHex(notification.meta.hash.c_str());
            //showDrive(rawDrivePubKey);
            //saveFsTree(rawDrivePubKey, fsTree);

            emit createDriveTransactionConfirmed(driveAlias, rawDrivePubKey);
        }
    });

    auto onSuccess = [this, data = prepareDriveTransaction->binary()](){
        mpBlockchainEngine->announceNewTransaction(data, [](auto isAnnounced, auto isSuccess, auto message, auto code){
            if (!isSuccess) {
                qWarning() << message.c_str() << " : " << code.c_str();
            }
        });
    };

    auto onError = [hash](auto errorCode){
        qCritical() << errorCode.message().c_str() << " hash: " << hash.c_str();
    };

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { prepareDriveNotifier }, onSuccess, onError);
}

void TransactionsEngine::closeDrive(const std::array<uint8_t, 32>& rawDrivePubKey) {
    QString drivePubKey = rawHashToHex(rawDrivePubKey);

    xpx_chain_sdk::Key driveKey;
    xpx_chain_sdk::ParseHexStringIntoContainer(drivePubKey.toStdString().c_str(), drivePubKey.size(), driveKey);

    auto driveClosureTransaction = xpx_chain_sdk::CreateDriveClosureTransaction(driveKey, std::nullopt, std::nullopt, mpChainClient->getConfig()->NetworkId);
    mpChainAccount->signTransaction(driveClosureTransaction.get());

    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification> statusNotifier;
    xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification> driveClosureNotifier;

    statusNotifier.set([this, drivePubKey, driveClosureNotifierId = driveClosureNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionStatusNotification& notification) {
        //emit log(Error, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), "onRemoveDrive : ", notification.status.c_str(), notification.hash.c_str()});
        //emit driveClosureTransactionFailed(drivePubKey, notification.status.c_str());

        mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [](auto code) {
            qWarning() << code.message().c_str();
        }, { driveClosureNotifierId });

        mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [](auto code) {
            qWarning() << code.message().c_str();
        }, { id });
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, [](){}, [](auto errorCode) {
        qCritical() << errorCode.message().c_str();
    });

    const std::string hash = rawHashToHex(driveClosureTransaction->hash()).toStdString();

    driveClosureNotifier.set([this, hash, rawDrivePubKey, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            qInfo() << "confirmed driveClosureTransaction hash: " << hash.c_str();
            mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [](auto code) {
                qWarning() << code.message().c_str();
            }, { statusNotifierId });

            mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [](auto code) {
                qWarning() << code.message().c_str();
            }, { id });

            //emit driveClosureTransactionConfirmed(rawDrivePubKey);
        }
    });

    auto onSuccess = [this, data = driveClosureTransaction->binary()]() {
        mpBlockchainEngine->announceNewTransaction(data, [](auto isAnnounced, auto isSuccess, auto message, auto code){
            if (!isSuccess) {
                qWarning() << message.c_str() << " : " << code.c_str();
            }
        });
    };

    auto onError = [hash](auto errorCode) {
        qCritical() << errorCode.message().c_str() << " hash: " << hash.c_str();
    };

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { driveClosureNotifier }, onSuccess, onError);
}