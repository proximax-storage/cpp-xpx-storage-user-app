#ifndef ONCHAINCLIENT_H
#define ONCHAINCLIENT_H

#include <QObject>
#include <memory>
#include <xpxchaincpp/sdk.h>

#include "StorageEngine.h"
#include "BlockchainEngine.h"
#include "TransactionsEngine.h"
#include "Utils.h"

class OnChainClient : public QObject
{
    Q_OBJECT

    public:
        OnChainClient(std::shared_ptr<StorageEngine> storage,
                      const std::string& privateKey,
                      const std::string& address,
                      const std::string& port,
                      QObject* parent = nullptr);

        ~OnChainClient() = default;

    public:
        void loadDrives();
        void loadDownloadChannels(const QString& drivePubKey);
        std::shared_ptr<BlockchainEngine> getBlockchainEngine();

        std::string addDownloadChannel(const std::string& channelAlias,
                                       const std::vector<std::array<uint8_t, 32>>& listOfAllowedPublicKeys,
                                       const std::array<uint8_t, 32>& drivePubKey,
                                       const uint64_t& prepaidSize,
                                       const uint64_t& feedbacksNumber);

        void closeDownloadChannel(const std::array<uint8_t, 32>& channelId);
        std::string addDrive(const std::string& driveAlias, const uint64_t& driveSize, ushort replicatorsCount);
        void closeDrive(const std::array<uint8_t, 32>& rawDrivePubKey);

        void applyDataModification(const std::array<uint8_t, 32>& driveKey,
                                   const sirius::drive::ActionList& actions,
                                   const std::array<uint8_t, 32> &channelId,
                                   const std::string& sandboxFolder);

        void downloadPayment(const std::array<uint8_t, 32>& channelId, uint64_t amount);
        void storagePayment(const std::array<uint8_t, 32> &driveId, const uint64_t& amount);

    signals:
        void initializedSuccessfully();
        void initializedFailed(const QString& error);
        void dataModificationTransactionConfirmed(const std::array<uint8_t, 32>& modificationId);
        void dataModificationTransactionFailed(const std::array<uint8_t, 32>& modificationId);
        void drivesLoaded(const QStringList& drives);
        void downloadChannelsLoaded(const QStringList& channels);
        void downloadChannelOpenTransactionConfirmed(const std::string& channelAlias, const std::array<uint8_t, 32>& channelId, const std::array<uint8_t, 32>& driveKey);
        void downloadChannelOpenTransactionFailed(const QString& channelId, const QString& errorText);
        void downloadChannelCloseTransactionConfirmed(const std::array<uint8_t, 32>& channelId);
        void downloadChannelCloseTransactionFailed(const QString& errorText);
        void fsTreeDownloaded(const std::string& driveId, const std::array<uint8_t, 32>& fsTreeHash, const sirius::drive::FsTree& fsTree);
        void prepareDriveTransactionConfirmed(const std::string& driveAlias, const std::array<uint8_t, 32>& driveKey);
        void prepareDriveTransactionFailed(const std::string& driveAlias, const std::array<uint8_t, 32>& driveKey, const QString& errorText);
        void closeDriveTransactionConfirmed(const std::array<uint8_t, 32>& driveKey);
        void closeDriveTransactionFailed(const QString& errorText);
        void downloadPaymentTransactionConfirmed(const std::array<uint8_t, 32>& driveKey);
        void downloadPaymentTransactionFailed(const QString& errorText);
        void storagePaymentTransactionConfirmed(const std::array<uint8_t, 32>& driveKey);
        void storagePaymentTransactionFailed(const QString& errorText);

    private:
        void initConnects();
        void initAccount(const std::string& privateKey);
        void init(const std::string& address,
                  const std::string& port,
                  const std::string& privateKey);

    private:
        std::shared_ptr<StorageEngine> mpStorageEngine;
        std::shared_ptr<BlockchainEngine> mpBlockchainEngine;
        std::shared_ptr<TransactionsEngine> mpTransactionsEngine;
        std::shared_ptr<xpx_chain_sdk::Account> mpChainAccount;
        std::shared_ptr<xpx_chain_sdk::IClient> mpChainClient;
};


#endif //ONCHAINCLIENT_H
