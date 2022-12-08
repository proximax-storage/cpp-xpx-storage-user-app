#include "ModifyProgressPanel.h"
#include "Model.h"
#include "ui_ModifyProgressPanel.h"

#include <QPushButton>

ModifyProgressPanel::ModifyProgressPanel( int x, int y, QWidget* parent, const std::function<void()>& cancelModificationFunc )
    : QFrame(parent)
    , ui(new Ui::Frame)
    , m_cancelModificationFunc( cancelModificationFunc )
{
    ui->setupUi(this);

    m_commonSize = QSize(20, 20);
    m_loading = new QMovie(getResource("./resources/icons/loader.gif"));
    m_loading->setScaledSize(m_commonSize);
    m_loading->setParent(this);

    m_loaded = new QPixmap(getResource("./resources/icons/loaded.png"));
    m_error = new QPixmap(getResource("./resources/icons/error.png"));

    ui->m_title->setWindowTitle("Modification status");
    ui->m_title->setAlignment(Qt::AlignCenter);
    setGeometry( QRect( x, y, 255, 130) );
    setFrameShape( QFrame::StyledPanel );
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Midlight);
    setFrameStyle( (int)QFrame::StyledPanel | (int)QFrame::Raised );
    ui->m_cancel->setText("Cancel modification");

    connect( ui->m_cancel, &QPushButton::released, this, [this]
    {
        qDebug() << "ui->m_cancel, ::released:";

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
        if ( auto* driveInfo = Model::currentDriveInfoPtr(); driveInfo == nullptr )
        {
            setVisible(false);
        }
        else
        {
            qDebug() << "ui->m_cancel, ::released: status: " << driveInfo->m_modificationStatus;

            if (driveInfo->m_modificationStatus == is_registering ||
                driveInfo->m_modificationStatus == is_registered )
            {
                qDebug() << "ui->m_cancel, ::released: 1";
                driveInfo->m_modificationStatus = is_canceling;
                m_cancelModificationFunc();
            }
            else if ( driveInfo->m_modificationStatus == no_modification ||
                      driveInfo->m_modificationStatus == is_approved ||
                      driveInfo->m_modificationStatus == is_approvedWithOldRootHash ||
                      driveInfo->m_modificationStatus == is_failed ||
                      driveInfo->m_modificationStatus == is_canceled )
            {
                qDebug() << "ui->m_cancel, ::released: 2";

                driveInfo->m_currentModificationHash.reset();
                driveInfo->m_modificationStatus = no_modification;
                setVisible(false);
            }
        }

        lock.unlock();
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
    ui->m_statusLabel->setText( "Modification is registering ");
    ui->m_statusIcon->setScaledContents(false);
    ui->m_cancel->setText("Cancel");
    ui->m_cancel->setEnabled(true);
    ui->m_statusIcon->setMovie(m_loading);
    m_loading->start();
    adjustSize();
}

void ModifyProgressPanel::setRegistered()
{
    ui->m_statusLabel->setText( "Modification is uploading ");
    ui->m_cancel->setText("Cancel");
    ui->m_cancel->setEnabled(true);
    adjustSize();
}

void ModifyProgressPanel::setApproved()
{
    ui->m_statusLabel->setText("Modification is completed!");
    ui->m_cancel->setText("Ok");
    ui->m_statusIcon->clear();
    ui->m_statusIcon->setScaledContents(true);
    ui->m_statusIcon->setPixmap(*m_loaded);
    ui->m_cancel->setEnabled(true);
    adjustSize();
}

void ModifyProgressPanel::setFailed()
{
    ui->m_statusLabel->setText( "Modification is failed!");
    ui->m_cancel->setText("Close");
    ui->m_statusIcon->clear();
    ui->m_statusIcon->setScaledContents(true);
    ui->m_statusIcon->setPixmap(*m_error);
    ui->m_cancel->setEnabled(true);
    adjustSize();
}

void ModifyProgressPanel::setIsApprovedWithOldRootHash()
{
//    ui->m_registeredLabel->setText( "modification is registred (in blockchain)");
//    ui->m_registeredLabel->setStyleSheet("color: blue");
//    ui->m_appliedLabel->setText( "Modification is completed");
//    ui->m_appliedLabel->setStyleSheet("color: blue");
//    ui->m_appliedLabel->setText( "root hash is not changed");
//    ui->m_appliedLabel->setStyleSheet("color: red");
//    ui->m_cancel->setText("Close");
//    adjustSize();
//
//    ui->m_cancel->setEnabled(true);
}

void ModifyProgressPanel::setCanceling()
{
    ui->m_statusLabel->setText( "Modification is canceling ");
    ui->m_cancel->setEnabled(false);
    ui->m_cancel->setText("Ok");
    adjustSize();
}

void ModifyProgressPanel::setCanceled()
{
    ui->m_statusLabel->setText("Modification is canceled!");
    ui->m_cancel->setText("Ok");
    ui->m_statusIcon->clear();
    ui->m_statusIcon->setScaledContents(true);
    ui->m_statusIcon->setPixmap(*m_loaded);
    ui->m_cancel->setEnabled(true);
    adjustSize();
}
