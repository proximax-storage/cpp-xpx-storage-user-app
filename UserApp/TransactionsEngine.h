#ifndef TRANSACTIONSENGINE_H
#define TRANSACTIONSENGINE_H

#include <xpxchaincpp/sdk.h>
#include <BlockchainEngine.h>
#include <utils/HexFormatter.h>

class TransactionsEngine {
    public:
        TransactionsEngine(std::shared_ptr<xpx_chain_sdk::IClient> client, std::shared_ptr<xpx_chain_sdk::Account> account, std::shared_ptr<BlockchainEngine> blockchainEngine);
        ~TransactionsEngine() = default;

    public:
        void addDownloadChannel(const std::vector<std::array<uint8_t, 32>>& listOfAllowedPublicKeys,
                                 const std::array<uint8_t, 32>& drivePubKey,
                                 const uint64_t& prepaidSize,
                                 const uint64_t& feedbacksNumber);

        void closeDownloadChannel(const std::array<uint8_t, 32>& channelId);

        void downloadPayment(const std::array<uint8_t, 32>& channelId, uint64_t prepaidSize);

    public:
        static QString rawHashToHex(const std::array<uint8_t, 32>& rawHash);
        static std::array<uint8_t, 32> rawHashFromHex(const QString& hex);

    private:
        std::shared_ptr<xpx_chain_sdk::Account> mpChainAccount;
        std::shared_ptr<xpx_chain_sdk::IClient> mpChainClient;
        std::shared_ptr<BlockchainEngine> mpBlockchainEngine;
};


#endif //TRANSACTIONSENGINE_H
