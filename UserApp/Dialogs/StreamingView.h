#pragma once

#include <QDialog>

#ifdef _WIN64
    #include <QtMultimedia/QCamera>
    #include <QtMultimedia/QImageCapture>
    #include <QtMultimedia/QMediaRecorder>
    #include <QScopedPointer>
    #include <QtMultimedia/QMediaMetaData>
    #include <QtMultimedia/QMediaCaptureSession>
    #include <QtMultimedia/QMediaDevices>
    #include <QtMultimedia/QAudioInput>
#else
    #include <QCamera>
    #include <QImageCapture>
    #include <QMediaRecorder>
    #include <QScopedPointer>
    #include <QMediaMetaData>
    #include <QMediaCaptureSession>
    #include <QMediaDevices>
    #include <QAudioInput>
#endif


namespace Ui { class StreamingView; }

class StreamingView : public QDialog
{
public:
    Q_OBJECT

public:
    explicit StreamingView( std::function<void()> cancelStreamingFunc,
                            std::function<void()> finishStreamingFunc,
                            QWidget *parent = nullptr);
    
    ~StreamingView() override;

private:
    Ui::StreamingView* ui;
    
    QMediaCaptureSession    m_captureSession;
    QScopedPointer<QCamera> m_camera;
    
    std::function<void()> m_cancelStreamingFunc;
    std::function<void()> m_finishStreamingFunc;
};
