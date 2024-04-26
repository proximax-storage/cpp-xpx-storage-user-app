#pragma once

#include <QDialog>
#include <QString>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "types.h"

class CheckReplicatorEndpointDialog : public QDialog
{
    sirius::drive::ReplicatorList           m_replicatorList;
    
public:
    
    static bool check( const sirius::drive::ReplicatorList& replicatorList );
    
private:
    CheckReplicatorEndpointDialog( const sirius::drive::ReplicatorList& replicatorList );
    
    void checkEndpoints();
};
