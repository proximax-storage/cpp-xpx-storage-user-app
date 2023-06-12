#pragma once

#include <QObject>
#include "ContractDeploymentData.h"

class DriveContractModel
        : public QObject {

    Q_OBJECT

private:

    std::map <std::string, ContractDeploymentData> m_drives;

    signals:

    void driveContractAdded( const std::string& driveKey );

    void driveContractRemoved( const std::string& driveKey );

public:

    void onDriveStateChanged( const std::string& driveKey, int state );

    std::map <std::string, ContractDeploymentData>& getContractDrives();

};
