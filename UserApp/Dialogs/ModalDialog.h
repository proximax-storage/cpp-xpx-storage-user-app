#pragma once

#include <QDialog>
#include <QString>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <optional>
#include <functional>

class ModalDialog : public QDialog
{
    std::optional<std::function<void()>> m_callback;
    
public:
    ModalDialog( const std::string& title, const std::string& text, std::optional<std::function<void()>> callback = {} ) : QDialog(), m_callback(callback)
    {
        setWindowTitle( QString::fromStdString(title) );

        QLabel*      label  = new QLabel( QString::fromStdString(text), this );
        QHBoxLayout* layout = new QHBoxLayout( this );
        layout->addWidget( label );        
    }
    
    void closeModal( bool success )
    {
        reject();
        
        if ( success && m_callback )
        {
            (*m_callback)();
        }
    }
};
