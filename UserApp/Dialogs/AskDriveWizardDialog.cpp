#include "Dialogs/AskDriveWizardDialog.h"
#include "Dialogs/AddDriveDialog.h"
#include <QDebug>

AskDriveWizardDialog::AskDriveWizardDialog(QString title,
                                           OnChainClient* onChainClient,
                                           Model* model,
                                           QWidget* parent)
    : QMessageBox(parent)
    , m_onChainClient(onChainClient)
    , m_model(model)
    , m_parent(parent)
{
    setWindowTitle("There are no created drives");
    setText( "<p align='center'>Create drive for streaming?<br><br></p>" );
    setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    setDefaultButton(QMessageBox::Ok);
    setBaseSize(1000, height());
}

void AskDriveWizardDialog::resizeEvent(QResizeEvent *event)
{
    QMessageBox::resizeEvent(event);
    setFixedWidth(500);
}
void AskDriveWizardDialog::showEvent(QShowEvent *event)
{
    QMessageBox::showEvent(event);
    setFixedWidth(500);
}

AskDriveWizardDialog::~AskDriveWizardDialog()
{
}
