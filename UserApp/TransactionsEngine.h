#ifndef TRANSACTIONSENGINE_H
#define TRANSACTIONSENGINE_H

#include <QObject>
#include <xpxchaincpp/sdk.h>
#include <BlockchainEngine.h>
#include <utils/HexFormatter.h>
#include <drive/ActionList.h>

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
        std::string addDownloadChannel(const std::string& channelAlias,
                                       const std::vector<std::array<uint8_t, 32>>& listOfAllowedPublicKeys,
                                       const std::array<uint8_t, 32>& drivePubKey,
                                       const uint64_t& prepaidSize,
                                       const uint64_t& feedbacksNumber);

        void closeDownloadChannel(const std::array<uint8_t, 32>& channelId);
        void downloadPayment(const std::array<uint8_t, 32>& channelId, uint64_t prepaidSize);
        void storagePayment(const std::array<uint8_t, 32> &driveId, const uint64_t& amount);
        std::string addDrive(const std::string& driveAlias, const uint64_t& driveSize, ushort replicatorsCount);
        void closeDrive(const std::array<uint8_t, 32>& rawDrivePubKey);
        void cancelDataModification(const std::array<uint8_t, 32> &driveKey);
        void applyDataModification(const std::array<uint8_t, 32>& driveId,
                                   const sirius::drive::ActionList& actions,
                                   const std::string& sandboxFolder,
                                   const std::vector<xpx_chain_sdk::Address>& replicators);
        void replicatorOnBoarding(const QString& replicatorPrivateKey, uint64_t capacityMB);
        void replicatorOffBoarding(const std::array<uint8_t, 32> &driveId, const QString& replicatorPrivateKey);
        static bool isValidHash(const std::array<uint8_t, 32>& hash);

    signals:
        void createDownloadChannelConfirmed(const std::string& channelAlias, const std::array<uint8_t, 32>& channelId, const std::array<uint8_t, 32>& rawDrivePubKey);
        void createDownloadChannelFailed(const QString& channelId, const QString& errorText);
        void closeDownloadChannelConfirmed(const std::array<uint8_t, 32>& channelId);
        void closeDownloadChannelFailed(const QString& errorText);
        void createDriveConfirmed(const std::string& driveAlias, const std::array<uint8_t, 32>& driveId);
        void createDriveFailed(const std::string& driveAlias, const std::array<uint8_t, 32>& driveKey, const QString& errorText);
        void closeDriveConfirmed(const std::array<uint8_t, 32>& driveId);
        void closeDriveFailed(const QString& errorText);
        void downloadPaymentConfirmed(const std::array<uint8_t, 32> &channelId);
        void downloadPaymentFailed(const std::array<uint8_t, 32> &channelId, const QString& errorText);
        void storagePaymentConfirmed(const std::array<uint8_t, 32> &driveId);
        void storagePaymentFailed(const std::array<uint8_t, 32> &driveId, const QString& errorText);

        void addActions(const sirius::drive::ActionList& actionList,
                        const sirius::Key& drivePublicKey,
                        const std::string& sandboxFolder,
                        std::function<void(uint64_t totalModifySize, std::array<uint8_t, 32>)> callback);

        void dataModificationApprovalConfirmed(const std::array<uint8_t, 32>& driveId, const std::string& fileStructureCdi);
        void dataModificationApprovalFailed();
        void dataModificationConfirmed(const std::array<uint8_t, 32>& modificationId);
        void dataModificationFailed(const std::array<uint8_t, 32>& modificationId);
        void cancelModificationConfirmed(const std::array<uint8_t, 32>& driveId, const QString& modificationId);
        void cancelModificationFailed(const QString& modificationId);
        void modificationCreated(const QString& driveId, const std::array<uint8_t,32>& modificationId);
        void replicatorOffBoardingConfirmed(const QString& replicatorPublicKey);
        void replicatorOffBoardingFailed(const QString& replicatorPublicKey);
        void replicatorOnBoardingConfirmed(const QString& replicatorPublicKey);
        void replicatorOnBoardingFailed(const QString& replicatorPublicKey);

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
                              const std::string& sandboxFolder,
                              const std::vector<xpx_chain_sdk::Address>& replicators);

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

    private:
        std::shared_ptr<xpx_chain_sdk::Account> mpChainAccount;
        std::shared_ptr<xpx_chain_sdk::IClient> mpChainClient;
        BlockchainEngine* mpBlockchainEngine;
        std::map<std::array<uint8_t, 32>, std::set<std::array<uint8_t, 32>>> mDataModificationApprovals;
};

#endif //TRANSACTIONSENGINE_H
