#ifndef TRANSACTIONSENGINE_H
#define TRANSACTIONSENGINE_H

#include <QObject>
#include <xpxchaincpp/sdk.h>
#include <xpxchaincpp/model/storage/drive.h>
#include <Engines/BlockchainEngine.h>
#include <utils/HexFormatter.h>
#include <drive/ActionList.h>
#include "ContractDeploymentData.h"
#include "ContractManualCallData.h"

class TransactionsEngine : public QObject
{
    Q_OBJECT

    public:
        TransactionsEngine(std::shared_ptr<xpx_chain_sdk::IClient> client,
                           std::shared_ptr<xpx_chain_sdk::Account> account,
                           BlockchainEngine* blockchainEngine,
                           QObject* parent = nullptr);
        ~TransactionsEngine() = default;

    public:
        void addDownloadChannel(const std::string& channelAlias,
                                const std::vector<std::array<uint8_t, 32>>& listOfAllowedPublicKeys,
                                const std::array<uint8_t, 32>& drivePubKey,
                                const uint64_t& prepaidSize,
                                const uint64_t& feedbacksNumber,
                                const std::optional<xpx_chain_sdk::NetworkDuration>& deadline,
                                const std::function<void(std::string hash)>& callback);

        void closeDownloadChannel(const std::array<uint8_t, 32>& channelId, const std::optional<xpx_chain_sdk::NetworkDuration>& deadline);
        void downloadPayment(const std::array<uint8_t, 32>& channelId, uint64_t prepaidSize, const std::optional<xpx_chain_sdk::NetworkDuration>& deadline);
        void storagePayment(const std::array<uint8_t, 32> &driveId, const uint64_t& amount, const std::optional<xpx_chain_sdk::NetworkDuration>& deadline);
        void addDrive(const uint64_t& driveSize, ushort replicatorsCount,
                      const std::optional<xpx_chain_sdk::NetworkDuration>& deadline,
                      const std::function<void(std::string hash)>& callback);
        void closeDrive(const std::array<uint8_t, 32>& rawDrivePubKey, const std::optional<xpx_chain_sdk::NetworkDuration>& deadline);
        void cancelDataModification(const xpx_chain_sdk::Drive& drive, const std::optional<xpx_chain_sdk::NetworkDuration>& deadline);
        void applyDataModification(const std::array<uint8_t, 32>& driveId,
                                   const sirius::drive::ActionList& actions,
                                   const std::vector<xpx_chain_sdk::Address>& addresses,
                                   const std::vector<std::string>& replicators,
                                   const std::optional<xpx_chain_sdk::NetworkDuration>& deadline);
        void replicatorOnBoarding(const QString& replicatorPrivateKey, const QString& nodeBootPrivateKey, uint64_t capacityMB, const std::optional<xpx_chain_sdk::NetworkDuration>& deadline);
        void replicatorOffBoarding(const std::array<uint8_t, 32> &driveId, const QString& replicatorPrivateKey, const std::optional<xpx_chain_sdk::NetworkDuration>& deadline);
        static bool isValidHash(const std::array<uint8_t, 32>& hash);
        std::array<uint8_t, 32> getLatestModificationId(const std::array<uint8_t, 32> &driveId);
        bool isModificationsPresent(const std::array<uint8_t, 32> &driveId);

        void deployContract( const std::array<uint8_t, 32>& driveId,
                             const ContractDeploymentData& data,
                             const std::vector<xpx_chain_sdk::Address>& replicators,
                             const std::optional<xpx_chain_sdk::NetworkDuration>& deadline );

        void runManualCall( const ContractManualCallData& manualCallData,
                            const std::vector<xpx_chain_sdk::Address>& replicators,
                            const std::optional<xpx_chain_sdk::NetworkDuration>& deadline );

        void streamStart(const std::array<uint8_t, 32>& rawDrivePubKey,
                                const std::string& folderName,
                                uint64_t expectedUploadSizeMegabytes,
                                uint64_t feedbackFeeAmount,
                                const std::optional<xpx_chain_sdk::NetworkDuration>& deadline,
                                const std::function<void(std::string hash)>& callback);

