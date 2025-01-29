#include "Models/Model.h"
#include "Entities/Account.h"
#include "Entities/StreamInfo.h"
#include "WizardAddStreamAnnounceDialog.h"
#include "drive/Utils.h"
#include "ui_WizardAddStreamAnnounceDialog.h"

#include <fstream>

#include <QFileDialog>
#include <QPushButton>
#include <QRegularExpression>
#include <QToolTip>
#include <QMessageBox>

WizardAddStreamAnnounceDialog::WizardAddStreamAnnounceDialog( OnChainClient* onChainClient,
                                                          Model*         model,
                                                          QString        driveKey,
                                                          QWidget*       parent ) :
    QDialog( parent ),
    ui( new Ui::WizardAddStreamAnnounceDialog() ),
    mp_onChainClient(onChainClient),
    m_model(model),
    mDriveKey(driveKey)
{
    if ( m_model->currentStreamingDrive() != nullptr )
    {
        QMessageBox msgBox;
        const QString message = "internal error: drive already is streaming: " + m_model->currentStreamingDrive()->getKey();
        msgBox.setText(message);
        msgBox.setStandardButtons( QMessageBox::Close );
        msgBox.exec();
        return;
    }

    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Confirm");
    setModal(true);
    if(auto currentDrive = model->findDrive(driveKey); currentDrive->getState() == DriveState::creating)
    {
        ui->m_driveSelection->setEnabled(false);
        ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
        ui->m_driveSelection->setPlaceholderText("drive creating...");
    }
    else
    {
        addDrivesToCBox();
        // ui->m_driveSelection->setCurrentIndex(
        //     ui->m_driveSelection->findData(QVariant::fromValue(mDriveKey))
        //     );
        //mDriveKey = ui->m_driveSelection->itemData(0).toString().toStdString();
    }

    ui->m_errorText->setText("");

    ui->m_dateTime->setDateTime( QDateTime::currentDateTime().addSecs(180) );



    // Title
    connect(ui->m_title, &QLineEdit::textChanged, this, [this](auto text){
        validate();
    });

    // Start time
    connect(ui->m_dateTime, &QDateTimeEdit::dateTimeChanged, this, [this](auto dateTime){
        validate();
    });

//    // Stream folder
//    connect(ui->m_localDriveFolder, &QLineEdit::textChanged, this, [this](auto text){
//        validate();
//    });
//
//    connect(ui->m_localFolderBtn, &QPushButton::released, this, [this]()
//    {
//        if ( mCurrentDriveKey.empty() )
//        {
//            QMessageBox msgBox;
//            const QString message = QString::fromStdString("Drive is not selected.");
//            msgBox.setText(message);
//            msgBox.setStandardButtons( QMessageBox::Close );
//            auto reply = msgBox.exec();
//            return;
//        }
//
//        auto* drive = mpModel->findDrive( mCurrentDriveKey );
//        if ( drive == nullptr )
//        {
//            QMessageBox msgBox;
//            const QString message = QString::fromStdString("Drive is not found.");
//            msgBox.setText(message);
//            msgBox.setStandardButtons( QMessageBox::Close );
//            auto reply = msgBox.exec();
//            return;
//        }
//
//        QFlags<QFileDialog::Option> options = QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks;
//#ifdef Q_OS_LINUX
//        options |= QFileDialog::DontUseNativeDialog;
//#endif
//        const QString path = QFileDialog::getExistingDirectory(this, tr("Choose directory"),
//                                                               QString::fromStdString(drive->getLocalFolder()), options);
//        ui->m_localDriveFolder->setText(path.trimmed());
//    });

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &WizardAddStreamAnnounceDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &WizardAddStreamAnnounceDialog::reject);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);

    setWindowTitle("Create Stream Announcement");
    setFocus();
    ui->m_title->setFocus();

    setTabOrder(ui->m_title, ui->m_dateTime);
    setTabOrder(ui->m_dateTime, ui->buttonBox);
}

WizardAddStreamAnnounceDialog::~WizardAddStreamAnnounceDialog()
{
    delete ui;
}

