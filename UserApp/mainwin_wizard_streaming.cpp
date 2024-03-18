#include "Dialogs/AskDriveWizardDialog.h"
#include "Models/Model.h"
#include "mainwin.h"
#include "./ui_mainwin.h"
//
#include "Entities/StreamInfo.h"

void MainWin::initWizardStreaming()
{
    connect(ui->m_wizardAddStreamAnnouncementBtn, &QPushButton::released, this, [this] () {
            if(ui->m_driveCBox->count() == 0) {
                AskDriveWizardDialog dial(ui->m_wizardAddStreamAnnouncementBtn->text(),
                                          m_onChainClient, m_model, this);
            }



        // auto drive = currentStreamingDrive();
        // assert( drive );
        // std::string driveKey = currentStreamingDriveKey().toStdString();
        // if ( m_model->findDrive(driveKey) == nullptr )
        // {
        //     qWarning() << LOG_SOURCE << "bad driveKey";
        //     return;
        // }

        // AddStreamAnnouncementDialog dialog(m_onChainClient, m_model, driveKey, this);
        // auto rc = dialog.exec();
    }, Qt::QueuedConnection);

    connect(ui->m_wizardStartStreamingBtn, &QPushButton::released, this, [this] ()
        {

        }, Qt::QueuedConnection);
}