        void streamFinish(const std::array<uint8_t, 32>& rawDrivePubKey,
                          const std::array<uint8_t, 32>& streamId,
                          uint64_t actualUploadSizeMegabytes,
                          const std::array<uint8_t, 32>& streamStructureCdi,
                          const std::optional<xpx_chain_sdk::NetworkDuration>& deadline,
                          const std::vector<xpx_chain_sdk::Address>& replicatorAddresses);

        void streamPayment(const std::array<uint8_t, 32>& rawDrivePubKey,
                           const std::array<uint8_t, 32>& streamId,
                           uint64_t additionalUploadSizeMegabytes,
                           const std::optional<xpx_chain_sdk::NetworkDuration>& deadline);

    signals:
        void createDownloadChannelConfirmed(const std::string& channelAlias, const std::array<uint8_t, 32>& channelId, const std::array<uint8_t, 32>& rawDrivePubKey);
        void createDownloadChannelFailed(const QString& channelId, const QString& errorText);
        void closeDownloadChannelConfirmed(const std::array<uint8_t, 32>& channelId);
        void closeDownloadChannelFailed(const std::array<uint8_t, 32>& channelId, const QString& errorText);
        void createDriveConfirmed(const std::array<uint8_t, 32>& driveId);
        void createDriveFailed(const std::array<uint8_t, 32>& driveKey, const QString& errorText);
        void closeDriveConfirmed(const std::array<uint8_t, 32>& driveId);
        void closeDriveFailed(const std::array<uint8_t, 32>& driveKey, const QString& errorText);
        void downloadPaymentConfirmed(const std::array<uint8_t, 32> &channelId);
        void downloadPaymentFailed(const std::array<uint8_t, 32> &channelId, const QString& errorText);
        void storagePaymentConfirmed(const std::array<uint8_t, 32> &driveId);
        void storagePaymentFailed(const std::array<uint8_t, 32> &driveId, const QString& errorText);

        void addActions(const sirius::drive::ActionList& actionList,
                        const sirius::Key& drivePublicKey,
                        const std::string& sandboxFolder,
                        const std::vector<std::string>& replicators,
                        std::function<void(uint64_t totalModifySize, std::array<uint8_t, 32>)> callback);

        void dataModificationApprovalConfirmed(const std::array<uint8_t, 32>& driveId, const std::string& fileStructureCdi, bool isStream);
        void dataModificationApprovalFailed(const std::array<uint8_t, 32>& driveId, const std::string& fileStructureCdi, uint8_t errorCode);
        void dataModificationConfirmed(const std::array<uint8_t, 32>& driveId, const std::array<uint8_t, 32>& modificationId);
        void dataModificationFailed(const std::array<uint8_t, 32>& driveId, const std::array<uint8_t, 32>& modificationId, const QString& errorText);
        void cancelModificationConfirmed(const std::array<uint8_t, 32>& driveId, const QString& modificationId);
        void cancelModificationFailed(const std::array<uint8_t, 32>& driveId, const QString& modificationId, const QString& error);
        void modificationCreated(const QString& driveId, const std::array<uint8_t,32>& modificationId);
        void replicatorOffBoardingConfirmed(const QString& replicatorPublicKey);
        void replicatorOffBoardingFailed(const QString& replicatorPublicKey, const QString& error);
        void replicatorOnBoardingConfirmed(const QString& replicatorPublicKey);
        void replicatorOnBoardingFailed(const QString& replicatorPublicKey, const QString& replicatorPrivateKey, const QString& error);
        void newError(const QString& errorText);
        void removeTorrent(const std::array<uint8_t, 32>& torrentId);

        void deployContractInitiated(std::array<uint8_t, 32> driveId, std::array<uint8_t, 32> contractId);
        void deployContractConfirmed(std::array<uint8_t, 32> driveId, std::array<uint8_t, 32> contractId);
        void deployContractFailed(std::array<uint8_t, 32> driveId, std::array<uint8_t, 32> contractId);
        void deployContractApprovalConfirmed(std::array<uint8_t, 32> driveId, std::array<uint8_t, 32> contractId);
        void deployContractApprovalFailed(std::array<uint8_t, 32> driveId, std::array<uint8_t, 32> contractId);

