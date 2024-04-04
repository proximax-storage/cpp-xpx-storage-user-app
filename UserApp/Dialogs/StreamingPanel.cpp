#include "StreamingPanel.h"
#include "ui_StreamingPanel.h"

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
            
}

StreamingPanel::~StreamingPanel()
{
    delete ui;
}

void StreamingPanel::onStartStreaming()
{
    ui->m_statusLabel->setText("starting...");
    show();
}

void StreamingPanel::hidePanel()
{
    hide();
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
    }
    else
    {
        QDialog::setVisible(false);
    }
}
