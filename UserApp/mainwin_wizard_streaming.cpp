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
            std::string driveKey = currentStreamingDriveKey().toStdString();

            auto* drive = m_model->findDrive(driveKey);
            if ( drive == nullptr )
            {
                qWarning() << LOG_SOURCE << "bad driveKey";
                return;
            }

            if ( drive->isStreaming() )
            {
                qWarning() << LOG_SOURCE << "drive already 'is streaming'";
                return;
            }

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
            if ( StreamInfo* streamInfo = wizardSelectedStreamInfo(); streamInfo != nullptr )
            {
                //            drive->setIsStreaming();
                //            m_streamingProgressPanel->updateStreamingMode(drive);
                //            m_streamingProgressPanel->setRegistering();
                //            m_streamingProgressPanel->setVisible(true);
                startStreamingProcess( *streamInfo );
                //int y = 0;
            }
        }, Qt::QueuedConnection);

}

void MainWin::wizardUpdateStreamerTable( Drive& drive )
{
    readStreamingAnnotations( drive );
    //    ui->m_streamAnnouncementTable->clearContents();

    qDebug() << "announcements: " << m_model->streamerAnnouncements().size();

    while( ui->m_wizardStreamAnnouncementTable->rowCount() > 0 )
    {
        ui->m_wizardStreamAnnouncementTable->removeRow(0);
    }

    for( const auto& streamInfo: m_model->streamerAnnouncements() )
    {
        if ( streamInfo.m_streamingStatus == StreamInfo::ss_finished )
        {
            continue;
        }

        size_t rowIndex = ui->m_wizardStreamAnnouncementTable->rowCount();
        ui->m_wizardStreamAnnouncementTable->insertRow( (int)rowIndex );
        {
            QDateTime dateTime = QDateTime::currentDateTime();
            dateTime.setSecsSinceEpoch( streamInfo.m_secsSinceEpoch );
            ui->m_wizardStreamAnnouncementTable->setItem( (int)rowIndex, 0, new QTableWidgetItem( dateTime.toString() ) );
        }
        {
            auto* item = new QTableWidgetItem();
            //            else if ( streamInfo.m_streamingStatus == StreamInfo::ss_running )
            //            {
            //                item->setData( Qt::DisplayRole, QString::fromStdString( streamInfo.m_title + " (running...)") );
            //            }
            //            else
            //            {
            item->setData( Qt::DisplayRole, QString::fromStdString( streamInfo.m_title ) );
            //            }
            ui->m_wizardStreamAnnouncementTable->setItem( (int)rowIndex, 1, item );
        }
        {
            ui->m_wizardStreamAnnouncementTable->setItem( (int)rowIndex, 3, new QTableWidgetItem(
                                                  QString::fromStdString(
                                                      //m_model->findDrive(streamInfo.m_driveKey)->getName()
                                                      streamInfo.m_uniqueFolderName
                                                      ) ) );
        }
    }
}


StreamInfo* MainWin::wizardSelectedStreamInfo() const
{
    auto rowList = ui->m_wizardStreamAnnouncementTable->selectionModel()->selectedRows();
    if ( rowList.count() > 0 )
    {
        try
        {
            int rowIndex = rowList.constFirst().row();
            return &m_model->streamerAnnouncements().at(rowIndex);
        }
        catch (...) {}
    }
    return nullptr;
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

