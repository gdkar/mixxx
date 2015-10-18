_Pragma("once")
#include <QDebug>
#include <QString>

template <typename T>
QString toDebugString(const T& object)
{
    QString output;
#ifndef QT_NO_DEBUG_OUTPUT
    QDebug deb(&output);
    deb << object;
#endif
    return output;
}
// Calling this will report a qFatal and quit Mixxx, possibly disgracefully. Use
// very sparingly! A modal message box will be issued to the user which allows
// the Qt event loop to continue processing. This means that you must not call
// this function from a code section which is not re-entrant (e.g. paintEvent on
// a QWidget).
void reportFatalErrorAndQuit(QString message);
// Calling this will report a qCritical and quit Mixxx, possibly
// disgracefully. Use very sparingly! A modal message box will be issued to the
// user which allows the Qt event loop to continue processing. This means that
// you must not call this function from a code section which is not re-entrant
// (e.g. paintEvent on a QWidget).
void reportCriticalErrorAndQuit(QString message);
