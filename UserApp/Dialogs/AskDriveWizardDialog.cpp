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
    setWindowTitle(title);
    setText( "<p align='center'><br><b>There are no created drives</b></p>" );
    setInformativeText( "<p align='center'>Create drive for streaming?</p>" );
    setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    setDefaultButton(QMessageBox::Ok);
    setBaseSize(1000, height());

    // processAction(exec());
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

void AskDriveWizardDialog::processAction(int choice)
{
    switch (choice)
    {
    case QMessageBox::Ok:
    {
        AddDriveDialog dialog(m_onChainClient, m_model, m_parent);

        if(auto rc = dialog.exec(); rc == QDialog::Accepted)
        {

        }
        else {

        }
        break;
    }

    case QMessageBox::Cancel:
        break;

    default:
        // should never be reached
        break;
    }
}

AskDriveWizardDialog::~AskDriveWizardDialog()
{
}
