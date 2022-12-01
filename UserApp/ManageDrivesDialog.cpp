#include "Settings.h"
#include "AddDriveDialog.h"
#include "CloseDriveDialog.h"
#include "ManageDrivesDialog.h"
#include "OnChainClient.h"
#include "StoragePaymentDialog.h"
#include "./ui_ManageDrivesDialog.h"

#include <QIntValidator>
#include <QClipboard>
#include <QFileDialog>

#include <random>

ManageDrivesDialog::ManageDrivesDialog( OnChainClient* onChainClient, QWidget *parent ) :
    QDialog( parent ),
    ui( new Ui::ManageDrivesDialog() ),
    m_onChainClient(onChainClient)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Close");

    setModal(true);
    ui->m_table->setColumnCount(3);
    ui->m_table->setShowGrid(true);

    QStringList headers;
       headers.append("name");
       headers.append("drive key");
       headers.append("local path");
    ui->m_table->setHorizontalHeaderLabels(headers);
    ui->m_table->horizontalHeader()->setStretchLastSection(true);
    ui->m_table->setEditTriggers(QTableWidget::NoEditTriggers);

    connect(ui->m_addBtn, &QPushButton::released, this, [this] ()
    {
        AddDriveDialog dialog( m_onChainClient, this );
        dialog.exec();
    });

    connect(ui->m_editNameBtn, &QPushButton::released, this, [this] ()
    {
        auto index = ui->m_table->selectionModel()->currentIndex();
        if (index.isValid())
        {
            ui->m_table->setCurrentCell(index.row(), 0);
            index = ui->m_table->selectionModel()->currentIndex();
            if ( index.isValid() ) {
                ui->m_table->edit(index);
            }
        }
    });

    connect(ui->m_editPathBtn, &QPushButton::released, this, [this] ()
    {
        auto index = ui->m_table->selectionModel()->currentIndex();
        if (!index.isValid())
        {
            qWarning () << LOG_SOURCE << "bad index";
            return;
        }

        if (index.column() != 2) {
            ui->m_table->setCurrentCell(index.row(), 2);
            index = ui->m_table->selectionModel()->currentIndex();
            if (!index.isValid())
            {
                qWarning () << LOG_SOURCE << "bad index";
                return;
            }
        }

        QFlags<QFileDialog::Option> options = QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks;
#ifdef Q_OS_LINUX
        options |= QFileDialog::DontUseNativeDialog;
#endif
        const QString path = QFileDialog::getExistingDirectory(this, tr("Choose directory"), "/", options);
        if (path.trimmed().isEmpty()) {
            qWarning () << LOG_SOURCE  << "bad path";
            return;
        }

        ui->m_table->item(index.row(), index.column())->setText(path);

        int selectedRow = ui->m_table->currentRow();
        auto selectedDrive = ui->m_table->model()->index(selectedRow, 1);
        if (!selectedDrive.isValid()) {
            qWarning () << LOG_SOURCE  << "bad index";
            return;
        }

        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
        auto* drive = Model::findDrive( selectedDrive.data().toString().toStdString() );
        if ( !drive )
        {
            qWarning () << LOG_SOURCE  << "bad drive";
            lock.unlock();
            return;
        }

        const std::string oldPath = drive->m_localDriveFolder;
        drive->m_localDriveFolder = path.toStdString();
        Model::saveSettings();
        lock.unlock();

        emit updateDrives(drive->m_driveKey, oldPath);
    });

    connect(ui->m_table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (item->column() == 0) {
            QRegularExpression nameTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9_]{1,40})")));
            if (nameTemplate.match(item->text()).hasMatch()) {
                int selectedRow = ui->m_table->currentRow();
                auto selectedDrive = ui->m_table->model()->index(selectedRow, 1);
                if (!selectedDrive.isValid()) {
                    qWarning() << LOG_SOURCE << "bad index";
                    return;
                }

                std::unique_lock<std::recursive_mutex> lock(gSettingsMutex);
                auto *drive = Model::findDrive(selectedDrive.data().toString().toStdString());
                if (!drive) {
                    qWarning() << LOG_SOURCE << "bad drive";
                    lock.unlock();
                    return;
                }

                drive->m_name = item->text().toStdString();
                Model::saveSettings();
                lock.unlock();

                emit updateDrives(drive->m_driveKey, drive->m_localDriveFolder);
            }
        }
    });

    connect(ui->m_removeBtn, &QPushButton::released, this, [this] ()
    {
        int selectedRow = ui->m_table->currentRow();
        auto selectedDrive = ui->m_table->model()->index(selectedRow, 1);
        if (selectedDrive.isValid()) {
            auto driveKey = selectedDrive.data().toString();
            auto drive = Model::findDrive(driveKey.toStdString());
            if (!drive) {
                qWarning() << LOG_SOURCE << "bad drive";
                return;
            }

            CloseDriveDialog dialog( m_onChainClient, drive->m_driveKey.c_str(), drive->m_name.c_str(), this);
            if ( dialog.exec() == QDialog::Accepted )
            {
                drive->m_isDeleting = true;
                ui->m_table->removeRow(selectedRow);
                emit updateDrives(drive->m_driveKey, drive->m_localDriveFolder);
            }

        } else {
            qWarning() << LOG_SOURCE << "bad index";
        }
    });

    connect(ui->m_topupBtn, &QPushButton::released, this, [this]() {
        StoragePaymentDialog dialog(m_onChainClient, this);
        dialog.exec();
    });

    connect(ui->m_copyHashBtn, &QPushButton::released, this, [this] ()
    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
        const auto& drives = gSettings.config().m_drives;
        int currentRow = ui->m_table->currentRow();
        if ( currentRow >= 0 && currentRow < drives.size()  )
        {
            QClipboard* clipboard = QApplication::clipboard();
            if (!clipboard) {
                qWarning() << LOG_SOURCE << "bad clipboard";
                lock.unlock();
                return;
            }

            const QString publicKey = drives[currentRow].m_driveKey.c_str();
            clipboard->setText(publicKey, QClipboard::Clipboard);
            if (clipboard->supportsSelection()) {
                clipboard->setText(publicKey, QClipboard::Selection);
            }
        }

        lock.unlock();
    });

    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
        const auto& drives = gSettings.config().m_drives;
        qDebug() << LOG_SOURCE << "drives.size: " << drives.size();

        int i = 0;
        for( const auto& drive : drives )
        {
            ui->m_table->insertRow(i);
            ui->m_table->setItem(i, 0, new QTableWidgetItem( QString::fromStdString(drive.m_name)));
            ui->m_table->setItem(i, 1, new QTableWidgetItem( QString::fromStdString(drive.m_driveKey)));
            ui->m_table->setItem(i, 2, new QTableWidgetItem( QString::fromStdString(drive.m_localDriveFolder)));
            i++;
        }

        ui->m_table->resizeColumnsToContents();
        ui->m_table->setSelectionMode( QAbstractItemView::SingleSelection );
        ui->m_table->setSelectionBehavior( QAbstractItemView::SelectItems );
    }

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ManageDrivesDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ManageDrivesDialog::reject);

    connect(ui->m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection &selected, auto){
        if (selected.isEmpty()) {
            ui->m_removeBtn->setDisabled(true);
            ui->m_editNameBtn->setDisabled(true);
            ui->m_editPathBtn->setDisabled(true);
            ui->m_copyHashBtn->setDisabled(true);
        } else {
            ui->m_removeBtn->setEnabled(true);
            ui->m_editNameBtn->setEnabled(true);
            ui->m_editPathBtn->setEnabled(true);
            ui->m_copyHashBtn->setEnabled(true);
        }
    });

    ui->m_removeBtn->setDisabled(true);
    ui->m_editNameBtn->setDisabled(true);
    ui->m_editPathBtn->setDisabled(true);
    ui->m_copyHashBtn->setDisabled(true);

    setWindowTitle("Manage drives");
    setFocus();
}

ManageDrivesDialog::~ManageDrivesDialog()
{
    delete ui;
}

void ManageDrivesDialog::accept()
{
    QDialog::accept();
}

void ManageDrivesDialog::reject()
{
    QDialog::reject();
}

bool ManageDrivesDialog::verify()
{
//    if ( verifyDriveName() && verifyLocalDriveFolder() && verifyKey() )
//    {
//        ui->m_errorText->setText( QString::fromStdString("") );
//        return true;
//    }
    return true;
}
