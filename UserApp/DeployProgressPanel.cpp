#include "DeployProgressPanel.h"
#include "Model.h"
#include "Utils.h"
#include "mainwin.h"
#include "ui_DeployProgressPanel.h"

#include <QPushButton>

DeployProgressPanel::DeployProgressPanel( Model* model, int x, int y, QWidget* parent, const std::function<void()>& cancelDeployFunc, Mode mode )
    : QFrame(parent)
    , ui(new Ui::Frame)
    , mp_model(model)
    , m_cancelDeployFunc( cancelDeployFunc )
    , m_mode(mode)
{
    ui->setupUi(this);

    m_commonSize = QSize(20, 20);
    m_loading = new QMovie(getResource("./resources/icons/loader.gif"));
    m_loading->setScaledSize(m_commonSize);
    m_loading->setParent(this);

    m_loaded = new QPixmap(getResource("./resources/icons/loaded.png"));
    m_error = new QPixmap(getResource("./resources/icons/error.png"));

    if ( mode == drive_modification )
    {
        ui->m_title->setWindowTitle("Deploy progress");
    }

    ui->m_title->setAlignment(Qt::AlignCenter);
    setGeometry( QRect( x, y, 280, 130) );
    setFrameShape( QFrame::StyledPanel );
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Midlight);
    setFrameStyle( (int)QFrame::StyledPanel | (int)QFrame::Raised );
    ui->m_cancel->setText("Ok");

    stackUnder( this );

    ui->horizontalLayout->setAlignment(Qt::AlignCenter);
    ui->m_statusIcon->setMovie(m_loading);
    ui->m_statusLabel->setText( "Initializing supercontract deployment");
    adjustSize();
}

DeployProgressPanel::~DeployProgressPanel()
{
}

void DeployProgressPanel::setInitialize()
{
    if ( m_mode == deploy )
    {
        ui->m_statusLabel->setText( "Initializing supercontract deployment");
    }

    ui->m_statusIcon->clear();
    ui->m_statusIcon->setScaledContents(true);
    ui->m_statusIcon->setMovie(m_loading);
    ui->m_cancel->setEnabled(false);
    ui->m_cancel->setText("Ok");
    m_loading->start();
    ui->m_statusLabel->adjustSize();
    adjustSize();
}

void DeployProgressPanel::setDeploying()
{
    if ( m_mode == deploy )
    {
        ui->m_statusLabel->setText( "Supercontract is being deployed");
    }

    ui->m_cancel->setEnabled(false);
    m_loading->start();

    adjustSize();
}

void DeployProgressPanel::setApproving()
{
    if ( m_mode == deploy )
    {
        ui->m_statusLabel->setText( "Approving supercontract deployment");
    }

    ui->m_cancel->setEnabled(false);
    m_loading->start();

    adjustSize();
}

void DeployProgressPanel::setApproved()
{
    if ( m_mode == deploy )
    {
        ui->m_statusLabel->setText("Supercontract deployment is completed");
    }

    ui->m_cancel->setText("Ok");
    ui->m_statusIcon->clear();
    ui->m_statusIcon->setScaledContents(true);
    m_loading->stop();
    ui->m_statusIcon->setPixmap(*m_loaded);
    ui->m_cancel->setEnabled(true);
    adjustSize();
}

void DeployProgressPanel::setFailed()
{
    if ( m_mode == drive_modification )
    {
        ui->m_statusLabel->setText( "Supercontract deployment failed!!");
    }

    ui->m_cancel->setText("Close");
    ui->m_statusIcon->clear();
    ui->m_statusIcon->setScaledContents(true);
    m_loading->stop();
    ui->m_statusIcon->setPixmap(*m_error);
    ui->m_cancel->setEnabled(true);
    adjustSize();
}
