#include "EditDialog.h"
#include "ui_EditDialog.h"

#include <boost/algorithm/string.hpp>

EditDialog::EditDialog( const QString& title, const QString& text, std::string& renameText, QWidget *parent ) :
    QDialog(parent),
    ui(new Ui::EditDialog),
    m_renameText(renameText)
{
    ui->setupUi(this);

    setWindowTitle( title );
    ui->label->setText( text );
    ui->lineEdit->setText( QString::fromStdString(renameText) );
}

void EditDialog::accept()
{
    auto renamedText = ui->lineEdit->text().toStdString();
    qDebug() << "EditDialog::accept: renamedText:" << renamedText.c_str();

    boost::trim( renamedText );
    qDebug() << "EditDialog::accept: renamedText:" << renamedText.c_str();
    if ( ! renamedText.empty() )
    {
        qDebug() << "EditDialog::accept: not empty";
        m_renameText = renamedText;
        QDialog::accept();
        return;
    }

    QDialog::reject();
}

EditDialog::~EditDialog()
{
    delete ui;
}

