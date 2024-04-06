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
            if ( ui->m_driveCBox->count() == 0 )
            {
                AskDriveWizardDialog dial(ui->m_wizardAddStreamAnnouncementBtn->text(),
                                          m_onChainClient, m_model, this);
                auto rc = dial.exec();
                if (rc == QMessageBox::Ok)
                {
                    AddDriveDialog dialog(m_onChainClient,
                                          m_model,
                                          this,
                                          [this](std::string driveHash)
                    {
                        m_wizardAddStreamAnnounceDialog
                          = new WizardAddStreamAnnounceDialog(m_onChainClient,
                                                              m_model,
                                                              driveHash,
                                                              this);
                        m_wizardAddStreamAnnounceDialog->exec();
                        delete m_wizardAddStreamAnnounceDialog;
                        m_wizardAddStreamAnnounceDialog = nullptr;
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

    // Start streaming w/o annoucement
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
                        auto m_wizardAddStreamAnnounceDialog
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

    //
    connect(ui->m_wizardStartSelectedStreamBtn, &QPushButton::released, this, [this] ()
        {
            std::string driveKey = selectedDriveKeyInWizardTable().toStdString();

            auto rowList = ui->m_wizardStreamAnnouncementTable->selectionModel()->selectedRows();
            if ( rowList.count() == 0 )
            {
                if ( m_model->getStreams().size() == 0 )
                {
                    displayError( "Add stream announcement" );
                    return;
                }
            }

            auto* drive = m_model->findDrive(driveKey);
            if ( drive == nullptr )
            {
                qWarning() << LOG_SOURCE << "bad driveKey: " << driveKey.c_str();
                displayError( "Cannot found drive: " );
                return;
            }

            if ( drive->isStreaming() )
            {
                qWarning() << LOG_SOURCE << "drive already 'is streaming'";
                displayError( "Current stream is not finished" );
                return;
            }

            if ( StreamInfo* streamInfo = wizardSelectedStreamInfo(); streamInfo != nullptr )
            {
                startStreamingProcess( *streamInfo );
            }
            else
            {
                qWarning() << LOG_SOURCE << "Internal error: no streaInfo";
                displayError( "Internal error: no streaInfo" );
                return;
            }
        }, Qt::QueuedConnection);
}

QString MainWin::selectedDriveKeyInWizardTable() const
{
    auto rowList = ui->m_wizardStreamAnnouncementTable->selectionModel()->selectedRows();
    if ( rowList.count() > 0 )
    {
        return ui->m_wizardStreamAnnouncementTable->item( rowList.first().row(), 2 )->data(Qt::UserRole).toString();
    }
    return "";
}

Drive* MainWin::selectedDriveInWizardTable() const
{
    auto driveKey = selectedDriveKeyInWizardTable().toStdString();
    Drive* drive = m_model->findDrive( driveKey );
    if ( drive == nullptr && (ui->m_streamDriveCBox->count() == 0) )
    {
        qWarning() << LOG_SOURCE << "! internal error: bad drive key";
    }
    return drive;
}

void MainWin::wizardUpdateStreamerTable( const Drive& drive )
{
    readStreamingAnnotations( drive );
    //    ui->m_streamAnnouncementTable->clearContents();

    qDebug() << "announcements: " << m_model->getStreams().size();

    while( ui->m_wizardStreamAnnouncementTable->rowCount() > 0 )
    {
        ui->m_wizardStreamAnnouncementTable->removeRow(0);
    }

    for( const auto& streamInfo: m_model->getStreams() )
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
            item->setData( Qt::DisplayRole, QString::fromStdString( streamInfo.m_title ) );
            ui->m_wizardStreamAnnouncementTable->setItem( (int)rowIndex, 1, item );
        }
        {
            auto* item = new QTableWidgetItem( QString::fromStdString( m_model->findDrive(streamInfo.m_driveKey)->getName() ));
            item->setData( Qt::UserRole, QString::fromStdString( streamInfo.m_driveKey ) );
            ui->m_wizardStreamAnnouncementTable->setItem( (int)rowIndex, 2, item );
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
            return &m_model->getStreams().at(rowIndex);
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

