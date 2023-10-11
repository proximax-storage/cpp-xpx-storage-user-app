#include "Model.h"
#include "AddStreamRefDialog.h"
#include "./ui_AddStreamRefDialog.h"

#include <QFileDialog>
#include <QRegularExpression>
#include <QToolTip>

AddStreamRefDialog::AddStreamRefDialog( OnChainClient* onChainClient,
                                Model* model,
                                QWidget *parent ) :
    QDialog( parent ),
    ui( new Ui::AddStreamRefDialog() ),
    mp_onChainClient(onChainClient),
    mpModel(model)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Confirm");
    setModal(true);

    ui->m_dateTime->setDateTime( QDateTime::currentDateTime() );

    connect(ui->m_localDriveFolder, &QLineEdit::textChanged, this, [this](auto text){
        bool isDirExists = QDir(text).exists();
        if (text.trimmed().isEmpty() || !isDirExists) {
            QToolTip::showText(ui->m_localDriveFolder->mapToGlobal(QPoint(0, 15)), tr(isDirExists ? "Invalid path!" : "Folder does not exists!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->m_localDriveFolder->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->m_localDriveFolder->setProperty("is_valid", true);
            validate();
        }
    });

    connect(ui->m_localFolderBtn, &QPushButton::released, this, [this]()
    {
        QFlags<QFileDialog::Option> options = QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks;
#ifdef Q_OS_LINUX
        options |= QFileDialog::DontUseNativeDialog;
#endif
        const QString path = QFileDialog::getExistingDirectory(this, tr("Choose directory"), "/", options);
        ui->m_localDriveFolder->setText(path.trimmed());
    });

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddStreamRefDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AddStreamRefDialog::reject);

    setWindowTitle("Create Stream Announcement");
    setFocus();
    ui->lineEdit->setFocus();

    setTabOrder(ui->lineEdit, ui->m_title);
    setTabOrder(ui->m_title, ui->m_dateTime);
    setTabOrder(ui->m_dateTime, ui->m_localDriveFolder);
    setTabOrder(ui->m_localDriveFolder, ui->m_localFolderBtn);
    setTabOrder(ui->m_localFolderBtn, ui->buttonBox);
}

AddStreamRefDialog::~AddStreamRefDialog()
{
    delete ui;
}

void AddStreamRefDialog::validate()
{
//    if (ui->m_driveName->property("is_valid").toBool() &&
//        ui->m_replicatorNumber->property("is_valid").toBool() &&
//        ui->m_size->property("is_valid").toBool() &&
//        ui->m_localDriveFolder->property("is_valid").toBool()) {
//        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
//    } else {
//        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
//    }
}

void AddStreamRefDialog::accept()
{
//    mpModel->addStreamAnnouncement( driveKey, title, startTime, chunkFolder );

    QDialog::accept();
}

void AddStreamRefDialog::reject()
{
    QDialog::reject();
}
