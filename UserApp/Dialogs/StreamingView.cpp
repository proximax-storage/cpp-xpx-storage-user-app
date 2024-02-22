#include "StreamingView.h"
#include "ui_StreamingView.h"

#include <iostream>

#include <QPushButton>
#include <QUrl>


// !!! opera very slow while playing Video or flash !!!
// "Menu -> settings -> browser" and uncheck "Use hardware acceleration when available"
// https://forums.opera.com/topic/26171/opera-very-slow-while-playing-video-or-flash

StreamingView::StreamingView( std::function<void()> cancelStreamingFunc,
                              std::function<void()> finishStreamingFunc,
                              QWidget* parent )
    : QDialog( parent, (Qt::Popup | Qt::Dialog) )
    , ui(new Ui::StreamingView)
    , m_cancelStreamingFunc(cancelStreamingFunc)
    , m_finishStreamingFunc(finishStreamingFunc)
{
    ui->setupUi(this);
    
    connect( ui->m_finishBtn, &QPushButton::released, this, [this]
    {
        m_finishStreamingFunc();
        accept();
    });
    
    connect( ui->m_cancelBtn, &QPushButton::released, this, [this]
    {
        m_cancelStreamingFunc();
        accept();
    });
    
    m_camera.reset( new QCamera( QMediaDevices::defaultVideoInput() ) );
    m_captureSession.setCamera( m_camera.data() );

    m_captureSession.setVideoOutput(ui->m_preview);

    ui->m_preview->setAspectRatioMode( Qt::IgnoreAspectRatio );
    //ui->m_preview->setFullScreen(true);
    
    if ( m_camera->cameraFormat().isNull() )
    {
        auto formats = QMediaDevices::defaultVideoInput().videoFormats();
        if ( !formats.isEmpty() )
        {
            m_camera->setCameraFormat( *formats.begin() );
        }
    }
    
    m_camera->start();

}

StreamingView::~StreamingView()
{
}

