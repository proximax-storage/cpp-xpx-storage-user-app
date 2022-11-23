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
    Q_ENUM()

    public:
        OnChainClient(StorageEngine* storage,
                      const std::string& privateKey,
                      const std::string& address,
                      const std::string& port,
                      QObject* parent = nullptr);

        ~OnChainClient() = default;

    public:
        enum ChannelsType { MyOwn, Sponsored, Others };

    public:
        void loadDrives(const xpx_chain_sdk::DrivesPageOptions& options);
        void loadDownloadChannels(const xpx_chain_sdk::DownloadChannelsPageOptions& options);

        BlockchainEngine* getBlockchainEngine();

        std::string addDownloadChannel(const std::string& channelAlias,
                                       const std::vector<std::array<uint8_t, 32>>& listOfAllowedPublicKeys,
                                       const std::array<uint8_t, 32>& drivePubKey,
                                       const uint64_t& prepaidSize,
                                       const uint64_t& feedbacksNumber);

        void closeDownloadChannel(const std::array<uint8_t, 32>& channelId);
        std::string addDrive(const std::string& driveAlias, const uint64_t& driveSize, ushort replicatorsCount);
        void closeDrive(const std::array<uint8_t, 32>& rawDrivePubKey);
        void cancelDataModification(const std::array<uint8_t, 32> &driveId);
        void applyDataModification(const std::array<uint8_t, 32>& driveKey,
                                   const sirius::drive::ActionList& actions,
                                   const std::string& sandboxFolder);

        void downloadPayment(const std::array<uint8_t, 32>& channelId, uint64_t amount);
        void storagePayment(const std::array<uint8_t, 32> &driveId, const uint64_t& amount);

    signals:
        void initializedSuccessfully(const QString& networkName);
        void initializedFailed(const QString& error);
        void dataModificationTransactionConfirmed(const std::array<uint8_t, 32>& modificationId);
        void dataModificationTransactionFailed(const std::array<uint8_t, 32>& modificationId);
        void drivesLoaded(const std::vector<xpx_chain_sdk::drives_page::DrivesPage>& drivesPages);
        void downloadChannelsLoaded(ChannelsType type, const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& channelsPages);
        void downloadChannelOpenTransactionConfirmed(const std::string& channelAlias, const std::array<uint8_t, 32>& channelId, const std::array<uint8_t, 32>& driveKey);
        void downloadChannelOpenTransactionFailed(const QString& channelId, const QString& errorText);
        void downloadChannelCloseTransactionConfirmed(const std::array<uint8_t, 32>& channelId);
        void downloadChannelCloseTransactionFailed(const QString& errorText);
        void fsTreeDownloaded(const std::string& driveId, const std::array<uint8_t, 32>& fsTreeHash, const sirius::drive::FsTree& fsTree);
        void prepareDriveTransactionConfirmed(const std::string& driveAlias, const std::array<uint8_t, 32>& driveKey);
        void prepareDriveTransactionFailed(const std::string& driveAlias, const std::array<uint8_t, 32>& driveKey, const QString& errorText);
        void closeDriveTransactionConfirmed(const std::array<uint8_t, 32>& driveKey);
        void closeDriveTransactionFailed(const QString& errorText);
        void downloadPaymentTransactionConfirmed(const std::array<uint8_t, 32>& channelId);
        void downloadPaymentTransactionFailed(const std::array<uint8_t, 32> &channelId, const QString& errorText);
        void storagePaymentTransactionConfirmed(const std::array<uint8_t, 32>& driveKey);
        void storagePaymentTransactionFailed(const std::array<uint8_t, 32> &driveId, const QString& errorText);
        void dataModificationApprovalTransactionConfirmed(const std::array<uint8_t, 32>& driveId, const std::string& fileStructureCdi);
        void dataModificationApprovalTransactionFailed(const std::array<uint8_t, 32>& driveId );
        void cancelModificationTransactionConfirmed(const std::array<uint8_t, 32>& driveId, const QString& modificationId);
        void cancelModificationTransactionFailed(const QString& modificationId);

    // internal signals
    signals:
        void downloadChannelsPageLoaded(const QUuid& id, ChannelsType type, const xpx_chain_sdk::download_channels_page::DownloadChannelsPage& drivesPages);
        void drivesPageLoaded(const QUuid& id, const xpx_chain_sdk::drives_page::DrivesPage& drivesPages);

    private:
        void initConnects();
        void initAccount(const std::string& privateKey);
        void init(const std::string& address,
                  const std::string& port,
                  const std::string& privateKey);
        void loadMyOwnChannels(const QUuid& id, xpx_chain_sdk::download_channels_page::DownloadChannelsPage channelsPage);
        void loadSponsoredChannels(const QUuid& id, xpx_chain_sdk::download_channels_page::DownloadChannelsPage channelsPage);

    private:
        StorageEngine* mpStorageEngine;
        BlockchainEngine* mpBlockchainEngine;
        TransactionsEngine* mpTransactionsEngine;
        std::shared_ptr<xpx_chain_sdk::Account> mpChainAccount;
        std::shared_ptr<xpx_chain_sdk::IClient> mpChainClient;
        std::map<QUuid, std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>> mLoadedChannels;
        std::map<QUuid, std::vector<xpx_chain_sdk::drives_page::DrivesPage>> mLoadedDrives;
};

Q_DECLARE_METATYPE(OnChainClient::ChannelsType)

#endif //ONCHAINCLIENT_H
