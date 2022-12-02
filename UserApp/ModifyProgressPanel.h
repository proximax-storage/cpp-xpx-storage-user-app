#pragma once

#include <QFrame>
#include <QMovie>
#include <QPixmap>
#include <functional>

#include "Model.h"

namespace Ui {
    class Frame;
}

//enum ModificationStatus { no_modification, is_registring, is_registred, is_approved, is_failed, is_canceling, is_canceled };


class ModifyProgressPanel : public QFrame
{
public:

    ModifyProgressPanel( int x, int y, QWidget* parent, const std::function<void()>& cancelModificationFunc );
    ~ModifyProgressPanel();

    void setRegistering();
    void setRegistered();
    void setApproved();
    void setFailed();
    void setIsApprovedWithOldRootHash();
    void setCanceling();
    void setCanceled();

private:
    std::function<void()> m_cancelModificationFunc;
    Ui::Frame* ui;
    QMovie* m_loading;
    QPixmap* m_loaded;
    QPixmap* m_error;
    QSize m_commonSize;
};

