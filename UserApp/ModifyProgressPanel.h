#pragma once

#include <QFrame>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

#include <functional>

#include "Model.h"

//enum ModificationStatus { no_modification, is_registring, is_registred, is_approved, is_failed, is_canceling, is_canceled };


class ModifyProgressPanel : public QFrame
{
public:

    ModifyProgressPanel( int x, int y, QWidget* parent, std::function<void()> cancelModificationFunc );

    void setIsRegistring();
    void setIsRegistred();
    void setIsApproved();
    void setIsFailed();
    void setIsApprovedWithOldRootHash();
    void setIsCanceling();
    void setIsCanceled();

private:
    std::function<void()> m_cancelModificationFunc;

    QVBoxLayout*    m_vLayout;
    QFrame*         m_frame;
    QVBoxLayout*    m_vLayout_2;
    QLabel*         m_title;
    QLabel*         m_text1;
    QLabel*         m_text2;
    QLabel*         m_text3;
    QHBoxLayout*    m_hLayout;
    QPushButton*    m_cancelButton;
};

