#pragma once

#include <QDialog>

namespace Ui {
class StreamingPanel;
}

class StreamingPanel : public QDialog
{
    Q_OBJECT

    Ui::StreamingPanel *ui;

    bool                  m_woAnnoucement;
    bool                  m_driveIsCreated = false;

    std::function<void()> m_cancelStreamingFunc;
    std::function<void()> m_finishStreamingFunc;
    
    QString               m_streamLink;

public:
    explicit StreamingPanel( std::function<void()> cancelStreamingFunc,
                             std::function<void()> finishStreamingFunc,
                             QWidget *parent = nullptr );
    ~StreamingPanel();

    void setStreamLink( const QString& streamLink ) { m_streamLink = streamLink; }
    
    void onDriveCreatedOrApproved();
    void onStreamStarted();

    void onStartStreaming();
    void onStartStreamingWoAnnouncement();
    void hidePanel();
    bool isVisible();
    
    void setVisible( bool );

public slots:
    void onUpdateStatus( const QString& status );
};

