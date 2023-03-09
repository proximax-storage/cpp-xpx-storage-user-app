#ifndef BLOCKCHAINENGINE_H
#define BLOCKCHAINENGINE_H

#include <Worker.h>
#include <QObject>
#include <QThread>
#include <QUuid>

#include <memory>
#include <thread>
#include <iostream>

#include <drive/FsTree.h>
#include <drive/ActionList.h>
#include <xpxchaincpp/sdk.h>

class BlockchainEngine : public QObject
{
    Q_OBJECT

public:
    explicit BlockchainEngine(std::shared_ptr<xpx_chain_sdk::IClient> chainClient, QObject *parent = nullptr);
    ~BlockchainEngine() = default;

public:
    void init(int delay);

    void announceNewTransaction(
            const std::vector<uint8_t>& binaryData,
            const std::function<void(bool, bool, std::string, std::string)>& callback);

    void getNetworkInfo(const std::function<void(xpx_chain_sdk::NetworkInfo, bool, std::string, std::string)>& callback);

    void getAccountInfo(const std::string& accountPublicKey,
                        const std::function<void(xpx_chain_sdk::AccountInfo, bool, std::string, std::string)>& callback);

    void getBlockchainHeight(const std::function<void(uint64_t, bool, std::string, std::string)>& callback);

    void getDownloadChannelById(const std::string& channelPublicKey,
                                const std::function<void(xpx_chain_sdk::DownloadChannel, bool, std::string, std::string)>& callback);

    void getDownloadChannels(const xpx_chain_sdk::DownloadChannelsPageOptions& options,
                             const std::function<void(xpx_chain_sdk::download_channels_page::DownloadChannelsPage, bool, std::string, std::string)>& callback);

    void getBlockByHeight(
            uint64_t height,
            const std::function<void(xpx_chain_sdk::Block, bool, std::string, std::string)>& callback);

    void getDriveById(const std::string& drivePublicKey,
                      const std::function<void(xpx_chain_sdk::Drive, bool, std::string, std::string)>& callback);

    void getDrives(const xpx_chain_sdk::DrivesPageOptions& options,
                   const std::function<void(xpx_chain_sdk::drives_page::DrivesPage, bool, std::string, std::string)>& callback);

    void getTransactionInfo(xpx_chain_sdk::TransactionGroup group,
                            const std::string &id,
                            const std::function<void(std::shared_ptr<xpx_chain_sdk::transactions_info::BasicTransaction>, bool, std::string, std::string)>& callback);

    void getReplicatorById(const std::string& replicatorPublicKey,
                           const std::function<void(xpx_chain_sdk::Replicator, bool, std::string, std::string)>& callback);

    void getMosaicsNames(const std::vector<xpx_chain_sdk::MosaicId>& ids,
                         const std::function<void(xpx_chain_sdk::MosaicNames, bool, std::string, std::string)>& callback);

signals:
    void addResolver(const QUuid& id, const std::function<void(QVariant)>& resolver);
    void removeResolver(const QUuid& id);
    void runProcess(const QUuid& id, const std::function<QVariant()>& task);

private:
    void callbackResolver(const QUuid& id, const QVariant& data);

    template<class Type>
    void typeResolver(const QVariant& data, const std::function<void(Type, bool, std::string, std::string)>& callback) {
        if (data.canConvert<Type>()) {
            callback(data.value<Type>(), true, "", "");
        } else if (data.canConvert<xpx_chain_sdk::ErrorMessage>()) {
            auto message = data.value<xpx_chain_sdk::ErrorMessage>();
            callback(Type(), false, message.message, message.code);
        } else if (data.canConvert<std::string>()) {
            callback(Type(), false, data.value<std::string>(), "");
        } else {
            callback(Type(), false, "unknown error", "");
        }
    }

private slots:
    void onAddResolver(const QUuid& id, const std::function<void(QVariant)>& resolver);
    void onRemoveResolver(const QUuid& id);

private:
    Worker* mpWorker;
    QThread* mpThread;
    std::map<QUuid, std::function<void(QVariant)>> mResolvers;
    std::shared_ptr<xpx_chain_sdk::IClient> mpChainClient;
};

typedef std::array<uint8_t,32> stdArrayUint8_t32;

Q_DECLARE_METATYPE(QUuid)
Q_DECLARE_METATYPE(std::function<QVariant()>)
Q_DECLARE_METATYPE(std::function<void(QVariant)>)
Q_DECLARE_METATYPE(std::function<void(uint64_t, stdArrayUint8_t32)>)
Q_DECLARE_METATYPE(stdArrayUint8_t32)
Q_DECLARE_METATYPE(xpx_chain_sdk::ErrorMessage)
Q_DECLARE_METATYPE(std::string)
Q_DECLARE_METATYPE(xpx_chain_sdk::Block)
Q_DECLARE_METATYPE(xpx_chain_sdk::Drive)
Q_DECLARE_METATYPE(sirius::drive::FsTree)
Q_DECLARE_METATYPE(sirius::drive::ActionList)
Q_DECLARE_METATYPE(xpx_chain_sdk::DownloadChannel)
Q_DECLARE_METATYPE(std::vector<xpx_chain_sdk::DownloadChannel>)
Q_DECLARE_METATYPE(xpx_chain_sdk::drives_page::DrivesPage)
Q_DECLARE_METATYPE(xpx_chain_sdk::download_channels_page::DownloadChannelsPage)
Q_DECLARE_METATYPE(xpx_chain_sdk::Replicator)
Q_DECLARE_METATYPE(xpx_chain_sdk::replicators_page::ReplicatorsPage)
Q_DECLARE_METATYPE(xpx_chain_sdk::NetworkInfo)
Q_DECLARE_METATYPE(xpx_chain_sdk::AccountInfo)
Q_DECLARE_METATYPE(std::shared_ptr<xpx_chain_sdk::transactions_info::BasicTransaction>)
Q_DECLARE_METATYPE(sirius::drive::Action)
Q_DECLARE_METATYPE(sirius::Key)

#endif // BLOCKCHAINENGINE_H
