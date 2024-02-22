#pragma once

#include <QDialog>
#include "Models/Model.h"

namespace Ui {
class EditDialog;
}

class EditDialog : public QDialog
{
    Q_OBJECT

public:
    enum EntityType
    {
        DownloadChannel,
        Drive
    };

public:
    explicit EditDialog( const QString& title, const QString& text, std::string& renameText, EntityType type, Model* model, QWidget *parent = nullptr );
    ~EditDialog();

protected:
    void accept() override;

private:
    void validate();

private:
    Ui::EditDialog *ui;
    Model* mp_model;
};

