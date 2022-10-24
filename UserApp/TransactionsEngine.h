#ifndef TRANSACTIONSENGINE_H
#define TRANSACTIONSENGINE_H

#include <xpxchaincpp/sdk.h>
#include <BlockchainEngine.h>
#include <utils/HexFormatter.h>

class TransactionsEngine {
    public:
        TransactionsEngine(const std::string& clientPrivateKey, std::shared_ptr<xpx_chain_sdk::IClient> client, std::shared_ptr<BlockchainEngine> blockchainEngine);
        ~TransactionsEngine() = default;

    public:
        void addDownloadChannel(const std::vector<std::array<uint8_t, 32>>& listOfAllowedPublicKeys,
                                 const std::array<uint8_t, 32>& drivePubKey,
                                 const uint64_t& prepaidSize,
                                 const uint64_t& feedbacksNumber);

        void closeDownloadChannel(const std::array<uint8_t, 32>& channelId);

        void downloadPayment(const std::array<uint8_t, 32>& channelId, uint64_t prepaidSize);

    private:
        static QString rawHashToHex(const std::array<uint8_t, 32>& drivePubKey);

    private:
        std::shared_ptr<xpx_chain_sdk::Account> mpChainSdkAccount;
        std::shared_ptr<xpx_chain_sdk::IClient> mpChainSdkClient;
        std::shared_ptr<BlockchainEngine> mpBlockchainEngine;
};


#endif //TRANSACTIONSENGINE_H
