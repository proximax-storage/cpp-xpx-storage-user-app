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
    ModalDialog( const QString& title, const QString& text, std::optional<std::function<void()>> callback = {} ) : QDialog(), m_callback(callback)
    {
        setWindowTitle( title );

        QLabel*      label  = new QLabel( text, this );
        QHBoxLayout* layout = new QHBoxLayout( this );
        layout->addWidget( label );
        resize(sizeHint());
        // resize(320, 200);
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
