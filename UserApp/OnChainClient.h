#ifndef ONCHAINCLIENT_H
#define ONCHAINCLIENT_H

#include <QObject>
#include <memory>
#include <xpxchaincpp/sdk.h>

#include "Utils.h"
#include "Engines/StorageEngine.h"
#include "Engines/BlockchainEngine.h"
#include "Engines/TransactionsEngine.h"
#include "ContractDeploymentData.h"
#include "SignalEmitters/DialogSignals.h"

class OnChainClient : public QObject
{
    Q_OBJECT

    public:
        OnChainClient(StorageEngine* storage,
                      QObject* parent = nullptr);

        ~OnChainClient() = default;

    public:
        enum ChannelsType { MyOwn, Sponsored, Others };
        Q_ENUM(ChannelsType)

    public:
        void start(const std::string& privateKey,
                   const std::string& address,
                   const std::string& port,
                   double feeMultiplier);

        void loadDrives(const xpx_chain_sdk::DrivesPageOptions& options);
        void loadDownloadChannels(const xpx_chain_sdk::DownloadChannelsPageOptions& options);

        BlockchainEngine* getBlockchainEngine();

        void addDownloadChannel(const std::string& channelAlias,
                                       const std::vector<std::array<uint8_t, 32>>& listOfAllowedPublicKeys,
                                       const std::array<uint8_t, 32>& drivePubKey,
                                       const uint64_t& prepaidSize,
                                       const uint64_t& feedbacksNumber,
                                       const std::function<void(std::string hash)>& callback);

        void closeDownloadChannel(const std::array<uint8_t, 32>& channelId);
        void addDrive(const uint64_t& driveSize, ushort replicatorsCount, const std::function<void(std::string hash)>& callback);
        void closeDrive(const std::array<uint8_t, 32>& rawDrivePubKey);
        void cancelDataModification(const std::array<uint8_t, 32> &driveId);
        void applyDataModification(const std::array<uint8_t, 32>& driveKey,
                                   const sirius::drive::ActionList& actions);

        void downloadPayment(const std::array<uint8_t, 32>& channelId, uint64_t amount);
        void storagePayment(const std::array<uint8_t, 32> &driveId, const uint64_t& amount);
        void replicatorOnBoarding(const QString& replicatorPrivateKey, const QString& nodeBootPrivateKey, uint64_t capacityMB, bool isExists);
        void replicatorOffBoarding(const std::array<uint8_t, 32> &driveId, const QString& replicatorPrivateKey);

        void calculateUsedSpaceOfReplicator(const QString& publicKey, std::function<void(uint64_t index, uint64_t usedSpace)> callback);

        void deployContract(const std::array<uint8_t, 32>& driveKey, const ContractDeploymentData& data);
        void runContract(const ContractManualCallData& data);

        void streamStart(const std::array<uint8_t, 32>& rawDrivePubKey,
                         const std::string& folderName,
                         uint64_t expectedUploadSizeMegabytes,
                         uint64_t feedbackFeeAmount,
                         const std::function<void(std::string hash)>& callback);

        void streamFinish(const std::array<uint8_t, 32>& rawDrivePubKey,
                          const std::array<uint8_t, 32>& streamId,
                          uint64_t actualUploadSizeMegabytes,
                          const std::array<uint8_t, 32>& streamStructureCdi);

        void streamPayment(const std::array<uint8_t, 32>& rawDrivePubKey,
                           const std::array<uint8_t, 32>& streamId,
                           uint64_t additionalUploadSizeMegabytes);

        TransactionsEngine* transactionsEngine() { return mpTransactionsEngine; }
        StorageEngine* getStorageEngine();
        DialogSignals* getDialogSignalsEmitter();

