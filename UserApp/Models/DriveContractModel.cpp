#include "DriveContractModel.h"
#include "Entities/Drive.h"

void DriveContractModel::onDriveStateChanged( const QString& driveKey, int state ) {
    if ( state == creating || state == unconfirmed || state == deleted || state == contract_deployed ) {
        if ( auto it = m_drives.find( driveKey ); it != m_drives.end()) {
            m_drives.erase( it );
            emit driveContractRemoved( driveKey );
        }
    }
    else {
        auto [driveIt, inserted] = m_drives.try_emplace(driveKey);
        if ( inserted ) {
            emit driveContractAdded( driveKey );
        }
        driveIt->second.m_deploymentAllowed = state != contract_deploying;
    }
}

std::map <QString, ContractDeploymentData>& DriveContractModel::getContractDrives() {
    return m_drives;
}

ContractManualCallData& DriveContractModel::getContractManualCallData() {
    return m_contractManualCallData;
}
