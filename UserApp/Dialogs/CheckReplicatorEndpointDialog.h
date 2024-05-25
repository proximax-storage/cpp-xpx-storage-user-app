#pragma once

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
    
    std::string                     m_channelKey;
    Model*                          m_model = nullptr;

public:
    
    static bool check( const sirius::drive::ReplicatorList& replicatorList );
    static bool check( Model* model, const std::string& channelKey );

private:
    CheckReplicatorEndpointDialog( const sirius::drive::ReplicatorList& replicatorList );
    CheckReplicatorEndpointDialog( Model* model, const std::string& channelKey );
    void init();
    
    void checkEndpoints();
};
