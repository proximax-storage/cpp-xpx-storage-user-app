#pragma once

#include <QDialog>

namespace Ui {
class EditDialog;
}

class EditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditDialog( const QString& title, const QString& text, std::string& renameText, QWidget *parent = nullptr );
    ~EditDialog();

protected:
    void accept() override;

private:
    void validate();

private:
    Ui::EditDialog *ui;
    std::string&    m_renameText;
};

