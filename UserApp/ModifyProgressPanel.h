#pragma once

#include <QFrame>
#include <QMovie>
#include <QPixmap>
#include <functional>

#include "Model.h"

namespace Ui {
    class Frame;
}

class ModifyProgressPanel : public QFrame
{
public:
    enum Mode { drive_modification, streaming };
    
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

    void updateUploadedDataAmount(const uint64_t amount);

private:
    std::function<void()> m_cancelModificationFunc;
    Mode m_mode;
    Ui::Frame* ui;
    Model* mp_model;
    QMovie* m_loading;
    QPixmap* m_loaded;
    QPixmap* m_error;
    QSize m_commonSize;
};

