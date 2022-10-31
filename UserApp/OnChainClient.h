#ifndef ONCHAINCLIENT_H
#define ONCHAINCLIENT_H

#include <QObject>
#include <memory>
#include <xpxchaincpp/sdk.h>

#include "TransactionsEngine.h"
#include "BlockchainEngine.h"

class OnChainClient : public QObject
{
    Q_OBJECT

    public:
        OnChainClient(const std::string& privateKey, const std::string& address, const std::string& port, QObject* parent = nullptr);
        ~OnChainClient() = default;

    public:
        void loadDrives();
        void loadDownloadChannels(const QString& drivePubKey);
        std::shared_ptr<BlockchainEngine> getBlockchainEngine();
        void addDownloadChannel(const std::vector<std::array<uint8_t, 32>>& listOfAllowedPublicKeys,
                                const std::array<uint8_t, 32>& drivePubKey,
                                const uint64_t& prepaidSize,
                                const uint64_t& feedbacksNumber);
        void closeDownloadChannel(const std::array<uint8_t, 32>& channelId);

    signals:
        void drivesLoaded(const QStringList& drives);
        void downloadChannelsLoaded(const QStringList& channels);
        void downloadChannelOpenTransactionConfirmed(const std::array<uint8_t, 32>& channelId, const std::array<uint8_t, 32>& rawDrivePubKey);
        void downloadChannelOpenTransactionFailed(const QString& channelId, const QString& errorText);
        void downloadChannelCloseTransactionConfirmed(const std::array<uint8_t, 32>& channelId);
        void downloadChannelCloseTransactionFailed(const QString& errorText);
        void prepareDriveTransactionConfirmed(const std::array<uint8_t, 32>& drivePubKey);
        void prepareDriveTransactionFailed(const QString& drivePubKey, const QString& errorText);

    private:
        std::shared_ptr<BlockchainEngine> mpBlockchainEngine;
        std::shared_ptr<TransactionsEngine> mpTransactionsEngine;
        std::shared_ptr<xpx_chain_sdk::Account> mpChainAccount;
        std::shared_ptr<xpx_chain_sdk::IClient> mpChainClient;
};


#endif //ONCHAINCLIENT_H
