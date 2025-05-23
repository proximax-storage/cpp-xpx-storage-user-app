#include "StreamingPanel.h"
#include "ui_StreamingPanel.h"
#include <QClipboard>

StreamingPanel::StreamingPanel( std::function<void()> cancelStreamingFunc,
                                std::function<void()> finishStreamingFunc,
                                QWidget *parent ) :
    QDialog(parent),
    ui(new Ui::StreamingPanel),
    m_cancelStreamingFunc(cancelStreamingFunc),
    m_finishStreamingFunc(finishStreamingFunc)
{
    ui->setupUi(this);
    
    connect( ui->m_cancelBtn, &QPushButton::released, this, [this]
    {
        m_cancelStreamingFunc();
    });
            
    connect( ui->m_finishBtn, &QPushButton::released, this, [this]
    {
        m_finishStreamingFunc();
    });
            
    connect( ui->m_copyLinkBtn, &QPushButton::released, this, [this]
    {
        QClipboard* clipboard = QApplication::clipboard();
        if ( !clipboard ) {
            qWarning() << "StreamingPanel: bad clipboard";
            return;
        }

        clipboard->setText( m_streamLink, QClipboard::Clipboard );
        if ( clipboard->supportsSelection() ) {
             clipboard->setText( m_streamLink, QClipboard::Selection );
        }
    });
}

StreamingPanel::~StreamingPanel()
{
    delete ui;
}

void StreamingPanel::onStartStreaming()
{
    m_woAnnoucement = false;
    ui->m_statusLabel->setText("starting...");
    ui->m_info->setText("");
    show();
}

void StreamingPanel::onStartStreamingWoAnnouncement()
{
    m_woAnnoucement = true;
    m_driveIsCreated = false;
    ui->m_statusLabel->setText("starting...");
    ui->m_info->setText("(creating drive)");
    show();
}

//void StreamingPanel::onDriveCreatedOrApproved();
//{
//    if ( ! m_driveIsCreated )
//    {
//        m_driveIsCreated = true;
//        ui->m_info->setText("(creating stream info)");
//    }
//    else
//    {
//        ui->m_info->setText("(regestering stream transaction)");
//    }
//    
//}

void StreamingPanel::onStreamStarted()
{
    ui->m_statusLabel->setText("is streaming...");
    ui->m_info->setText("");
}

void StreamingPanel::hidePanel()
{
    hide();
}

bool StreamingPanel::isVisible()
{
    return QDialog::isVisible();
}

void StreamingPanel::onUpdateStatus( const QString& status )
{
    ui->m_statusLabel->setText( status );
}

// for debugging
void StreamingPanel::setVisible( bool value )
{
    if ( value )
    {
        QDialog::setVisible(true);
        ui->m_finishBtn->setFocus();
    }
    else
    {
        QDialog::setVisible(false);
    }
}
