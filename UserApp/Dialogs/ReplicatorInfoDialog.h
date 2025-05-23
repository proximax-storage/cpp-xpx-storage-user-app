#ifndef REPLICATORINFODIALOG_H
#define REPLICATORINFODIALOG_H

#include <QDialog>
#include "Models/Model.h"
#include "xpxchaincpp/model/storage/replicator.h"

namespace Ui {
class ReplicatorInfoDialog;
}

class ReplicatorInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplicatorInfoDialog(const QString& replicator,
                                  Model* model,
                                  QWidget *parent = nullptr);
    ~ReplicatorInfoDialog();

public:
    void accept() override;
    void reject() override;
    void updateFreeSpace(uint64_t freeSpace);
    void updateTotalSpace(uint64_t index, uint64_t totalSpace);
    void updateDrivesAmount(uint64_t freeSpace);
    void updateChannelsAmount(uint64_t usedSpace);

private:
    void validate();

private:
    Ui::ReplicatorInfoDialog* ui;
    Model* mpModel;
    QString mPublicKey;
};

#endif // REPLICATORINFODIALOG_H
