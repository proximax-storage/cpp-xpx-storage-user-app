#include "EditDialog.h"
#include "ui_EditDialog.h"

#include <QPushButton>
#include <QToolTip>
#include <QRegularExpression>


EditDialog::EditDialog( const QString& title, const QString& text, std::string& renameText, EntityType type, Model* model, QWidget *parent ) :
    QDialog(parent),
    ui(new Ui::EditDialog),
    mp_model(model)
{
    ui->setupUi(this);

    setWindowTitle( title );
    ui->label->setText( text );
    ui->name->setText( QString::fromStdString(renameText) );
    ui->name->selectAll();

    QRegularExpression nameTemplate(QRegularExpression::anchoredPattern(QLatin1String(R"([a-zA-Z0-9_]{1,40})")));
    connect(ui->name, &QLineEdit::textChanged, this, [this, nameTemplate, type, &renameText] (auto text)
    {
        bool isEntityExists = type == Drive ? mp_model->isDriveWithNameExists(text) : mp_model->isChannelWithNameExists(text);
        if (!nameTemplate.match(text).hasMatch() || isEntityExists) {
            QToolTip::showText(ui->name->mapToGlobal(QPoint(0, 15)), tr(isEntityExists ? "Invalid name(Already exists)!" : "Invalid name!"), nullptr, {}, 3000);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            ui->name->setProperty("is_valid", false);
        } else {
            QToolTip::hideText();
            ui->name->setProperty("is_valid", true);
            renameText = ui->name->text().toStdString();
        }

        validate();
    });

    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
}

void EditDialog::accept()
{
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
