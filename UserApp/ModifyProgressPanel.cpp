#include "ModifyProgressPanel.h"
#include "Model.h"

#include <QPushButton>

ModifyProgressPanel::ModifyProgressPanel( int x, int y, QWidget* parent, std::function<void()> cancelModificationFunc )
    : QFrame(parent), m_cancelModificationFunc( cancelModificationFunc )
{
    setGeometry( QRect( x, y, 300, 200 ) );

    setFrameShape( QFrame::StyledPanel );
    setLineWidth(1);
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Midlight);
    setFrameStyle( QFrame::Panel | QFrame::Sunken );

    m_vLayout = new QVBoxLayout(this);
    {
        m_title = new QLabel(this);
        m_title->setFrameStyle( QFrame::NoFrame );
        m_title->setText( "Modification status:");
        m_title->setStyleSheet("font-weight: bold;");
        m_title->setAlignment( Qt::AlignBottom | Qt::AlignHCenter );
        m_vLayout->addWidget( m_title );

        m_vLayout->addStretch(1);

        m_frame = new QFrame(this);
        m_frame->setFrameStyle( QFrame::Panel | QFrame::Sunken );
        m_vLayout->addWidget( m_frame );

        m_vLayout_2 = new QVBoxLayout(m_frame);

        {
            m_text1 = new QLabel(this);
            m_text1->setText( "modification is registring...");
            m_text1->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
            m_vLayout_2->addWidget( m_text1 );

            m_text2 = new QLabel(this);
            m_text2->setText( "");
            m_text2->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
            m_vLayout_2->addWidget( m_text2 );

            m_text3 = new QLabel(this);
            m_text3->setText( "");
            m_text3->setAlignment( Qt::AlignBottom | Qt::AlignLeft );
            m_vLayout_2->addWidget( m_text3 );
        }

        m_vLayout->addStretch(2);

        m_hLayout = new QHBoxLayout();
        m_vLayout->addLayout( m_hLayout );
        {
            m_hLayout->addStretch(1);
            m_hLayout->addWidget( m_cancelButton );
            m_hLayout->addStretch(1);
        }
    }

    m_frame->adjustSize();
    adjustSize();

    //setIsFailedAfterRegistred();

    connect( m_cancelButton, &QPushButton::released, this, [this]
    {
        qDebug() << "m_cancelButton, ::released:";

        if ( auto* driveInfo = Model::currentDriveInfoPtr(); driveInfo == nullptr )
        {
            setVisible(false);
        }
        else
        {
            qDebug() << "m_cancelButton, ::released: status: " << driveInfo->m_modificationStatus;

            if ( driveInfo->m_modificationStatus == is_registring ||
                 driveInfo->m_modificationStatus == is_registred )
            {
                qDebug() << "m_cancelButton, ::released: 1";
                m_cancelModificationFunc();
                driveInfo->m_modificationStatus = is_canceling;
                setIsCanceling();
            }
            else if ( driveInfo->m_modificationStatus == no_modification ||
                      driveInfo->m_modificationStatus == is_approved ||
                      driveInfo->m_modificationStatus == is_approvedWithOldRootHash ||
                      driveInfo->m_modificationStatus == is_failed ||
                      driveInfo->m_modificationStatus == is_canceled )
            {
                qDebug() << "m_cancelButton, ::released: 2";

                driveInfo->m_currentModificationHash.reset();
                driveInfo->m_modificationStatus = no_modification;
                setVisible(false);
            }
        }
    });

    stackUnder( this );
}

void ModifyProgressPanel::setIsRegistring()
{
    m_text1->setText( "modification is registring...");
    m_text1->setStyleSheet("color: black");
    m_text2->setText( "");
    m_text3->setText( "");
    m_frame->adjustSize();
    adjustSize();

    m_cancelButton->setEnabled(true);
}


void ModifyProgressPanel::setIsRegistred()
{
    qDebug() << "setIsRegistred(): '";

    m_text1->setText( "modification is registred (in blockchain)");
    m_text1->setStyleSheet("color: blue");
    m_text2->setText( "modification is uploading...");
    m_text2->setStyleSheet("color: black");
    m_text3->setText( "");
    m_frame->adjustSize();
    adjustSize();

    m_cancelButton->setEnabled(true);
}

void ModifyProgressPanel::setIsApproved()
{
    m_text1->setText( "modification is registred (in blockchain)");
    m_text1->setStyleSheet("color: blue");
    m_text2->setText( "modification is completed");
    m_text2->setStyleSheet("color: blue");
    m_text3->setText( "");
    m_cancelButton->setText("Ok");
    m_frame->adjustSize();
    adjustSize();

    m_cancelButton->setEnabled(true);
}

void ModifyProgressPanel::setIsFailed()
{
    m_text1->setText( "modification is failed");
    m_text1->setStyleSheet("color: red");
    m_text2->setText( "");
    m_text3->setText( "");
    m_cancelButton->setText("Close");
    m_frame->adjustSize();
    adjustSize();

    m_cancelButton->setEnabled(true);
}

void ModifyProgressPanel::setIsApprovedWithOldRootHash()
{
    m_text1->setText( "modification is registred (in blockchain)");
    m_text1->setStyleSheet("color: blue");
    m_text2->setText( "modification is completed");
    m_text2->setStyleSheet("color: blue");
    m_text3->setText( "root hash is not changed");
    m_text3->setStyleSheet("color: red");
    m_cancelButton->setText("Close");
    m_frame->adjustSize();
    adjustSize();

    m_cancelButton->setEnabled(true);
}

void ModifyProgressPanel::setIsCanceling()
{
    m_text1->setText( "modification is canceling...");
    m_text1->setStyleSheet("color: black");
    m_text2->setText( "");
    m_text3->setText( "");
    m_frame->adjustSize();
    adjustSize();

    m_cancelButton->setEnabled(false);
}

void ModifyProgressPanel::setIsCanceled()
{
    m_text1->setText( "modification is canceled");
    m_text1->setStyleSheet("color: black");
    m_text2->setText( "");
    m_text3->setText( "");
    m_frame->adjustSize();
    adjustSize();

    m_cancelButton->setEnabled(true);
}
