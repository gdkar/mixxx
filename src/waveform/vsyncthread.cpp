#include <QThread>
#include <QOpenGLWidget>
#include <QGLFormat>
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

#if defined(__APPLE__)
#elif defined(__WINDOWS__)
#else
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
   extern const QX11Info *qt_x11Info(const QPaintDevice *pd);
#endif
#endif

VSyncThread::VSyncThread(MixxxMainWindow* mixxxMainWindow)
        : QThread(mixxxMainWindow),
          m_bDoRendering(true),
          m_vSyncTypeChanged(false),
          m_nsSyncIntervalTime(33333333),
          m_nominalFrames(0),
          m_vSyncMode(ST_TIMER),
          m_syncOk(false),
          m_droppedFrames(0),
          m_swapWait(0),
          m_displayFrameRate(60.0),
          m_vSyncPerRendering(1),
          m_pGuiTick(mixxxMainWindow->getGuiTick()) {
}

VSyncThread::~VSyncThread() {
    m_bDoRendering = false;
    m_semaVsyncSlot.release(2); // Two slots
    wait();
    //delete m_glw;
}

void VSyncThread::stop() {
    m_bDoRendering = false;
}


void VSyncThread::run() {
    Counter droppedFrames("VsyncThread real time error");
    QThread::currentThread()->setObjectName("VSyncThread");

    m_nsWaitToSwap = m_nsSyncIntervalTime;
    m_timer.start();
    int64_t nextFrameTime = m_nsSyncIntervalTime;
    int64_t estimatedSwapTime = 264192;
    while (m_bDoRendering) {
        if (m_vSyncMode == ST_FREE) {
            // for benchmark only!

            Event::start("VsyncThread vsync render");
            // renders the waveform, Possible delayed due to anti tearing
            emit(vsyncRender());
            m_semaVsyncSlot.acquire();
            Event::end("VsyncThread vsync render");
            Event::start("VsyncThread vsync swap");
            emit(vsyncSwap()); // swaps the new waveform to front
            m_semaVsyncSlot.acquire();
            Event::end("VsyncThread vsync swap");

            m_timer.restart();
            m_nsWaitToSwap = 1000000;
            usleep(m_nsWaitToSwap/1000);
        } else { // if (m_vSyncMode == ST_TIMER) {

            Event::start("VsyncThread vsync render");
            emit(vsyncRender()); // renders the new waveform.

            // wait until rendering was scheduled. It might be delayed due a
            // pending swap (depends one driver vSync settings)
            m_semaVsyncSlot.acquire();
            Event::end("VsyncThread vsync render");
            
            // qDebug() << "ST_TIMER                      " << nsLast << nsRest;
            int64_t nsRemainingForSwap = nextFrameTime - ((int64_t)m_timer.elapsed() ) - estimatedSwapTime;
            // waiting for interval by sleep
            if (nsRemainingForSwap > 0 ) {
                Event::start("VsyncThread nsleep for VSync");
                usleep(nsRemainingForSwap/1000);
                Event::end("VsyncThread nsleep for VSync");
            }

            Event::start("VsyncThread vsync swap");
            // swaps the new waveform to front in case of gl-wf
            emit(vsyncSwap());

            // wait until swap occurred. It might be delayed due to driver vSync
            // settings.
            m_semaVsyncSlot.acquire();
            Event::end("VsyncThread vsync swap");

            // <- Assume we are VSynced here ->
            int64_t nsLastSwapTime = (int64_t)m_timer.elapsed() ;
            nextFrameTime  += m_nsSyncIntervalTime;
//            if (usRemainingForSwap < 0) {
//                // Our swapping call was already delayed
//                // The real swap might happens on the following VSync, depending on driver settings
//                m_droppedFrames++; // Count as Real Time Error
//                droppedFrames.increment();
//            }
            // try to stay in right intervals
            m_nsWaitToSwap = nextFrameTime - nsLastSwapTime;//m_nsSyncIntervalTime +
            if(m_nsWaitToSwap<(2*m_nsSyncIntervalTime)/3){
              estimatedSwapTime *= 1.25;
            }else if(m_nsWaitToSwap > (3*m_nsSyncIntervalTime)/2){
              estimatedSwapTime /= 1.25;
            }
            while(nextFrameTime - nsLastSwapTime  < 0){
              droppedFrames.increment();
              m_droppedFrames ++;
              nextFrameTime += m_nsWaitToSwap;
            }
//                    ((m_usWaitToSwap - nsLastSwapTime) % m_nsSyncIntervalTime);
        }

        // Qt timers are not that useful in our case, because they
        // are handled with priority without respecting the callback
        m_pGuiTick->process();
    }
}


