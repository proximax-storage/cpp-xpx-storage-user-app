#include "mainwin.h"
#include "Utils.h"
#include "Model.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

#include <csignal>

void sigHandler(int s)
{
    std::cerr << "signal: " << s << "\n";
    std::signal(s, SIG_DFL);
    qApp->quit();
    exit(s);
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT,  sigHandler);
    std::signal(SIGTERM, sigHandler);
    std::signal(SIGKILL, sigHandler);
    std::signal(SIGILL,  sigHandler);
    std::signal(SIGABRT, sigHandler);
    std::signal(SIGSEGV, sigHandler);

restart_label:
    QApplication a(argc, argv);
    QApplication::setWindowIcon(QIcon(getResource("./resources/icons/icon.png")));
    qInstallMessageHandler(customMessageHandler);
// TODO: same style for all platforms
//#ifdef Q_OS_LINUX
//    QFile styleFile( getResource("./resources/styles/ubuntu.qss") );
//    if ( styleFile.open( QFile::ReadOnly ) ) {
//        a.setStyleSheet( styleFile.readAll() );
//    } else {
//        qWarning () << LOG_SOURCE << "default style not loaded";
//    }
//#endif

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Demo_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            QApplication::installTranslator(&translator);
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

    auto rc = QApplication::exec();
    if ( rc == 1024 )
        goto restart_label;

    return 0;
}
