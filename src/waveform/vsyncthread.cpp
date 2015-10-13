#include <QThread>
#include <QTime>
#include <QtDebug>
#include <QTime>

#include "mixxx.h"
#include "vsyncthread.h"
#include "util/performancetimer.h"
#include "util/event.h"
#include "util/counter.h"
#include "util/math.h"
#include "waveform/guitick.h"

VSyncThread::VSyncThread(MixxxMainWindow* mixxxMainWindow)
        : QThread(mixxxMainWindow),
          m_bDoRendering(true),
          m_vSyncTypeChanged(false),
          m_usSyncIntervalTime(33333),
          m_syncOk(false),
          m_droppedFrames(0),
          m_swapWait(0),
          m_displayFrameRate(60.0),
          m_vSyncPerRendering(1),
          m_pGuiTick(mixxxMainWindow->getGuiTick()) {
}
VSyncThread::~VSyncThread()
{
    m_bDoRendering = false;
    wait();
    //delete m_glw;
}
void VSyncThread::stop() {m_bDoRendering = false;}
void VSyncThread::run() {
    Counter droppedFrames("VsyncThread real time error");
    QThread::currentThread()->setObjectName("VSyncThread");
    m_usWaitToSwap = m_usSyncIntervalTime;
    m_timer.start();
    while (m_bDoRendering)
    {
        Event::start("VsyncThread vsync render");
        emit(vsyncRender()); // renders the new waveform.
        // wait until rendering was scheduled. It might be delayed due a
        // pending swap (depends one driver vSync settings)
        int usRemainingForSwap = m_usWaitToSwap - (int)m_timer.elapsed() / 1000;
        // waiting for interval by sleep
        if (usRemainingForSwap > 5)
        {
            Event::start("VsyncThread usleep for VSync");
            usleep(usRemainingForSwap - 5);
            Event::end("VsyncThread usleep for VSync");
        }
        // <- Assume we are VSynced here ->
        auto usLastSwapTime = (int)m_timer.restart() / 1000;
        if (usRemainingForSwap < 0)
        {
            // Our swapping call was already delayed
            // The real swap might happens on the following VSync, depending on driver settings
            m_droppedFrames++; // Count as Real Time Error
            droppedFrames.increment();
        }
        // try to stay in right intervals
        m_usWaitToSwap = m_usSyncIntervalTime + ((m_usWaitToSwap - usLastSwapTime) % m_usSyncIntervalTime);
        // Qt timers are not that useful in our case, because they
        // are handled with priority without respecting the callback
        m_pGuiTick->process();
    }
}
// static
int VSyncThread::elapsed()
{ 
  return (int)m_timer.elapsed() / 1000;
}
void VSyncThread::setUsSyncIntervalTime(int syncTime)
{
    m_usSyncIntervalTime = syncTime;
    m_vSyncPerRendering = round(m_displayFrameRate * m_usSyncIntervalTime / 1000);
}
int VSyncThread::usToNextSync()
{
    int usRest = m_usWaitToSwap - ((int)m_timer.elapsed() / 1000);
    // int math is fine here, because we do not expect times > 4.2 s
    if (usRest < 0) {
        usRest %= m_usSyncIntervalTime;
        usRest += m_usSyncIntervalTime;
    }
    return usRest;
}
int VSyncThread::usFromTimerToNextSync(PerformanceTimer* timer)
{
    int usDifference = (int)m_timer.difference(timer) / 1000;
    // int math is fine here, because we do not expect times > 4.2 s
    return usDifference + m_usWaitToSwap;
}
int VSyncThread::droppedFrames() {return m_droppedFrames;}
