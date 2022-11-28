#include "mainwin.h"
#include "Utils.h"
#include "Model.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
restart_label:
    QApplication a(argc, argv);
    QApplication::setWindowIcon(QIcon("./resources/icons/icon.png"));
    qInstallMessageHandler(customMessageHandler);

#ifdef Q_OS_LINUX
    QFile styleFile( "./resources/styles/ubuntu.qss" );
    if ( styleFile.open( QFile::ReadOnly ) ) {
        a.setStyleSheet( styleFile.readAll() );
    } else {
        qWarning () << LOG_SOURCE << "default style not loaded";
    }
#endif

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
    w.init();
    if ( w.m_mustExit )
    {
        return 0;
    }
    w.show();

    auto rc = a.exec();

    Model::endStorageEngine();

    if ( rc == 1024 )
        goto restart_label;
}
