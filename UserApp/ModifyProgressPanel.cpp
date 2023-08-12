#include "ModifyProgressPanel.h"
#include "Model.h"
#include "Utils.h"
#include "mainwin.h"
#include "ui_ModifyProgressPanel.h"

#include <QPushButton>

ModifyProgressPanel::ModifyProgressPanel( Model* model, int x, int y, QWidget* parent, const std::function<void()>& cancelModificationFunc, Mode mode )
    : QFrame(parent)
    , ui(new Ui::Frame)
    , mp_model(model)
    , m_cancelModificationFunc( cancelModificationFunc )
    , m_mode(mode)
{
    ui->setupUi(this);

    m_commonSize = QSize(20, 20);
    m_loading = new QMovie(getResource("./resources/icons/loader.gif"));
    m_loading->setScaledSize(m_commonSize);
    m_loading->setParent(this);

    m_loaded = new QPixmap(getResource("./resources/icons/loaded.png"));
    m_error = new QPixmap(getResource("./resources/icons/error.png"));

    ui->m_requestedSize->setText("0");
    ui->m_unitsType->setText("Bytes");
    if ( mode == drive_modification )
    {
        ui->m_title->setWindowTitle("Modification status");
    }
    else
    {
        ui->m_title->setWindowTitle("Streaming status");
    }
    ui->m_title->setAlignment(Qt::AlignCenter);
    setGeometry( QRect( x, y, 255, 130) );
    setFrameShape( QFrame::StyledPanel );
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Midlight);
    setFrameStyle( (int)QFrame::StyledPanel | (int)QFrame::Raised );
    ui->m_cancel->setText("Cancel modification");

    connect( ui->m_cancel, &QPushButton::released, this, [this]
    {
        if ( m_mode == drive_modification )
        {
            auto drive = mp_model->currentDrive();
            if (drive) {
                if (ui->m_cancel->text() == "Cancel") {
                    m_cancelModificationFunc();
                } else {
                    setVisible(false);
                    drive->updateDriveState(no_modifications);
                }
            } else {
                qWarning () << "ModifyProgressPanel::cancel: invalid drive";
            }
        }
        else
        {
            m_cancelModificationFunc();
        }
    });

    stackUnder( this );

    ui->horizontalLayout->setAlignment(Qt::AlignCenter);
    ui->m_statusIcon->setText("");
}

ModifyProgressPanel::~ModifyProgressPanel()
{
}

void ModifyProgressPanel::setRegistering()
{
    ui->m_requestedSize->setText("0");
    if ( m_mode == drive_modification )
    {
        ui->m_statusLabel->setText( "Modification is registering ");
    }
    else
    {
        ui->m_statusLabel->setText( "Stream is registering ");
    }
    ui->m_statusIcon->setScaledContents(false);
    ui->m_cancel->setText("Cancel");
    ui->m_cancel->setEnabled(m_mode == streaming ); //todo always 'false'
    ui->m_statusIcon->setMovie(m_loading);
    m_loading->start();
    adjustSize();
}

void ModifyProgressPanel::setUploading()
{
    if ( m_mode == drive_modification )
    {
        ui->m_statusLabel->setText( "Modification is uploading ");
    }
    else
    {
        ui->m_statusLabel->setText( "Is streaming ...");
        ui->m_cancel->setText("Finish streaming");
    }
    ui->m_cancel->setEnabled(true);
    adjustSize();
}

void ModifyProgressPanel::setApproving()
{
    if ( m_mode == drive_modification )
    {
        ui->m_statusLabel->setText( "Modification is completing ");
    }
    else
    {
        ui->m_statusLabel->setText( "Stream is ending...");
    }
    ui->m_cancel->setEnabled(false);
    adjustSize();
}

void ModifyProgressPanel::setApproved()
{
    if ( m_mode == drive_modification )
    {
        ui->m_statusLabel->setText("Modification is completed!");
    }
    else
    {
        ui->m_statusLabel->setText("Stream is completed!");
    }
    ui->m_cancel->setText("Ok");
    ui->m_statusIcon->clear();
    ui->m_statusIcon->setScaledContents(true);
    ui->m_statusIcon->setPixmap(*m_loaded);
    ui->m_cancel->setEnabled(true);
    adjustSize();
}

void ModifyProgressPanel::setFailed()
{
    if ( m_mode == drive_modification )
    {
        ui->m_statusLabel->setText( "Modification is failed!");
    }
    else
    {
        ui->m_statusLabel->setText( "Stream is failed!");
    }
    ui->m_cancel->setText("Close");
    ui->m_statusIcon->clear();
    ui->m_statusIcon->setScaledContents(true);
    ui->m_statusIcon->setPixmap(*m_error);
    ui->m_cancel->setEnabled(true);
    adjustSize();
}

void ModifyProgressPanel::setCanceling()
{
    if ( m_mode == drive_modification )
    {
        ui->m_statusLabel->setText( "Modification is canceling ");
    }
    else
    {
        ui->m_statusLabel->setText( "Stream is canceling...");
    }
    ui->m_cancel->setEnabled(false);
    ui->m_cancel->setText("Ok");
    adjustSize();
}

void ModifyProgressPanel::setCanceled()
{
    if ( m_mode == drive_modification )
    {
        ui->m_statusLabel->setText("Modification is canceled!");
    }
    else
    {
        ui->m_statusLabel->setText("Stream is canceled!");
    }
    ui->m_cancel->setText("Ok");
    ui->m_statusIcon->clear();
    ui->m_statusIcon->setScaledContents(true);
    ui->m_statusIcon->setPixmap(*m_loaded);
    ui->m_cancel->setEnabled(true);
    adjustSize();
}

void ModifyProgressPanel::setWaitingChannelCreation()
{
    ui->m_statusLabel->setText( "Waiting channel creation");
    ui->m_statusIcon->setScaledContents(false);
    ui->m_cancel->setText("Cancel");
    ui->m_cancel->setEnabled(true);
    ui->m_statusIcon->setMovie(m_loading);
    m_loading->start();
    adjustSize();
}

void ModifyProgressPanel::setWaitingStreamStart()
{
    ui->m_statusLabel->setText( "Waiting stream start");
    ui->m_statusIcon->setScaledContents(false);
    ui->m_cancel->setText("Cancel");
    ui->m_cancel->setEnabled(true);
    ui->m_statusIcon->setMovie(m_loading);
    m_loading->start();
    adjustSize();
}

void ModifyProgressPanel::updateUploadedDataAmount(const uint64_t amount)
{
    uint64_t currentAmount = ui->m_requestedSize->text().toULongLong();
    ui->m_requestedSize->setText(QString::number(amount + currentAmount));
}
