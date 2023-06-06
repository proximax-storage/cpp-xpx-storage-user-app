#include "DriveContractModel.h"
#include <thread>

void DriveContractModel::onDriveStateChanged( const std::string& driveKey, int state ) {
    std::cout << "contract state changed " << std::this_thread::get_id() << std::endl;
    if ( state == creating || state == unconfirmed || state == deleted ) {
        if ( auto it = m_drives.find( driveKey ); it != m_drives.end()) {
            m_drives.erase( it );
            emit driveContractRemoved( driveKey );
        }
    } else {
        if ( m_drives.find( driveKey ) == m_drives.end()) {
            m_drives[driveKey] = {};
            emit driveContractAdded( driveKey );
        }
    }
}

std::map <std::string, DeploymentData>& DriveContractModel::getContractDrives() {
    return m_drives;
}
