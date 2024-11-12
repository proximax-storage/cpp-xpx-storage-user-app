#ifndef WA_APP

#include <QDialog>
#include <QString>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QHBoxLayout>
#include <functional>

#include "CheckReplicatorEndpointDialog.h"
#include "Entities/DownloadChannel.h"
#include "Engines/StorageEngine.h"
#include "Models/Model.h"

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

bool CheckReplicatorEndpointDialog::check( Model* model, const QString& channelKey )
{
    CheckReplicatorEndpointDialog modalDialog( model, channelKey );
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
    init();
}

CheckReplicatorEndpointDialog::CheckReplicatorEndpointDialog( Model* model, const QString& channelKey )
    :   QDialog(),
        m_channelKey(channelKey),
        m_model(model)
{
    init();
}

void CheckReplicatorEndpointDialog::init()
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
    sirius::drive::ReplicatorList list;
    
    if ( m_channelKey.isEmpty() )
    {
        list = m_replicatorList;
    }
    else
    {
        DownloadChannel* channel = m_model->findChannel( m_channelKey );
        if ( channel == nullptr )
        {
            QDialog::reject();
            return;
        }
        list = channel->getReplicators();
        qDebug() << "DownloadChannel: " << channel << " " << list.size();
    }
    
    int counter = 0;
    for( auto& replicatorKey : list )
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
        QTimer::singleShot( 100, this, [this]
        {
            checkEndpoints();
        });
    }
    else
    {
        QDialog::accept();
    }
}

#endif // #ifndef WA_APP
