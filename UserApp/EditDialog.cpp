#include "EditDialog.h"
#include "ui_EditDialog.h"

#include <QPushButton>
#include <QToolTip>
#include <QRegularExpression>
#include <boost/algorithm/string.hpp>

EditDialog::EditDialog( const QString& title, const QString& text, std::string& renameText, QWidget *parent ) :
    QDialog(parent),
    ui(new Ui::EditDialog),
    m_renameText(renameText)
{
    ui->setupUi(this);

    setWindowTitle( title );
    ui->label->setText( text );
    ui->name->setText( QString::fromStdString(renameText) );
    ui->name->selectAll();

    QRegularExpression nameTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9_]{1,40})")));
    connect(ui->name, &QLineEdit::textChanged, this, [this, nameTemplate] (auto text)
    {
        if (!nameTemplate.match(text).hasMatch()) {
            QToolTip::showText(ui->name->mapToGlobal(QPoint(0, 15)), tr("Invalid name!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->name->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->name->setProperty("is_valid", true);
            validate();
        }
    });

    if (!nameTemplate.match(ui->name->text()).hasMatch()) {
        QToolTip::showText(ui->name->mapToGlobal(QPoint(0, 15)), tr("Invalid name!"), nullptr, {}, 3000);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        ui->name->setProperty("is_valid", false);
    } else {
        QToolTip::hideText();
        ui->name->setProperty("is_valid", true);
        validate();
    }
}

void EditDialog::accept()
{
    m_renameText = ui->name->text().toStdString();
    QDialog::accept();
}

EditDialog::~EditDialog()
{
    delete ui;
}

void EditDialog::validate() {
    if (ui->name->property("is_valid").toBool()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
}
