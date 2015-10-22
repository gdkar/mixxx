/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Mon Feb 18 09:48:17 CET 2002
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                :
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include <QThread>
#include <QDir>
#include <QtDebug>
#include <QApplication>
#include <QStringList>
#include <QString>
#include <QTextCodec>
#include <QIODevice>
#include <QFile>

#include <stdio.h>
#include <iostream>

#include "mixxx.h"
#include "mixxxapplication.h"
#include "soundsourceproxy.h"
#include "preferences/errordialoghandler.h"
#include "util/cmdlineargs.h"
#include "util/logmanager.h"
#include "util/version.h"
#ifdef __WINDOWS__
#include "util/console.h"
#endif
#include <QFile>
#include <QFileInfo>
extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}
#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#endif

QStringList plugin_paths; //yes this is global. sometimes global is good.

//void qInitImages_mixxx();

int main(int argc, char * argv[])
{
#ifdef __WINDOWS__
    Console console;
#endif
#ifdef Q_OS_LINUX
    XInitThreads();
#endif
    // Check if an instance of Mixxx is already running
    // See http://qt.nokia.com/products/appdev/add-on-products/catalog/4/Utilities/qtsingleapplication

    // These need to be set early on (not sure how early) in order to trigger
    // logic in the OS X appstore support patch from QTBUG-16549.
    QCoreApplication::setOrganizationDomain("mixxx.org");
    // Setting the organization name results in a QDesktopStorage::DataLocation
    // of "$HOME/Library/Application Support/Mixxx/Mixxx" on OS X. Leave the
    // organization name blank.
    //QCoreApplication::setOrganizationName("Mixxx");
    QCoreApplication::setApplicationName("Mixxx");
    auto mixxxVersion = Version::version();
    auto mixxxVersionBA = mixxxVersion.toLocal8Bit();
    QCoreApplication::setApplicationVersion(mixxxVersion);
    // Construct a list of strings based on the command line arguments
    auto& args = CmdlineArgs::Instance();
    if (!args.Parse(argc, argv))
    {
        fputs("Mixxx DJ Software v", stdout);
        fputs(mixxxVersionBA.constData(), stdout);
        fputs(" - Command line options", stdout);
        fputs(
                   "\n(These are case-sensitive.)\n\n\
    [FILE]                  Load the specified music file(s) at start-up.\n\
                            Each must be one of the following file types:\n\
                            ", stdout);

        auto fileExtensions(SoundSourceProxy::getSupportedFileNamePatterns().join(" "));
        auto fileExtensionsBA = QString(fileExtensions).toUtf8();
        fputs(fileExtensionsBA.constData(), stdout);
        fputs("\n\n", stdout);
        fputs("\
                            Each file you specify will be loaded into the\n\
                            next virtual deck.\n\
\n\
    --resourcePath PATH     Top-level directory where Mixxx should look\n\
                            for its resource files such as controller mappings,\n\
                            overriding the default installation location.\n\
\n\
    --pluginPath PATH       Top-level directory where Mixxx should look\n\
                            for sound source plugins in addition to default\n\
                            locations.\n\
\n\
    --settingsPath PATH     Top-level directory where Mixxx should look\n\
                            for settings. Default is:\n", stdout);
        fprintf(stdout, "\
                            %s\n", args.getSettingsPath().toLocal8Bit().constData());
        fputs("\
\n\
    --controllerDebug       Causes Mixxx to display/log all of the controller\n\
                            data it receives and script functions it loads\n\
\n\
    --developer             Enables developer-mode. Includes extra log info,\n\
                            stats on performance, and a Developer tools menu.\n\
\n\
    --safeMode              Enables safe-mode. Disables OpenGL waveforms,\n\
                            and spinning vinyl widgets. Try this option if\n\
                            Mixxx is crashing on startup.\n\
\n\
    --locale LOCALE         Use a custom locale for loading translations\n\
                            (e.g 'fr')\n\
\n\
    -f, --fullScreen        Starts Mixxx in full-screen mode\n\
\n\
    -h, --help              Display this help message and exit", stdout);

        fputs("\n\n(For more information, see http://mixxx.org/wiki/doku.php/command_line_options)\n",stdout);
        return(0);
    }
    qInstallMessageHandler(LogManager::messageHandler);
    // Other things depend on this name to enforce thread exclusivity,
    //  so if you change it here, change it also in:
    //      * ErrorDialogHandler::errorDialog()
    QThread::currentThread()->setObjectName("Main");
    MixxxApplication a(argc, argv);
    // Support utf-8 for all translation strings. Not supported in Qt 5.
    // TODO(rryan): Is this needed when we switch to qt5? Some sources claim it
    // isn't.
    av_register_all();
    avcodec_register_all();
#ifdef __APPLE__
     auto dir = QDir(QApplication::applicationDirPath());
     // Set the search path for Qt plugins to be in the bundle's PlugIns
     // directory, but only if we think the mixxx binary is in a bundle.
     if (dir.path().contains(".app/"))
     {
         // If in a bundle, applicationDirPath() returns something formatted
         // like: .../Mixxx.app/Contents/MacOS
         dir.cdUp();
         dir.cd("PlugIns");
         qDebug() << "Setting Qt plugin search path to:" << dir.absolutePath();
         // asantoni: For some reason we need to do setLibraryPaths() and not
         // addLibraryPath(). The latter causes weird problems once the binary
         // is bundled (happened with 1.7.2 when Brian packaged it up).
         QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
     }
#endif
    auto mixxx = new MixxxMainWindow(&a, args);
    //a.setMainWidget(mixxx);
    QObject::connect(&a, &QApplication::lastWindowClosed, &a, &QApplication::quit);
    auto result = -1;
    if (!(ErrorDialogHandler::instance()->checkError()))
    {
        qDebug() << "Displaying mixxx";
        mixxx->show();
        qDebug() << "Running Mixxx";
        result = a.exec();
    }
    else mixxx->finalize();
    delete mixxx;
    qDebug() << "Mixxx shutdown complete with code" << result;
    LogManager::destroy();
//    qInstallMessageHandler(nullptr);  // Reset to default.
    // Don't make any more output after this
    //    or mixxx.log will get clobbered!

    //delete plugin_paths;
    return result;
}
