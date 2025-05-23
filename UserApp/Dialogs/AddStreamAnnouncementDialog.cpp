#include "Models/Model.h"
#include "Entities/Account.h"
#include "Entities/StreamInfo.h"
#include "AddStreamAnnouncementDialog.h"
#include "drive/Utils.h"
#include "./ui_AddStreamAnnouncementDialog.h"

#include <fstream>

#include <QFileDialog>
#include <QPushButton>
#include <QRegularExpression>
#include <QToolTip>
#include <QMessageBox>

AddStreamAnnouncementDialog::AddStreamAnnouncementDialog( OnChainClient* onChainClient,
                                                          Model*         model,
                                                          QString        driveKey,
                                                          QWidget*       parent ) :
    QDialog( parent ),
    ui( new Ui::AddStreamAnnouncementDialog() ),
    mp_onChainClient(onChainClient),
    m_model(model),
    mDriveKey(driveKey)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Confirm");
    setModal(true);

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
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddStreamAnnouncementDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AddStreamAnnouncementDialog::reject);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);

    setWindowTitle("Create Stream Announcement");
    setFocus();
    ui->m_title->setFocus();

    setTabOrder(ui->m_title, ui->m_dateTime);
    setTabOrder(ui->m_dateTime, ui->buttonBox);
}

AddStreamAnnouncementDialog::~AddStreamAnnouncementDialog()
{
    delete ui;
}

void AddStreamAnnouncementDialog::validate()
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
//        ui->m_errorText->setText( QString::fromStdString("Stream folder is not drive subfolder:\n"+QString(drive->getLocalFolder())));
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

void AddStreamAnnouncementDialog::accept()
{
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

    mUniqueFolderName = stdStringToQStringUtf8(sirius::drive::toString( buffer ).substr( 0, 40 ));
        
    auto streamFolder = fs::path(qStringToStdStringUTF8(drive->getLocalFolder()) + "/" +
                                    STREAM_ROOT_FOLDER_NAME + "/" +
                                    qStringToStdStringUTF8(mUniqueFolderName));
    
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
    auto confirmationCallback = [this, &drive](auto fee)
    {
        if (showConfirmationDialog(fee)) {
            drive->updateDriveState (registering);
            QDialog::accept();
            return true;
        }

        return false;
    };

    auto driveKeyHex = rawHashFromHex(drive->getKey());
    mp_onChainClient->applyDataModification(driveKeyHex, actionList, confirmationCallback);
}

void AddStreamAnnouncementDialog::reject()
{
    QDialog::reject();
}
