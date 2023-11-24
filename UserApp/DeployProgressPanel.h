#pragma once

#include <QFrame>
#include <QMovie>
#include <QPixmap>
#include <functional>

#include "Model.h"

namespace Ui {
    class Frame;
}

class DeployProgressPanel : public QFrame
{
public:
    enum Mode { deploy };
    
public:

    DeployProgressPanel( Model* model, int x, int y, QWidget* parent, const std::function<void()>& cancelDeployFunc, Mode mode = deploy );
    ~DeployProgressPanel();

    void setInitialize();
    void setDeploying();
    void setApproving();
    void setApproved();
    void setFailed();

private:
    std::function<void()> m_cancelDeployFunc;
    Mode m_mode;
    Ui::Frame* ui;
    Model* mp_model;
    QMovie* m_loading;
    QPixmap* m_loaded;
    QPixmap* m_error;
    QSize m_commonSize;
};