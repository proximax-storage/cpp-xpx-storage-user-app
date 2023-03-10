#include "mainwin.h"
#include "./ui_mainwin.h"

#include "AddStreamAnnouncementDialog.h"

#include <cerrno>
#include <qdebug.h>
#include <QFileIconProvider>
#include <QScreen>
#include <QComboBox>
#include <QProcess>
#include <QMessageBox>
#include <QDesktopServices>
#include <thread>
#include <QListWidget>
#include <QAction>
#include <QToolTip>
#include <QFileDialog>

#include <boost/algorithm/string.hpp>

void MainWin::initStreaming()
{
    connect(ui->m_addStreamAnnouncementBtn, &QPushButton::released, this, [this] () {
        AddStreamAnnouncementDialog dialog(m_onChainClient, m_model, this);
        dialog.exec();
    }, Qt::QueuedConnection);
}
