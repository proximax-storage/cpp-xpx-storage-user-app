#include "mainwin.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Demo_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    // https://doc.qt.io/archives/qt-4.8/style-reference.html
    //a.setStyleSheet("QLineEdit { background: yellow }");

    MainWin w;
    if ( w.m_mustExit )
    {
        return 0;
    }
    w.show();

    return a.exec();
}
