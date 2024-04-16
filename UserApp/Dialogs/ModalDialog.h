#pragma once

#include <QDialog>
#include <QString>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

class ModalDialog : public QDialog
{
public:
    ModalDialog( const std::string& title, const std::string& text ) : QDialog()
    {
        setWindowTitle( QString::fromStdString(title) );

        QLabel*      label  = new QLabel( QString::fromStdString(text), this );
        QHBoxLayout* layout = new QHBoxLayout( this );
        layout->addWidget( label );        
    }
};
