#include "Settings.h"
#include "AddDownloadChannelDialog.h"
#include "CloseChannelDialog.h"
#include "ManageChannelsDialog.h"
#include "DownloadPaymentDialog.h"
#include "./ui_ManageChannelsDialog.h"

#include <QClipboard>
#include <QRegularExpression>
#include <random>

ManageChannelsDialog::ManageChannelsDialog( OnChainClient* onChainClient, QWidget *parent ) :
    QDialog( parent ),
    mpOnChainClient(onChainClient),
    ui( new Ui::ManageChannelsDialog() )
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Close");

    setModal(true);

    ui->m_table->setColumnCount(3);
    ui->m_table->setShowGrid(true);

    ui->m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    QStringList headers;
       headers.append("name");
       headers.append("channel hash");
       headers.append("drive hash");
    ui->m_table->setHorizontalHeaderLabels(headers);
    ui->m_table->horizontalHeader()->setStretchLastSection(true);
    ui->m_table->setEditTriggers(QTableWidget::NoEditTriggers);

    connect(ui->m_addBtn, &QPushButton::released, this, [this] ()
    {
        AddDownloadChannelDialog dialog(mpOnChainClient, this);
        connect(&dialog, &AddDownloadChannelDialog::addDownloadChannel, this, [this](auto channelName, auto channelKey, auto driveKey, auto allowedPublicKeys) {
            int row = ui->m_table->rowCount();
            ui->m_table->insertRow(row);
            ui->m_table->setItem( row, 0, new QTableWidgetItem( QString::fromStdString(channelName)) );
            ui->m_table->setItem( row, 1, new QTableWidgetItem( QString::fromStdString(channelKey)) );
            ui->m_table->setItem( row, 2, new QTableWidgetItem( QString::fromStdString(driveKey)) );
            ui->m_table->resizeColumnsToContents();

            emit addDownloadChannel(channelName, channelKey, driveKey, allowedPublicKeys);
        });
        dialog.exec();
    });

    connect(ui->m_editBtn, &QPushButton::released, this, [this] ()
    {
        auto index = ui->m_table->selectionModel()->currentIndex();
        if ( index.isValid() && index.column() == 0 )
        {
            ui->m_table->edit(index);
        }
    });

    connect(ui->m_removeBtn, &QPushButton::released, this, [this] () {
        int selectedRow = ui->m_table->currentRow();
        auto selectedChannel = ui->m_table->model()->index(selectedRow, 1);
        if (selectedChannel.isValid()) {
            auto channelKey = selectedChannel.data().toString();
            auto channel = Model::findChannel(channelKey.toStdString());
            if (!channel) {
                qWarning() << LOG_SOURCE << "bad channel";
                return;
            }

            CloseChannelDialog dialog(mpOnChainClient, channel->m_hash.c_str(), channel->m_name.c_str(), this);
            if ( dialog.exec() == QDialog::Accepted )
            {
                channel->m_isDeleting = true;
                ui->m_table->removeRow(selectedRow);
                emit updateChannels();
            }
        } else {
            qWarning() << LOG_SOURCE << "bad index";
        }
    });

    connect(ui->m_topupBtn, &QPushButton::released, this, [this]() {
        DownloadPaymentDialog dialog(mpOnChainClient, this);
        dialog.exec();
    });

    connect(ui->m_copyChannelBtn, &QPushButton::released, this, [this] ()
    {
        std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
        const auto& channels = gSettings.config().m_dnChannels;
        int currentRow = ui->m_table->currentRow();
        if ( currentRow >= 0 && currentRow < channels.size()  )
        {
            QClipboard* clipboard = QApplication::clipboard();
            if (!clipboard) {
                qWarning() << LOG_SOURCE << "bad clipboard";
                lock.unlock();
                return;
            }

            const QString publicKey = channels[currentRow].m_hash.c_str();
            clipboard->setText(publicKey, QClipboard::Clipboard);
            if (clipboard->supportsSelection()) {
                clipboard->setText(publicKey, QClipboard::Selection);
            }
        }

        lock.unlock();
    });

    connect(ui->m_copyDriveBtn, &QPushButton::released, this, [this] ()
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

        const auto& channels = gSettings.config().m_dnChannels;
        int i = 0;
        for( const auto& channel : channels )
        {
            ui->m_table->setItem( i, 0, new QTableWidgetItem( QString::fromStdString(channel.m_name)) );
            ui->m_table->setItem( i, 1, new QTableWidgetItem( QString::fromStdString(channel.m_hash)) );
            ui->m_table->setItem( i, 2, new QTableWidgetItem( QString::fromStdString(channel.m_driveHash)) );
            i++;
        }

        ui->m_table->resizeColumnsToContents();
        ui->m_table->setSelectionMode( QAbstractItemView::SingleSelection );
        ui->m_table->setSelectionBehavior( QAbstractItemView::SelectRows );
    }

    connect(ui->m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection &selected, auto){
        if (selected.isEmpty()) {
            ui->m_removeBtn->setDisabled(true);
        } else {
            ui->m_removeBtn->setEnabled(true);
        }
    });

    connect(ui->m_table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        qDebug () << "new name: " + item->text();
        QRegularExpression nameTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9_]{1,40})")));
        if (nameTemplate.match(item->text()).hasMatch()) {
            int selectedRow = ui->m_table->currentRow();
            auto selectedChannel = ui->m_table->model()->index(selectedRow, 1);
            if (!selectedChannel.isValid()) {
                qWarning () << LOG_SOURCE  << "bad index";
                return;
            }

            std::unique_lock<std::recursive_mutex> lock( gSettingsMutex );
            auto* channel = Model::findChannel( selectedChannel.data().toString().toStdString() );
            if ( !channel )
            {
                qWarning () << LOG_SOURCE  << "bad channel";
                return;
            }

            channel->m_name = item->text().toStdString();
            Model::saveSettings();
            lock.unlock();

            emit updateChannels();
        } else {
            qWarning () << "invalid new name: " + item->text();
        }
    });

    ui->buttonBox->disconnect(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ManageChannelsDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ManageChannelsDialog::reject);

    ui->m_removeBtn->setDisabled(true);
}

ManageChannelsDialog::~ManageChannelsDialog()
{
    delete ui;
}

void ManageChannelsDialog::accept()
{
    QDialog::accept();
}

void ManageChannelsDialog::reject()
{
    QDialog::reject();
}

bool ManageChannelsDialog::verify()
{
//    if ( verifyDriveName() && verifyLocalDriveFolder() && verifyKey() )
//    {
//        ui->m_errorText->setText( QString::fromStdString("") );
//        return true;
//    }
    return false;
}
