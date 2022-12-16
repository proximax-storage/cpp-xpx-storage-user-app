#include "Model.h"
#include "DriveInfoDialog.h"
#include "./ui_DriveInfoDialog.h"

#include <QIntValidator>
#include <QFileDialog>
#include <QRegularExpression>
#include <QToolTip>

DriveInfoDialog::DriveInfoDialog( const DriveInfo& info,
                                QWidget *parent ) :
    QDialog( parent ),
    ui( new Ui::DriveInfoDialog() )
{
    ui->setupUi(this);
    setModal(true);

    ui->m_driveName->setText( QString::fromStdString(info.m_name) );
    ui->m_replicatorNumber->setText( QString::number(info.m_replicatorNumber) );
    ui->m_maxDriveSize->setText( QString::number(info.m_maxDriveSize) );
    ui->m_localDriveFolder->setText( QString::fromStdString(info.m_localDriveFolder) );

    setWindowTitle("Drive Info");
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