    signals:
        void connectedToServer();
        void networkInitialized(const QString& networkName);
        void updateBalance();
        void dataModificationTransactionConfirmed(const std::array<uint8_t, 32>& driveKey, const std::array<uint8_t, 32>& modificationId);
        void dataModificationTransactionFailed(const std::array<uint8_t, 32>& driveKey, const std::array<uint8_t, 32>& modificationId, const QString& status);
        void drivesLoaded(const std::vector<xpx_chain_sdk::drives_page::DrivesPage>& drivesPages);
        void downloadChannelsLoaded(ChannelsType type, const std::vector<xpx_chain_sdk::download_channels_page::DownloadChannelsPage>& channelsPages);
        void downloadChannelOpenTransactionConfirmed(const std::string& channelAlias, const std::string& channelId, const std::string& driveKey);
        void downloadChannelOpenTransactionFailed(const std::string& channelId, const std::string& errorText);
        void downloadChannelCloseTransactionConfirmed(const std::array<uint8_t, 32>& channelId);
        void downloadChannelCloseTransactionFailed(const std::array<uint8_t, 32>& channelId, const QString& errorText);
        void prepareDriveTransactionConfirmed(const std::array<uint8_t, 32>& driveKey);
        void prepareDriveTransactionFailed(const std::array<uint8_t, 32>& driveKey, const QString& errorText);
        void closeDriveTransactionConfirmed(const std::array<uint8_t, 32>& driveKey);
        void closeDriveTransactionFailed(const std::array<uint8_t, 32>& driveKey, const QString& errorText);
        void downloadFsTreeDirect(const std::string& driveId, const std::string& fileStructureCdi);
        void downloadPaymentTransactionConfirmed(const std::array<uint8_t, 32>& channelId);
        void downloadPaymentTransactionFailed(const std::array<uint8_t, 32> &channelId, const QString& errorText);
        void storagePaymentTransactionConfirmed(const std::array<uint8_t, 32>& driveKey);
        void storagePaymentTransactionFailed(const std::array<uint8_t, 32> &driveId, const QString& errorText);
        void dataModificationApprovalTransactionConfirmed(const std::array<uint8_t, 32>& driveId, const std::string& fileStructureCdi);
        void dataModificationApprovalTransactionFailed(const std::array<uint8_t, 32>& driveId, const std::string& fileStructureCdi, uint8_t code);
        void cancelModificationTransactionConfirmed(const std::array<uint8_t, 32>& driveId, const QString& modificationId);
        void cancelModificationTransactionFailed(const std::array<uint8_t, 32>& driveId, const QString& modificationId, const QString& error);
        void replicatorOffBoardingTransactionConfirmed(const QString& replicatorPublicKey);
        void replicatorOffBoardingTransactionFailed(const QString& replicatorPublicKey, const QString& error);
        void replicatorOnBoardingTransactionConfirmed(const QString& replicatorPublicKey);
        void replicatorOnBoardingTransactionFailed(const QString& replicatorPublicKey, const QString& replicatorPrivateKey, const QString& error);

        void deployContractTransactionConfirmed(std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId);
        void deployContractTransactionFailed(std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId);
        void deployContractTransactionApprovalConfirmed(std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId);
        void deployContractTransactionApprovalFailed(std::array<uint8_t, 32> driveKey, std::array<uint8_t, 32> contractId);

        void manualCallTransactionConfirmed(std::array<uint8_t, 32> contractId, std::array<uint8_t, 32> callId);
        void manualCallTransactionFailed(std::array<uint8_t, 32> contractId, std::array<uint8_t, 32> callId);

        void streamStartTransactionConfirmed(const std::array<uint8_t, 32> &streamId);
        void streamStartTransactionFailed(const std::array<uint8_t, 32> &streamId, const QString& errorText);
        void streamFinishTransactionConfirmed(const std::array<uint8_t, 32> &streamId);
        void streamFinishTransactionFailed(const std::array<uint8_t, 32> &streamId, const QString& errorText);
        void streamPaymentTransactionConfirmed(const std::array<uint8_t, 32> &streamId);
        void streamPaymentTransactionFailed(const std::array<uint8_t, 32> &streamId, const QString& errorText);

        void newNotification(const QString& notification);
        void newError(int errorType, const QString& errorText);

    // internal signals
    signals:
        void downloadChannelsPageLoaded(const QUuid& id, ChannelsType type, const xpx_chain_sdk::download_channels_page::DownloadChannelsPage& drivesPages);
        void drivesPageLoaded(const QUuid& id, const xpx_chain_sdk::drives_page::DrivesPage& drivesPages);

    private:
        void initConnects();
        void initAccount(const std::string& privateKey);
        void init(const std::string& address,
                  const std::string& port,
                  const double feeMultiplier,
                  const std::string& privateKey);
        void loadMyOwnChannels(const QUuid& id, xpx_chain_sdk::download_channels_page::DownloadChannelsPage channelsPage);
        void loadSponsoredChannels(const QUuid& id, xpx_chain_sdk::download_channels_page::DownloadChannelsPage channelsPage);

    private slots:
        void onConnected(xpx_chain_sdk::Config& config, const std::string& privateKey);

    private:
        DialogSignals* mpDialogSignalsEmitter;
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
