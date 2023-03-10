#include "Model.h"
#include "AddStreamAnnouncementDialog.h"
#include "./ui_AddStreamAnnouncementDialog.h"

#include <QFileDialog>
#include <QRegularExpression>
#include <QToolTip>

AddStreamAnnouncementDialog::AddStreamAnnouncementDialog( OnChainClient* onChainClient,
                                Model* model,
                                QWidget *parent ) :
    QDialog( parent ),
    ui( new Ui::AddStreamAnnouncementDialog() ),
    mp_onChainClient(onChainClient),
    mpModel(model)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Confirm");
    setModal(true);

    std::vector<std::string> drivesKeys;
    drivesKeys.reserve(mpModel->getDrives().size());
    for ( const auto& [key, drive] : mpModel->getDrives()) {
        drivesKeys.push_back(key);
        ui->selectDriveBox->addItem(drive.getName().c_str());
    }

    ui->selectDriveBox->model()->sort(0);
    ui->selectDriveBox->insertItem(0, "Select from my drives");
    ui->selectDriveBox->setCurrentIndex(0);

    connect( ui->selectDriveBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, drivesKeys] (int index)
    {
        if (index == 0) {
//            mCurrentDriveKey.clear();
//            ui->driveKey->setText("");
        } else if (index >= 1) {
//            mCurrentDriveKey = drivesKeys[--index];
//            ui->driveKey->setText(mCurrentDriveKey.c_str());
        }
    }, Qt::QueuedConnection);


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
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddStreamAnnouncementDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AddStreamAnnouncementDialog::reject);

    setWindowTitle("Create Stream Announcement");
    setFocus();
}

AddStreamAnnouncementDialog::~AddStreamAnnouncementDialog()
{
    delete ui;
}

void AddStreamAnnouncementDialog::validate()
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

void AddStreamAnnouncementDialog::accept()
{
//    mpModel->addStreamAnnouncement( driveKey, title, startTime, chunkFolder );

    QDialog::accept();
}

void AddStreamAnnouncementDialog::reject()
{
    QDialog::reject();
}
