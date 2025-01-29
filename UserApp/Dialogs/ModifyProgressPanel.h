#pragma once

#include <QFrame>
#include <QMovie>
#include <QPixmap>
#include <functional>

#include "Models/Model.h"

namespace Ui {
    class Frame;
}

class Drive;

class ModifyProgressPanel : public QFrame
{
public:
    enum Mode { drive_modification, create_announcement, is_streaming };
    
public:

    ModifyProgressPanel( Model* model, int x, int y, QWidget* parent, const std::function<void()>& cancelModificationFunc, Mode mode = drive_modification );
    ~ModifyProgressPanel();

    void setRegistering();
    void setUploading();
    void setApproving();
    void setApproved();
    void setFailed();
    void setCanceling();
    void setCanceled();
    void setWaitingStreamStart();
    void setStreamIsDownloading();

    void updateStreamingMode( Drive* );

    void updateUploadedDataAmount(const uint64_t amount, uint64_t replicatorNumber);
    
    bool setStartingWoAnnoncement( bool value ) { m_startingWoAnnoncement = value; }

private:
    std::function<void()> m_cancelModificationFunc;
    Mode m_mode;
    bool m_startingWoAnnoncement = false;
    Ui::Frame* ui;
    Model* mp_model;
    QMovie* m_loading;
    QPixmap* m_loaded;
    QPixmap* m_error;
    QSize m_commonSize;
    
    uint64_t m_lastUploadedDataAmount = 0;

};

