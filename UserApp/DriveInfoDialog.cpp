#include "Model.h"
#include "DriveInfoDialog.h"
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

    ui->m_driveName->setText( QString::fromStdString(drive.getName()) );
    ui->m_replicatorNumber->setText( QString::number(drive.getReplicatorsCount()) );
    ui->m_maxDriveSize->setText( QString::number(drive.getSize()) );
    ui->m_localDriveFolder->setText( QString::fromStdString(drive.getLocalFolder()) );

    QString title;
    title.append("About drive");
    title.append("'");
    title.append(drive.getName().c_str());
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