// static
void VSyncThread::swapGl(QOpenGLWidget* glw, int index) {
    Q_UNUSED(index);
    // No need for glw->makeCurrent() here.
    //qDebug() << "swapGl" << m_timer.elapsed();
#if defined(__APPLE__)
//    glw->swapBuffers();
#elif defined(__WINDOWS__)
//    glw->swapBuffers();
#else
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    const QX11Info *xinfo = qt_x11Info(glw);
    glXSwapBuffers(xinfo->display(), glw->winId());
#else
//    glw->swapBuffers();
#endif // QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#endif
}

int VSyncThread::elapsed() {
    return (int)m_timer.elapsed() ;
}

void VSyncThread::setNsSyncIntervalTime(int syncTime) {
    m_nsSyncIntervalTime = syncTime;
    m_vSyncPerRendering = round(m_displayFrameRate * m_nsSyncIntervalTime );
}

void VSyncThread::setVSyncType(int type) {
    if (type >= (int)VSyncThread::ST_COUNT) {
        type = VSyncThread::ST_TIMER;
    }
    m_vSyncMode = (enum VSyncMode)type;
    m_droppedFrames = 0;
    m_vSyncTypeChanged = true;
}

int VSyncThread::nsToNextSync() {
    int nsRest = m_nsWaitToSwap - ((int)m_timer.elapsed() );
    // int math is fine here, because we do not expect times > 4.2 s
    if (nsRest < 0) {
        nsRest %= m_nsSyncIntervalTime;
        nsRest += m_nsSyncIntervalTime;
    }
    return nsRest;
}

int VSyncThread::nsFromTimerToNextSync(PerformanceTimer* timer) {
    int nsDifference = (int)m_timer.difference(timer) ;
    // int math is fine here, because we do not expect times > 4.2 s
    return nsDifference + m_nsWaitToSwap;
}

int VSyncThread::droppedFrames() {
    return m_droppedFrames;
}

void VSyncThread::vsyncSlotFinished() {
    m_semaVsyncSlot.release();
}

void VSyncThread::getAvailableVSyncTypes(QList<QPair<int, QString > >* pList) {
    for (int i = (int)VSyncThread::ST_TIMER; i < (int)VSyncThread::ST_COUNT; i++) {
        //if (isAvailable(type))  // TODO
        {
            enum VSyncMode mode = (enum VSyncMode)i;

            QString name;
            switch (mode) {
            case VSyncThread::ST_TIMER:
                name = tr("Timer (Fallback)");
                break;
            case VSyncThread::ST_MESA_VBLANK_MODE_1:
                name = tr("MESA vblank_mode = 1");
                break;
            case VSyncThread::ST_SGI_VIDEO_SYNC:
                name = tr("Wait for Video sync");
                break;
            case VSyncThread::ST_OML_SYNC_CONTROL:
                name = tr("Sync Control");
                break;
            case VSyncThread::ST_FREE:
                name = tr("Free + 1 ms (for benchmark only)");
                break;
            default:
                break;
            }
            QPair<int, QString > pair = QPair<int, QString >(i, name);
            pList->append(pair);
        }
    }
}
