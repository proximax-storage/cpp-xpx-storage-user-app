#pragma once

#include "ServicePayment.h"
#include <vector>

struct ContractManualCallData {
    QString m_contractKey;
    QString m_file;
    QString m_function;
    QString m_parameters;
    int m_executionCallPayment = 0;
    int m_downloadCallPayment = 0;
    std::vector <ServicePayment> m_servicePayments;

    bool isValid() {
        const uint maxRowSize = 4096;

        QRegularExpression keyTemplate( QRegularExpression::anchoredPattern( QLatin1String( R"([a-zA-Z0-9]{64})" )));
        if ( !keyTemplate.match( m_contractKey ).hasMatch()) {
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