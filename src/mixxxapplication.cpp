

#include <QtDebug>
#include <QTouchEvent>

#include "mixxxapplication.h"
#include "controlobject.h"
#include "mixxx.h"

extern void qt_translateRawTouchEvent(QWidget *window,
        QTouchEvent::DeviceType deviceType,
        const QList<QTouchEvent::TouchPoint> &touchPoints);

MixxxApplication::MixxxApplication(int& argc, char** argv)
        : QApplication(argc, argv),
          m_activeTouchButton(Qt::NoButton)
{
}
MixxxApplication::~MixxxApplication() = default;
bool MixxxApplication::touchIsRightButton()
{
    if (!m_pTouchShift) m_pTouchShift = new ControlObject(ConfigKey("Controls", "touch_shift"),this);
    return (m_pTouchShift->get() != 0.0);
}