        void manualCallInitiated(std::array<uint8_t, 32> contractId, std::array<uint8_t, 32> callId);
        void manualCallConfirmed(std::array<uint8_t, 32> contractId, std::array<uint8_t, 32> callId);
        void manualCallFailed(std::array<uint8_t, 32> contractId, std::array<uint8_t, 32> callId);

        void streamStartConfirmed(const std::array<uint8_t, 32> &streamId);
        void streamStartFailed(const std::array<uint8_t, 32> &streamId, const QString& errorText);
        void streamFinishConfirmed(const std::array<uint8_t, 32> &streamId);
        void streamFinishFailed(const std::array<uint8_t, 32> &streamId, const QString& errorText);
        void streamPaymentConfirmed(const std::array<uint8_t, 32> &streamId);
        void streamPaymentFailed(const std::array<uint8_t, 32> &streamId, const QString& errorText);

    private:
        void subscribeOnReplicators(const std::vector<xpx_chain_sdk::Address>& addresses,
                                    const xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionNotification>& notifier,
                                    const xpx_chain_sdk::Notifier<xpx_chain_sdk::TransactionStatusNotification>& statusNotifier);
        void unsubscribeFromReplicators(const std::vector<xpx_chain_sdk::Address>& addresses,
                                        const std::string& notifierId,
                                        const std::string& statusNotifierId);

        void sendModification(const std::array<uint8_t, 32>& driveId,
                              const std::array<uint8_t, 32>& infoHash,
                              const sirius::drive::ActionList& actionList,
                              uint64_t totalModifySize,
                              const std::vector<xpx_chain_sdk::Address>& replicators,
                              const std::optional<xpx_chain_sdk::NetworkDuration>& deadline);

        void removeConfirmedAddedNotifier(const xpx_chain_sdk::Address& address,
                                          const std::string& id,
                                          std::function<void()> onSuccess = {},
                                          std::function<void(boost::beast::error_code errorCode)> onError = {});

        void removeUnconfirmedAddedNotifier(const xpx_chain_sdk::Address& address,
                                            const std::string &id,
                                            std::function<void()> onSuccess = {},
                                            std::function<void(boost::beast::error_code errorCode)> onError = {});

        void removeStatusNotifier(const xpx_chain_sdk::Address& address,
                                  const std::string& id,
                                  std::function<void()> onSuccess = {},
                                  std::function<void(boost::beast::error_code errorCode)> onError = {});

        void announceTransaction(const std::vector<uint8_t>& data);

        void onError(const std::string& transactionId, boost::beast::error_code errorCode);

        QString findFile(const QString& fileName, const QString& directory);

        void removeDriveModifications(const QString& pathToActionList, const QString& pathToSandbox);

        void removeFile(const QString& path);

        void sendContractDeployment( const std::array<uint8_t, 32>& driveId,
                                     const ContractDeploymentData& data,
                                     const std::vector<xpx_chain_sdk::Address>& replicators,
                                     const std::optional<xpx_chain_sdk::NetworkDuration>& deadline);

        void sendManualCall( const ContractManualCallData& data,
                             const std::vector<xpx_chain_sdk::Address>& replicators,
                             const std::optional<xpx_chain_sdk::NetworkDuration>& deadline);

    private:
        struct ModificationEntity
        {
            bool isConfirmed = false;
            bool isApproved = false;
            std::array<uint8_t, 32> id = { 0 };
        };

    private:
        std::string mSandbox;
        std::shared_ptr<xpx_chain_sdk::Account> mpChainAccount;
        std::shared_ptr<xpx_chain_sdk::IClient> mpChainClient;
        BlockchainEngine* mpBlockchainEngine;

        // driveId { modification { is confirmed by BC?, is approved by user? } }
        std::map<std::array<uint8_t, 32>, std::vector<ModificationEntity>> mDataModifications;
        std::map<std::array<uint8_t, 32>, std::set<std::array<uint8_t, 32>>> mDataModificationApprovals;
};

#endif //TRANSACTIONSENGINE_H
