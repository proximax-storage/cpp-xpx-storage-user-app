#pragma once

#include "DriveStateSubscriber.h"
#include <QObject>
#include <QRegularExpression>


struct ServicePayment {
    std::string m_mosaicId;
    std::string m_amount;
};

struct DeploymentData {
    std::string m_assignee;
    std::string m_file;
    std::string m_function;
    std::string m_parameters;
    int m_executionCallPayment = 0;
    int m_downloadCallPayment = 0;
    std::string m_automaticExecutionFileName;
    std::string m_automaticExecutionFunctionName;
    int m_automaticExecutionCallPayment = 0;
    int m_automaticDownloadCallPayment = 0;
    int m_automaticExecutionsNumber = 0;
    std::vector <ServicePayment> m_servicePayments;

    bool isValid() {
        uint maxRowSize = 4096;
        QRegularExpression keyTemplate( QRegularExpression::anchoredPattern( QLatin1String( R"([a-zA-Z0-9]{64})" )));
        if ( !keyTemplate.match( QString::fromStdString( m_assignee )).hasMatch()) {
            return false;
        }
        if ( m_file.empty() || m_file.size() > maxRowSize ) {
            return false;
        }
        if ( m_function.empty() || m_function.size() > maxRowSize ) {
            return false;
        }
        if ( m_parameters.size() > maxRowSize ) {
            return false;
        }
        if ( m_automaticExecutionFileName.size() > maxRowSize ) {
            return false;
        }
        if ( m_automaticExecutionFunctionName.size() > maxRowSize ) {
            return false;
        }

        for ( const auto&[mosaicId, amount]: m_servicePayments ) {
            if ( mosaicId.empty()) {
                return false;
            }

            bool mosaicIdIsNumber = false;
            auto mosaicIdNumber = QString::fromStdString( mosaicId ).toULongLong( &mosaicIdIsNumber, 10 );

            if ( !mosaicIdIsNumber ) {
                return false;
            }

            if ( amount.empty()) {
                return false;
            }

            bool amountIsNumber = false;
            auto amountNumber = QString::fromStdString( amount ).toULongLong( &amountIsNumber, 10 );

            // Do not allow transfer 0
            if ( !amountIsNumber || amountNumber == 0 ) {
                return false;
            }
        }

        return true;
    }
};

class DriveContractModel
        : public QObject {

    Q_OBJECT

private:

    std::map <std::string, DeploymentData> m_drives;

    signals:

            void driveContractAdded(
    const std::string& driveKey
    );

    void driveContractRemoved( const std::string& driveKey );

public:

    void onDriveStateChanged( const std::string& driveKey, int state );

    std::map <std::string, DeploymentData>& getContractDrives();

};
