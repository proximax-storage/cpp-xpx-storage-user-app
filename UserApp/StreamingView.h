#pragma once

#include <QDialog>

#include <QCamera>
#include <QImageCapture>
#include <QMediaRecorder>
#include <QScopedPointer>
#include <QMediaMetaData>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QAudioInput>

namespace Ui { class StreamingView; }

class StreamingView : public QDialog
{
public:
    Q_OBJECT

public:
    explicit StreamingView( QWidget *parent = nullptr);
    ~StreamingView() override;

private:
    Ui::StreamingView* ui;
    
    QMediaCaptureSession    m_captureSession;
    QScopedPointer<QCamera> m_camera;
    
//    QScopedPointer<QAudioInput>     m_audioInput;
//    QImageCapture*                  m_imageCapture;
//    QScopedPointer<QMediaRecorder>  m_mediaRecorder;

};
