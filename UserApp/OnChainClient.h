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

    signals:
        void drivesLoaded(const QStringList& drives);
        void downloadChannelsLoaded(const QStringList& channels);

    private:
        std::shared_ptr<BlockchainEngine> mpBlockchainEngine;
        std::shared_ptr<TransactionsEngine> mpTransactionsEngine;
        std::shared_ptr<xpx_chain_sdk::Account> mpChainAccount;
        std::shared_ptr<xpx_chain_sdk::IClient> mpChainClient;
};


#endif //ONCHAINCLIENT_H
