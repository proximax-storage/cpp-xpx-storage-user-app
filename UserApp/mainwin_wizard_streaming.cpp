#include "Dialogs/AddDriveDialog.h"
#include "Dialogs/AskDriveWizardDialog.h"
#include "Dialogs/WizardAddStreamAnnounceDialog.h"
#include "Models/Model.h"
#include "mainwin.h"
#include "./ui_mainwin.h"
//
#include "Entities/StreamInfo.h"

void MainWin::initWizardStreaming()
{
    ui->m_wizardStreamAnnouncementTable->hide();
    ui->m_wizardStartSelectedStreamBtn->hide();

    connect(ui->m_wizardStreamAnnouncementTable->model()
            , &QAbstractItemModel::rowsRemoved, this
            , &MainWin::onRowsRemoved);

    connect(ui->m_wizardStreamAnnouncementTable->model()
            , &QAbstractItemModel::rowsInserted, this
            , &MainWin::onRowsInserted);

    
    connect(ui->m_wizardAddStreamAnnouncementBtn, &QPushButton::released, this, [this] ()
        {
            if(ui->m_driveCBox->count() == 0) {
                AskDriveWizardDialog dial(ui->m_wizardAddStreamAnnouncementBtn->text(),
                                          m_onChainClient, m_model, this);
                auto rc = dial.exec();
                if (rc == QMessageBox::Ok)
                {
                    AddDriveDialog dialog(m_onChainClient,
                                          m_model,
                                          this,
                                          [this](std::string hash)
                    {
                        m_wizardAddStreamAnnounceDialog
                          = new WizardAddStreamAnnounceDialog(m_onChainClient,
                                                              m_model,
                                                              hash,
                                                              this);
                        m_wizardAddStreamAnnounceDialog->exec();
                        // delete m_wizardAddStreamAnnounceDialog;
                        // m_wizardAddStreamAnnounceDialog = nullptr;
                    });
                    dialog.exec();
                }
            }
            else
            {
                m_wizardAddStreamAnnounceDialog
                    = new WizardAddStreamAnnounceDialog(m_onChainClient,
                                                        m_model,
                                                        m_model->getDrives().begin()->first,
                                                        this);
                m_wizardAddStreamAnnounceDialog->exec();
                delete m_wizardAddStreamAnnounceDialog;
                m_wizardAddStreamAnnounceDialog = nullptr;
            }
        }, Qt::QueuedConnection);

    connect(ui->m_wizardStartStreamingBtn, &QPushButton::released, this, [this] ()
        {
            if(ui->m_driveCBox->count() == 0) {
                AskDriveWizardDialog dial(ui->m_wizardStartStreamingBtn->text(),
                                          m_onChainClient, m_model, this);
                auto rc = dial.exec();
                if (rc == QMessageBox::Ok)
                {
                    AddDriveDialog dialog(m_onChainClient,
                                          m_model,
                                          this,
                                          [this](std::string hash)
                    {
                        m_wizardAddStreamAnnounceDialog
                          = new WizardAddStreamAnnounceDialog(m_onChainClient,
                                                              m_model,
                                                              hash,
                                                              this);
                        m_wizardAddStreamAnnounceDialog->exec();
                        delete m_wizardAddStreamAnnounceDialog;
                        m_wizardAddStreamAnnounceDialog = nullptr;
                    });
                    dialog.exec();
                }
            }
        }, Qt::QueuedConnection);

    connect(ui->m_wizardStartSelectedStreamBtn, &QPushButton::released, this, [this] ()
        {
            auto rowList = ui->m_wizardStreamAnnouncementTable->selectionModel()->selectedRows();
            if ( rowList.count() == 0 )
            {
                if ( m_model->streamerAnnouncements().size() == 0 )
                {
                    QMessageBox msgBox;
                    msgBox.setText( QString::fromStdString( "Add stream announcement" ) );
                    msgBox.setInformativeText( "Press button '+'" );
                    msgBox.setStandardButtons(QMessageBox::Ok);
                    msgBox.exec();
                    return;
                }
            }
            if ( StreamInfo* streamInfo = selectedStreamInfo(); streamInfo != nullptr )
            {
                startStreamingProcess( *streamInfo );
            }
        }, Qt::QueuedConnection);
}

void MainWin::onRowsRemoved()
{
    if (ui->m_wizardStreamAnnouncementTable->rowCount() == 0)
    {
        ui->m_wizardStreamAnnouncementTable->hide();
        ui->m_wizardStartSelectedStreamBtn->hide();
    }
    else
    {
        ui->m_wizardStreamAnnouncementTable->show();
        ui->m_wizardStartStreamingBtn->show();
    }
}

void MainWin::onRowsInserted()
{
    ui->m_wizardStreamAnnouncementTable->show();
    ui->m_wizardStartSelectedStreamBtn->show();
}

