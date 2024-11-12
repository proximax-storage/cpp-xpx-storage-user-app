#include "Models/Model.h"
#include "Dialogs/DriveInfoDialog.h"
#include "./ui_DriveInfoDialog.h"

#include <QRegularExpression>
#include <QToolTip>

DriveInfoDialog::DriveInfoDialog(const Drive& drive,
                                 QWidget *parent ) :
    QDialog( parent ),
    ui( new Ui::DriveInfoDialog() )
{
    ui->setupUi(this);
    setModal(true);

    ui->m_driveName->setText(drive.getName());
    ui->m_replicatorNumber->setText( QString::number(drive.getReplicatorsCount()) );
    ui->m_maxDriveSize->setText( QString::number(drive.getSize()) );
    ui->m_localDriveFolder->setText( drive.getLocalFolder() );

    ui->m_driveName->setReadOnly(true);
    ui->m_replicatorNumber->setReadOnly(true);
    ui->m_maxDriveSize->setReadOnly(true);
    ui->m_localDriveFolder->setReadOnly(true);

    QString title;
    title.append("About drive ");
    title.append("'");
    title.append(drive.getName());
    title.append("'");
    setWindowTitle(title);
    setFocus();
}

DriveInfoDialog::~DriveInfoDialog()
{
    delete ui;
}

void DriveInfoDialog::accept()
{
    QDialog::accept();
}

void DriveInfoDialog::reject()
{
    QDialog::reject();
}
