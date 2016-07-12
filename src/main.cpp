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
#include <QCoreApplication>
#include <QGuiApplication>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QStringList>
#include <QString>
#include <QTextCodec>

#include "mixxx.h"
#include "mixxxapplication.h"
#include "sources/soundsourceproxy.h"
#include "errordialoghandler.h"
#include "util/cmdlineargs.h"
#include "util/console.h"
#include "util/logging.h"
#include "util/version.h"

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#endif

int main(int argc, char * argv[])
{
    Console console;
#ifdef Q_OS_LINUX
    XInitThreads();
#endif
    // These need to be set early on (not sure how early) in order to trigger
    // logic in the OS X appstore support patch from QTBUG-16549.
    // Setting the organization name results in a QDesktopStorage::DataLocation
    // of "$HOME/Library/Application Support/Mixxx/Mixxx" on OS X. Leave the
    // organization name blank.
    //QCoreApplication::setOrganizationName("Mixxx");

    {
        auto format = QSurfaceFormat::defaultFormat();
        format.setVersion(3,3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setSwapInterval(0);

        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
        format.setAlphaBufferSize(8);

        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);

        format.setRenderableType(QSurfaceFormat::OpenGL);

        format.setSamples(4);

        format.setSwapBehavior(QSurfaceFormat::DefaultSwapBehavior);
        QSurfaceFormat::setDefaultFormat(format);
    }
    QCoreApplication::setOrganizationDomain("mixxx.org");
    QCoreApplication::setApplicationName(Version::applicationName());
    QCoreApplication::setApplicationVersion(Version::version());
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts,true);

    // Construct a list of strings based on the command line arguments
    auto& args = CmdlineArgs::Instance();
    if (!args.Parse(argc, argv)) {
        args.printUsage();
        return 0;
    }
    // If you change this here, you also need to change it in
    // ErrorDialogHandler::errorDialog(). TODO(XXX): Remove this hack.
    QThread::currentThread()->setObjectName("Main");
    mixxx::Logging::initialize();


    auto result = -1;
    {
        MixxxApplication a(argc, argv);
        // Enumerate and load SoundSource plugins
        SoundSourceProxy::loadPlugins();
#ifdef __APPLE__
        QDir dir(QApplication::applicationDirPath());
        // Set the search path for Qt plugins to be in the bundle's PlugIns
        // directory, but only if we think the mixxx binary is in a bundle.
        if (dir.path().contains(".app/")) {
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
        {
            MixxxMainWindow mixxx_win(&a, args);
            // When the last window is closed, terminate the Qt event loop.
            QObject::connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
            // If startup produced a fatal error, then don't even start the Qt event
            // loop.
            if (ErrorDialogHandler::instance()->checkError()) {
                mixxx_win.finalize();
            } else {
                qDebug() << "Displaying mixxx";
                mixxx_win.show();
                qDebug() << "Running Mixxx";
                result = a.exec();
            }
        }
        qDebug() << "Mixxx shutdown complete with code" << result;
        mixxx::Logging::shutdown();
    }
    return result;
}
