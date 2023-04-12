#include "Model.h"
#include "StreamInfo.h"
#include "AddStreamAnnouncementDialog.h"
#include "./ui_AddStreamAnnouncementDialog.h"

#include <QFileDialog>
#include <QRegularExpression>
#include <QToolTip>
#include <QMessageBox>

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

    ui->m_errorText->setText("");

    ui->m_dateTime->setDateTime( QDateTime::currentDateTime().addSecs(180) );

    // Drive
    std::vector<std::string> drivesKeys;
    drivesKeys.reserve(mpModel->getDrives().size());
    ui->selectDriveBox->addItem("Select from my drives");
    for ( const auto& [key, drive] : mpModel->getDrives()) {
        drivesKeys.push_back(key);
        ui->selectDriveBox->addItem(drive.getName().c_str());
    }

    ui->selectDriveBox->setCurrentIndex(0);

    connect( ui->selectDriveBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, drivesKeys] (int index)
    {
        if (index == 0) {
            mCurrentDriveKey.clear();
        } else if (index >= 1) {
            mCurrentDriveKey = drivesKeys[--index];
        }
        validate();
    }, Qt::QueuedConnection);

    // Title
    connect(ui->m_title, &QLineEdit::textChanged, this, [this](auto text){
        validate();
    });

    // Start time
    connect(ui->m_dateTime, &QDateTimeEdit::dateTimeChanged, this, [this](auto dateTime){
        validate();
    });

    // Stream folder
    connect(ui->m_localDriveFolder, &QLineEdit::textChanged, this, [this](auto text){
        validate();
    });

    connect(ui->m_localFolderBtn, &QPushButton::released, this, [this]()
    {
        if ( mCurrentDriveKey.empty() )
        {
            QMessageBox msgBox;
            const QString message = QString::fromStdString("Drive is not selected.");
            msgBox.setText(message);
            msgBox.setStandardButtons( QMessageBox::Close );
            auto reply = msgBox.exec();
            return;
        }

        auto* drive = mpModel->findDrive( mCurrentDriveKey );
        if ( drive == nullptr )
        {
            QMessageBox msgBox;
            const QString message = QString::fromStdString("Drive is not found.");
            msgBox.setText(message);
            msgBox.setStandardButtons( QMessageBox::Close );
            auto reply = msgBox.exec();
            return;
        }

        QFlags<QFileDialog::Option> options = QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks;
#ifdef Q_OS_LINUX
        options |= QFileDialog::DontUseNativeDialog;
#endif
        const QString path = QFileDialog::getExistingDirectory(this, tr("Choose directory"),
                                                               QString::fromStdString(drive->getLocalFolder()), options);
        ui->m_localDriveFolder->setText(path.trimmed());
    });

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddStreamAnnouncementDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AddStreamAnnouncementDialog::reject);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);

    setWindowTitle("Create Stream Announcement");
    setFocus();
}

AddStreamAnnouncementDialog::~AddStreamAnnouncementDialog()
{
    delete ui;
}

void AddStreamAnnouncementDialog::validate()
{
    if ( mCurrentDriveKey.empty() )
    {
        ui->m_errorText->setText("Drive not assigned");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    auto* drive = mpModel->findDrive( mCurrentDriveKey );
    if ( drive == nullptr )
    {
        ui->m_errorText->setText("Drive not found");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    if ( ui->m_title->text().trimmed().size() == 0 )
    {
        ui->m_errorText->setText("No title");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    bool  startTimeIsValid = QDateTime::currentDateTime() < ui->m_dateTime->dateTime();
    if ( ! startTimeIsValid )
    {
        ui->m_errorText->setText("Start time is in the past");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    if ( ui->m_localDriveFolder->text().trimmed().size() == 0 )
    {
        ui->m_errorText->setText("Stream folder is not assigned");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    if ( ! std::filesystem::is_directory( ui->m_localDriveFolder->text().trimmed().toStdString() ) )
    {
        ui->m_errorText->setText("Stream folder not exists");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    mStreamFolder = std::filesystem::relative( ui->m_localDriveFolder->text().trimmed().toStdString(), drive->getLocalFolder() );
    bool isRelative = !mStreamFolder.empty() && std::filesystem::path(mStreamFolder).native()[0] != '.';
    if ( ! isRelative )
    {
        ui->m_errorText->setText( QString::fromStdString("Stream folder is not drive subfolder:\n"+std::string(drive->getLocalFolder())));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }
    
    const auto& announcements = mpModel->streamerAnnouncements();
    const auto& it = std::find_if( announcements.begin(), announcements.end(), [this] (const auto& ann) {
        return ann.m_localFolder == mStreamFolder;
    });

    if ( it != announcements.end() )
    {
        ui->m_errorText->setText("Stream folder is used for another stream");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    ui->m_errorText->setText("");
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void AddStreamAnnouncementDialog::accept()
{
    auto* drive = mpModel->findDrive( mCurrentDriveKey );
    if ( drive != nullptr )
    {
        mpModel->addStreamerAnnouncement( StreamInfo( drive->getKey(),
                                        ui->m_title->text().toStdString(),
                                        ui->m_dateTime->dateTime().toSecsSinceEpoch(),
                                        mStreamFolder ) );
    }
    QDialog::accept();
}

void AddStreamAnnouncementDialog::reject()
{
    QDialog::reject();
}
