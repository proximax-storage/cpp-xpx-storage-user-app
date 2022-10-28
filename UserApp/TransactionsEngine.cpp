#include "TransactionsEngine.h"

TransactionsEngine::TransactionsEngine(std::shared_ptr<xpx_chain_sdk::IClient> client,
                                       std::shared_ptr<xpx_chain_sdk::Account> account,
                                       std::shared_ptr<BlockchainEngine> blockchainEngine)
    : mpChainClient(client)
    , mpBlockchainEngine(blockchainEngine)
    , mpChainAccount(account)
{}

void TransactionsEngine::addDownloadChannel(const std::vector<std::array<uint8_t, 32>> &listOfAllowedPublicKeys,
                                            const std::array<uint8_t, 32> &rawDrivePubKey, const uint64_t &prepaidSize,
                                            const uint64_t &feedbacksNumber) {
    QString drivePubKey = rawHashToHex(rawDrivePubKey);
    //emit log(Info, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), ": drivePubKey: ", drivePubKey});

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
        //emit log(Error, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), "openDownloadChannel : ", notification.status.c_str(), notification.hash.c_str()});
        //emit downloadChannelOpenFailed(notification.hash.c_str(), notification.status.c_str());

        mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [this](auto code)
        {
            // error logs
        }, { downloadNotifierId });

        mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [this](auto code) {
            // error logs
        }, { id });
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, [](){}, [this](auto errorCode) {
        // error logs
    });

    const std::string hash = rawHashToHex(downloadTransaction->hash()).toStdString();

    downloadNotifier.set([this, hash, rawDrivePubKey, statusNotifierId = statusNotifier.getId()](
            const auto& id,
            const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            //emit log(Info, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), "confirmed downloadTransaction hash: ", hash.c_str()});

            mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [this](auto code) {
                // error logs
            }, { statusNotifierId });

            mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [this, hash](auto code) {
                // error logs
            }, { id });

            //emit balancesUpdated();
            //emit downloadChannelOpenConfirmed(rawHashFromHex(hash.c_str()), rawDrivePubKey);
        }
    });

    auto onSuccess = [this, data = downloadTransaction->binary(), type = downloadTransaction->type(), hash](){
        // TODO: logs
        mpBlockchainEngine->announceNewTransaction(data, [](auto isAnnounced, auto isSuccess, auto message, auto code){
            if (isSuccess) {
                // TODO: logs
            } else {
                // TODO: logs
            }
        });
    };

    auto onError = [this, hash](auto errorCode){
        //emit log(Error, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), "error addConfirmedAddedNotifiers: ", errorCode.message().c_str(), hash.c_str()});
    };

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { downloadNotifier }, onSuccess, onError);
}

void TransactionsEngine::closeDownloadChannel(const std::array<uint8_t, 32> &channelId) {
    QString channelIdHex = rawHashToHex(channelId);
    //emit log(Info, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), ": channelId: ", channelIdHex});

    if( channelId.empty()) {
        //emit log(Error, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), ": bad channelId: ", channelIdHex});
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
        //emit log(Error, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), "closeDownloadChannel : ", notification.status.c_str(), notification.hash.c_str()});
        //emit downloadChannelCloseFailed(notification.status.c_str());

        mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [this](auto code) {
            // error logs
        }, { finishDownloadNotifierId });

        mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [this](auto code) {
            // error logs
        }, { id });
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, [](){}, [this](auto errorCode) {
        // error logs
    });

    const std::string hash = rawHashToHex(finishDownloadTransaction->hash()).toStdString();

    finishDownloadNotifier.set([this, hash, channelId, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            //emit log(Info, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), "confirmed finishDownloadTransaction hash: ", hash.c_str()});

            mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [this](auto code) {
                // error logs
            } , { statusNotifierId });

            mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [this, hash](auto code) {
                // error logs
            }, { id });

            //emit downloadChannelCloseConfirmed(channelId);
        }
    });

    auto onSuccess = [this, data = finishDownloadTransaction->binary(), type = finishDownloadTransaction->type(), hash](){
        // TODO: logs
        mpBlockchainEngine->announceNewTransaction(data, [](auto isAnnounced, auto isSuccess, auto message, auto code){
            if (isSuccess) {
                // TODO: logs
            } else {
                // TODO: logs
            }
        });
    };

    auto onError = [this, hash](auto errorCode){
        //emit log(Error, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), "error addConfirmedAddedNotifiers: ", errorCode.message().c_str(), hash.c_str()});
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
            // error logs
        }, { downloadPaymentNotifierId });

        mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [this](auto code) {
            // error logs
        }, { id });
    });

    mpChainClient->notifications()->addStatusNotifiers(mpChainAccount->address(), { statusNotifier }, [](){}, [this](auto errorCode) {
        // error logs
    });

    const std::string hash = rawHashToHex(downloadPaymentTransaction->hash()).toStdString();

    downloadPaymentNotifier.set([this, hash, statusNotifierId = statusNotifier.getId()](const auto& id, const xpx_chain_sdk::TransactionNotification& notification) {
        if (notification.meta.hash == hash) {
            //emit log(Info, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), "confirmed downloadPaymentTransaction hash: ", hash.c_str()});

            mpChainClient->notifications()->removeStatusNotifiers(mpChainAccount->address(), [](){}, [this](auto code) {
                // error logs
            }, { statusNotifierId });

            mpChainClient->notifications()->removeConfirmedAddedNotifiers(mpChainAccount->address(), [](){}, [this, hash](auto code) {
                // error logs
            }, {id });

            //emit balancesUpdated();
            //emit downloadPaymentConfirmed();
        }
    });

    auto onSuccess = [this, data = downloadPaymentTransaction->binary(), type = downloadPaymentTransaction->type(), hash](){
        // TODO: logs
        mpBlockchainEngine->announceNewTransaction(data, [](auto isAnnounced, auto isSuccess, auto message, auto code){
            if (isSuccess) {
                // TODO: logs
            } else {
                // TODO: logs
            }
        });
    };

    auto onError = [this, hash](auto errorCode){
        //emit log(Error, {BOOST_CURRENT_FUNCTION, QString::number(__LINE__), "error addConfirmedAddedNotifiers: ", errorCode.message().c_str(), hash.c_str()});
    };

    mpChainClient->notifications()->addConfirmedAddedNotifiers(mpChainAccount->address(), { downloadPaymentNotifier }, onSuccess, onError);
}

QString TransactionsEngine::rawHashToHex(const std::array<uint8_t, 32>& rawHash) {
    std::ostringstream stream;
    stream << sirius::utils::HexFormat(rawHash);
    return stream.str().c_str();
}

std::array<uint8_t, 32> TransactionsEngine::rawHashFromHex(const QString& hex) {
    std::array<uint8_t, 32> hash{};
    xpx_chain_sdk::ParseHexStringIntoContainer(hex.toStdString().c_str(), hex.size(), hash);
    return hash;
};