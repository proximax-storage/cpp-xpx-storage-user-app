#pragma once

#include <QObject>
#include "ContractDeploymentData.h"
#include "ContractManualCallData.h"

class DriveContractModel : public QObject {

    Q_OBJECT

private:

    std::map <QString, ContractDeploymentData> m_drives;
    ContractManualCallData m_contractManualCallData;

signals:

    void driveContractAdded( const QString& driveKey );

    void driveContractRemoved( const QString& driveKey );

public:

    void onDriveStateChanged( const QString& driveKey, int state );

    std::map <QString, ContractDeploymentData>& getContractDrives();

    ContractManualCallData& getContractManualCallData();

};
