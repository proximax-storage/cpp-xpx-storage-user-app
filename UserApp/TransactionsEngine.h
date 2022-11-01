#ifndef TRANSACTIONSENGINE_H
#define TRANSACTIONSENGINE_H

#include <QObject>
#include <xpxchaincpp/sdk.h>
#include <BlockchainEngine.h>
#include <utils/HexFormatter.h>

class TransactionsEngine : public QObject
{
    Q_OBJECT

    public:
        TransactionsEngine(std::shared_ptr<xpx_chain_sdk::IClient> client, std::shared_ptr<xpx_chain_sdk::Account> account, std::shared_ptr<BlockchainEngine> blockchainEngine);
        ~TransactionsEngine() = default;

    public:
        void addDownloadChannel(const std::string& channelAlias,
                                const std::vector<std::array<uint8_t, 32>>& listOfAllowedPublicKeys,
                                const std::array<uint8_t, 32>& drivePubKey,
                                const uint64_t& prepaidSize,
                                const uint64_t& feedbacksNumber);

        void closeDownloadChannel(const std::array<uint8_t, 32>& channelId);
        void downloadPayment(const std::array<uint8_t, 32>& channelId, uint64_t prepaidSize);
        void addDrive(const std::string& driveAlias, const uint64_t& driveSize, ushort replicatorsCount);
        void closeDrive(const std::array<uint8_t, 32>& rawDrivePubKey);


    signals:
        void createDownloadTransactionConfirmed(const std::string& channelAlias, const std::array<uint8_t, 32>& channelId, const std::array<uint8_t, 32>& rawDrivePubKey);
        void createDownloadTransactionFailed(const QString& channelId, const QString& errorText);
        void finishDownloadTransactionConfirmed(const std::array<uint8_t, 32>& channelId);
        void finishDownloadTransactionFailed(const QString& errorText);
        void createDriveTransactionConfirmed(const std::string& driveAlias, const std::array<uint8_t, 32>& driveId);
        void createDriveTransactionFailed(const std::string& driveAlias, const std::array<uint8_t, 32>& driveKey, const QString& errorText);

    private:
        std::shared_ptr<xpx_chain_sdk::Account> mpChainAccount;
        std::shared_ptr<xpx_chain_sdk::IClient> mpChainClient;
        std::shared_ptr<BlockchainEngine> mpBlockchainEngine;
};

#endif //TRANSACTIONSENGINE_H
