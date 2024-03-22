#pragma once

#include <QFrame>
#include <QMovie>
#include <QPixmap>
#include <QMessageBox>
#include "OnChainClient.h"


class AskDriveWizardDialog : public QMessageBox
{
public:

    AskDriveWizardDialog(QString title,
                         OnChainClient* onChainClient,
                         Model* model,
                         QWidget* parent);

    ~AskDriveWizardDialog();

    void processAction(int choice);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    OnChainClient*  m_onChainClient;
    Model*          m_model;
    QWidget*        m_parent;
};

