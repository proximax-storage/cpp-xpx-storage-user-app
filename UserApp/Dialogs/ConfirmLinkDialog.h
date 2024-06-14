#pragma once

#include "Entities/DataInfo.h"
#include "qdialogbuttonbox.h"
#include "qgridlayout.h"
#include <QLabel>
#include <QLineEdit>
#include <QDialog>

class ConfirmLinkDialog : public QDialog
{
    Q_OBJECT

private:
    DataInfo               m_dataInfo;

    QGridLayout*            m_layout;
    QLabel*                 driveName;
    QLabel*                 path;
    QLabel*                 dataSize;
    QLabel*                 folderNameForSaving;
    QLabel*                 m_driveNameConfirmLabel;
    QLabel*                 m_pathConfirmLabel;
    QLabel*                 m_dataSizeConfirmLabel;
    QLineEdit*              m_folderNameConfirmEdit;
    QDialogButtonBox*       buttonBox;

private:
    void setFontAndSize();
    void setLayout();
    bool isValidFolderName(const std::string& filename);
    bool contains_invalid_chars(const std::string& filename, const std::string& invalid_chars);

public:
    explicit ConfirmLinkDialog( QWidget *parent   = nullptr
                              , DataInfo dataInfo = {});
    ~ConfirmLinkDialog();

    void accept() override;
    void reject() override;
};
