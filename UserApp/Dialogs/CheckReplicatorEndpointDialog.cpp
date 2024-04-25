#pragma once

#include <QDialog>
#include <QString>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <optional>
#include <functional>

#include "CheckReplicatorEndpointDialog.h"
#include "Engines/StorageEngine.h"
#include "drive/ViewerSession.h"

bool CheckReplicatorEndpointDialog::check( const sirius::drive::ReplicatorList& replicatorList )
{
    CheckReplicatorEndpointDialog modalDialog( replicatorList );
    modalDialog.show();
    modalDialog.hide();
    if ( modalDialog.exec() == QDialog::Accepted )
    {
        return true;
    }
    return false;
}
    
CheckReplicatorEndpointDialog::CheckReplicatorEndpointDialog( const sirius::drive::ReplicatorList&     replicatorList )
    :   QDialog(),
        m_replicatorList(replicatorList)
{
    setWindowTitle( "Waiting replicator endpoints" );

    QLabel* label  = new QLabel( "Please wait...", this );
    auto*   layout = new QVBoxLayout( this );
    layout->addWidget( label );
    
    QPushButton* cancelButton = new QPushButton("Cancel");
    layout->addWidget( cancelButton );
    
    connect( cancelButton, &QPushButton::released, this, &QDialog::reject );
    
    resize(320, 200);
    
    QTimer::singleShot( 0, this, [this]
    {
        checkEndpoints();
    });
}
    
void CheckReplicatorEndpointDialog::checkEndpoints()
{
    int counter = 0;
    for( auto& replicatorKey : m_replicatorList )
    {
        auto endpoint = gStorageEngine->getViewerSession()->session()->getEndpoint( replicatorKey );
        if ( endpoint )
        {
            counter++;
        }
    }
    qDebug() << "checkEndpoints: " << counter;

    if ( counter < (2*m_replicatorList.size())/3+1 )
    {
        show();
        QTimer::singleShot( 1, this, [this]
        {
            checkEndpoints();
        });
    }
    else
    {
        QDialog::accept();
    }
}
