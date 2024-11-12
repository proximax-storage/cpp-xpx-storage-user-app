#pragma once

#include <QRegularExpression>
#include "ServicePayment.h"

struct ContractDeploymentData {
    QString m_assignee;
    QString m_file;
    QString m_function;
    QString m_parameters;
    int m_executionCallPayment = 0;
    int m_downloadCallPayment = 0;
    QString m_automaticExecutionFileName;
    QString m_automaticExecutionFunctionName;
    int m_automaticExecutionCallPayment = 0;
    int m_automaticDownloadCallPayment = 0;
    int m_automaticExecutionsNumber = 0;
    std::vector <ServicePayment> m_servicePayments;

    bool m_deploymentAllowed = true;

    bool isValid() {
        const uint maxRowSize = 4096;

        if (!m_deploymentAllowed) {
            return false;
        }

        QRegularExpression keyTemplate( QRegularExpression::anchoredPattern( QLatin1String( R"([a-zA-Z0-9]{64})" )));
        if ( !keyTemplate.match( m_assignee ).hasMatch()) {
            return false;
        }
        if ( m_file.isEmpty() || m_file.size() > maxRowSize ) {
            return false;
        }
        if ( m_function.isEmpty() || m_function.size() > maxRowSize ) {
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