void WizardAddStreamAnnounceDialog::validate()
{
    if ( mDriveKey.isEmpty() )
    {
        ui->m_errorText->setText("Drive not assigned");
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

//    if ( ui->m_localDriveFolder->text().trimmed().size() == 0 )
//    {
//        ui->m_errorText->setText("Stream folder is not assigned");
//        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
//        return;
//    }
//
//    if ( ! std::filesystem::is_directory( ui->m_localDriveFolder->text().trimmed().toStdString() ) )
//    {
//        ui->m_errorText->setText("Stream folder not exists");
//        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
//        return;
//    }
//
//    mStreamFolder = std::filesystem::relative( ui->m_localDriveFolder->text().trimmed().toStdString(), drive->getLocalFolder() );
//    bool isRelative = !mStreamFolder.empty() && std::filesystem::path(mStreamFolder).native()[0] != '.';
//    if ( ! isRelative )
//    {
//        ui->m_errorText->setText( QString::fromStdString("Stream folder is not drive subfolder:\n"+std::string(drive->getLocalFolder())));
//        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
//        return;
//    }
//
//    const auto& announcements = mpModel->streamerAnnouncements();
//    const auto& it = std::find_if( announcements.begin(), announcements.end(), [this] (const auto& ann) {
//        return ann.m_localFolder == mStreamFolder;
//    });
//
//    if ( it != announcements.end() )
//    {
//        ui->m_errorText->setText("Stream folder is used for another stream");
//        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
//        return;
//    }

    ui->m_errorText->setText("");
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void WizardAddStreamAnnounceDialog::accept()
{
    if ( m_model->currentStreamingDrive() != nullptr )
    {
        QMessageBox msgBox;
        const QString message = "internal error: drive already is streaming: " + m_model->currentStreamingDrive()->getKey();
        msgBox.setText(message);
        msgBox.setStandardButtons( QMessageBox::Close );
        msgBox.exec();
        return;
    }
    
    auto* drive = m_model->findDrive( mDriveKey );
    if ( drive == nullptr )
    {
        qWarning() << "drive == nullptr";
        return;
    }

    if ( drive->getState() != no_modifications )
    {
        QMessageBox msgBox;
        const QString message = "Last drive operation is not completed.";
        msgBox.setText(message);
        msgBox.setStandardButtons( QMessageBox::Close );
        msgBox.exec();
        return;
    }
    
    //
    // Calculate unique stream folder
    //
    std::random_device   dev;
    std::seed_seq        seed({dev(), dev(), dev(), dev()});
    std::mt19937         rng(seed);

    std::array<uint8_t,32> buffer{};

    std::generate( buffer.begin(), buffer.end(), [&]
    {
        return std::uniform_int_distribution<std::uint32_t>(0,0xff) ( rng );
    });

    mUniqueFolderName = sirius::drive::toString( buffer ).substr( 0, 40 ).c_str();
        
    auto streamFolder = fs::path(qStringToStdStringUTF8(drive->getLocalFolder() + "/" + STREAM_ROOT_FOLDER_NAME + "/" + mUniqueFolderName));
    
    //
    // Try to create stream folder
    //
    std::error_code ec;
    fs::create_directories( streamFolder, ec );
    if ( ec )
    {
        QMessageBox msgBox;
        msgBox.setText( QString::fromStdString( "Cannot create folder: " + getSettingsFolder().string() ) );
        msgBox.setInformativeText( QString::fromStdString( ec.message() ) );
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return;
    }

    //
    // save stream annotaion on disk
    //
    StreamInfo  streamInfo( drive->getKey(),
                            ui->m_title->text(),
                            "",
                            ui->m_dateTime->dateTime().toSecsSinceEpoch(),
                            mUniqueFolderName );
    
    std::ostringstream os( std::ios::binary );
    cereal::PortableBinaryOutputArchive archive( os );
    archive( streamInfo );

    try
    {
        std::ofstream fStream( streamFolder.string() + "/" + STREAM_INFO_FILE_NAME, std::ios::binary );
        fStream << os.str();
        fStream.close();
    }
    catch (...) {
        QMessageBox msgBox;
        msgBox.setText( QString::fromStdString( "Cannot write file: " + streamFolder.string() + "/" + STREAM_INFO_FILE_NAME ) );
        msgBox.setInformativeText( QString::fromStdString( ec.message() ) );
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return;
    }

    //
    // Create action list
    //
    sirius::drive::ActionList actionList;
    auto destFolder = fs::path( STREAM_ROOT_FOLDER_NAME ).string() + "/" + qStringToStdStringUTF8(mUniqueFolderName);
    actionList.push_back( sirius::drive::Action::upload( streamFolder.string() + "/" + STREAM_INFO_FILE_NAME, destFolder + "/" + STREAM_INFO_FILE_NAME ) );

    //
    // Start modification
    //
    auto confirmationCallback = [this, drive](auto fee)
    {
        if (showConfirmationDialog(fee)) {
            drive->setCreatingStreamAnnouncement();
            drive->updateDriveState (registering);
            QDialog::accept();
            return true;
        }

        return false;
    };

    auto driveKeyHex = rawHashFromHex(drive->getKey());
    mp_onChainClient->applyDataModification(driveKeyHex, actionList, confirmationCallback);
}

void WizardAddStreamAnnounceDialog::reject()
{
    QDialog::reject();
}

// used when drive is created from wizard
//
void WizardAddStreamAnnounceDialog::onDriveCreated( const Drive* drive )
{
    QTimer::singleShot(10, this, [=, this]
    {
        ui->m_driveSelection->setEnabled(true);
        ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(true);
        ui->m_driveSelection->setPlaceholderText("");
        ui->m_driveSelection->insertItem(0, drive->getName(), drive->getKey());
        mDriveKey = drive->getKey();
    });
}

void WizardAddStreamAnnounceDialog::addDrivesToCBox()
{
    for (const auto& [key, drive] : m_model->getDrives())
    {
        int index = ui->m_driveSelection->findData(QVariant::fromValue(key)
                                                  , Qt::UserRole
                                                  , Qt::MatchFixedString);
        if (index == -1)
        {
            ui->m_driveSelection->addItem(drive.getName(), drive.getKey());
            ui->m_driveSelection->model()->sort(0);
        }
    }
}

void WizardAddStreamAnnounceDialog::on_m_driveSelection_activated(int index)
{
    QVariant driveKey = ui->m_driveSelection->itemData(index);
    mDriveKey = driveKey.toString();
}

