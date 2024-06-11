#pragma once

#include "OnChainClient.h"
#include "Entities/DataInfo.h"
#include "qdialogbuttonbox.h"
#include "qgridlayout.h"
#include "qlabel.h"
#include "qtextedit.h"
#include <QDialog>

namespace Ui
{
class PasteLinkDialog;
}

class MainWin;

class PasteLinkDialog : public QDialog
{
    Q_OBJECT

private:
    // Ui::PasteLinkDialog     *ui;
    MainWin*                m_mainwin;
    Model*                  m_model;
    QPushButton*            m_pasteLinkButton;
    DataInfo                m_dataInfo;
    bool                    m_dataInfoIsSet = false;

    QGridLayout*            m_layout;
    QTextEdit*              m_linkEdit;
    QLabel*                 driveName;
    QLabel*                 path;
    QLabel*                 dataSize;
    QLabel*                 folderName;
    QLabel*                 m_driveNameLabel;
    QLabel*                 m_pathLabel;
    QLabel*                 m_dataSizeLabel;
    QLabel*                 m_folderNameLabel;
    QDialogButtonBox*       buttonBox;

private:
    void setLayout();
    void setFontAndSize();

public:
    explicit PasteLinkDialog( MainWin* parent = nullptr, Model* model = nullptr);
    ~PasteLinkDialog();

public:
    void accept() override;
    void reject() override;
};
