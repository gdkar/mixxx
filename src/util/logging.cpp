#include "util/logging.h"

#include <cstdio>
#include <iostream>

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QThread>
#include <QtDebug>
#include <QtGlobal>

#include "util/cmdlineargs.h"
#include "util/ffmpeg-utils.hpp"
namespace mixxx {
namespace {

QFile Logfile;
QMutex mutexLogfile;

// Debug message handler which outputs to both a logfile and prepends the thread
// the message came from.
void MessageHandler(
    QtMsgType type
  , const QMessageLogContext&
  , const QString& input)
{
    // It's possible to deadlock if any method in this function can
    // qDebug/qWarning/etc. Writing to a closed QFile, for example, produces a
    // qWarning which causes a deadlock. That's why every use of Logfile is
    // wrapped with isOpen() checks.
    QMutexLocker locker(&mutexLogfile);
    QByteArray ba;
    auto thread = QThread::currentThread();
    if (thread) {
        ba = "[" + QThread::currentThread()->objectName().toLocal8Bit() + "]: ";
    } else {
        ba = "[?]: ";
    }
    ba += input.toLocal8Bit();
    ba += "\n";
    if (!Logfile.isOpen()) {
        // This Must be done in the Message Handler itself, to guarantee that the
        // QApplication is initialized
        auto logLocation = CmdlineArgs::Instance().getSettingsPath();
        auto logFileName = QString{};
        // Rotate old logfiles
        //FIXME: cerr << doesn't get printed until after mixxx quits (???)
        for (auto i = 9; i >= 0; --i) {
            if (i == 0) {
                logFileName = QDir(logLocation).filePath("mixxx.log");
            } else {
                logFileName = QDir(logLocation).filePath(QString("mixxx.log.%1").arg(i));
            }
            QFileInfo logbackup(logFileName);
            if (logbackup.exists()) {
                QString olderlogname =
                        QDir(logLocation).filePath(QString("mixxx.log.%1").arg(i + 1));
                // This should only happen with number 10
                if (QFileInfo(olderlogname).exists()) {
                    QFile::remove(olderlogname);
                }
                if (!QFile::rename(logFileName, olderlogname)) {
                    std::cerr << "Error rolling over logfile " << logFileName.toStdString();
                }
            }
        }
        // WARNING(XXX) getSettingsPath() may not be ready yet. This causes
        // Logfile writes below to print qWarnings which in turn recurse into
        // MessageHandler -- potentially deadlocking.
        // XXX will there ever be a case that we can't write to our current
        // working directory?
        Logfile.setFileName(logFileName);
        Logfile.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    auto debugLevel = CmdlineArgs::Instance().getDebugLevel(); // Get message verbosity

    switch (type) {
    case QtDebugMsg: // debugLevel 2
        if (debugLevel > kDebugLevelDefault)
            fprintf(stderr, "Debug %s", ba.constData());
        if (Logfile.isOpen()) {
            Logfile.write("Debug ");
            Logfile.write(ba);
        }
        break;
    case QtWarningMsg: // debugLevel 1
        if (debugLevel > kDebugLevelMin)
            fprintf(stderr, "Warning %s", ba.constData());
        if (Logfile.isOpen()) {
            Logfile.write("Warning ");
            Logfile.write(ba);
        }
        break;
    case QtCriticalMsg: // debugLevel 0 (always shown)
        fprintf(stderr, "Critical %s", ba.constData());
        if (Logfile.isOpen()) {
            Logfile.write("Critical ");
            Logfile.write(ba);
        }
        break; //NOTREACHED
    case QtFatalMsg: // debugLevel 0 (always shown)
        fprintf(stderr, "Fatal %s", ba.constData());
        if (Logfile.isOpen()) {
            Logfile.write("Fatal ");
            Logfile.write(ba);
        }
        break; //NOTREACHED
    default: // debugLevel unknown, we'll assume Warning
        if (debugLevel > 0)
            fprintf(stderr, "Unknown %s", ba.constData());
        if (Logfile.isOpen()) {
            Logfile.write("Unknown ");
            Logfile.write(ba);
        }
        break;
    }
    if (Logfile.isOpen()) {
        Logfile.flush();
    }
}

void FFmpegMessageHandler(void * avcl, int level, const char *fmt, va_list vi)
{
    if(level == AV_LOG_PANIC)
        qFatal(fmt, vi);
    else if(level == AV_LOG_FATAL)
        qCritical(fmt,vi);
//    else if(level == AV_LOG_INFO)
//        qInfo(fmt,vi);
//    else if(level == AV_LOG_DEBUG || level == AV_LOG_VERBOSE)
//        qDebug(fmt,vi);
}
}  // namespace
// static
void Logging::initialize()
{
    qInstallMessageHandler(MessageHandler);
    av_register_all();
    av_log_set_callback(&FFmpegMessageHandler);
    av_log_set_level(AV_LOG_FATAL);
}
// static
void Logging::shutdown()
{
    av_log_set_callback(av_log_default_callback);
    qInstallMessageHandler(nullptr);  // Reset to default.

    // Don't make any more output after this
    //    or mixxx.log will get clobbered!
    QMutexLocker locker(&mutexLogfile);
    if (Logfile.isOpen()) {
        Logfile.close();
    }
}
}  // namespace mixxx
