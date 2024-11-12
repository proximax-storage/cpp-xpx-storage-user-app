#pragma once

#ifndef WA_APP

#include <QDialog>
#include <QString>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "types.h"

class DownloadChannel;
class Model;

class CheckReplicatorEndpointDialog : public QDialog
{
    sirius::drive::ReplicatorList   m_replicatorList;
    
    QString                         m_channelKey;
    Model*                          m_model = nullptr;

public:
    
    static bool check( const sirius::drive::ReplicatorList& replicatorList );
    static bool check( Model* model, const QString& channelKey );

private:
    CheckReplicatorEndpointDialog( const sirius::drive::ReplicatorList& replicatorList );
    CheckReplicatorEndpointDialog( Model* model, const QString& channelKey );
    void init();
    
    void checkEndpoints();
};

#endif // #ifndef WA_APP
