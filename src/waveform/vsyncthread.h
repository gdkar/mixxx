#ifndef VSYNCTHREAD_H
#define VSYNCTHREAD_H

#include <QTime>
#include <QThread>
#include <QSemaphore>
#include <QPair>
#include <QGLWidget>

#if defined(__APPLE__)

#elif defined(__WINDOWS__)

#else
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#ifndef QT_OPENGL_ES_2
    #include <qx11info_x11.h>
    #include <GL/glx.h>
    //#include "GL/glxext.h"
    // clean up after Xlib.h, which #defines values that conflict with QT.
    #undef Bool
    #undef Unsorted
    #undef None
    #undef Status
#endif // QT_OPENGL_ES_2
#endif // QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#endif

#include "util/performancetimer.h"

class GuiTick;

class VSyncThread : public QThread {
    Q_OBJECT
  public:
    enum VSyncMode {
        ST_TIMER = 0,
        ST_MESA_VBLANK_MODE_1,
        ST_SGI_VIDEO_SYNC,
        ST_OML_SYNC_CONTROL,
        ST_FREE,
        ST_COUNT // Dummy Type at last, counting possible types
    };

    static void swapGl(QGLWidget* glw, int index);

    VSyncThread(QObject* pParent, GuiTick* pGuiTick);
    ~VSyncThread();

    void run();
    void stop();

    bool waitForVideoSync(QGLWidget* glw);
    int elapsed();
    int usToNextSync();
    void setUsSyncIntervalTime(int usSyncTimer);
    void setVSyncType(int mode);
    int droppedFrames();
    void setSwapWait(int sw);
    int usFromTimerToNextSync(const PerformanceTimer& timer);
    void vsyncSlotFinished();
    void getAvailableVSyncTypes(QList<QPair<int, QString > >* list);
    void setupSync(QGLWidget* glw, int index);
    void waitUntilSwap(QGLWidget* glw);

  signals:
    void vsyncRender();
    void vsyncSwap();

  private:
    bool m_bDoRendering;
    //QGLWidget *m_glw;
    bool m_vSyncTypeChanged;
    int m_usSyncIntervalTime;
    int m_usWaitToSwap;
    enum VSyncMode m_vSyncMode;
    bool m_syncOk;
    int m_droppedFrames;
    int m_swapWait;
    PerformanceTimer m_timer;
    QSemaphore m_semaVsyncSlot;
    double m_displayFrameRate;
    int m_vSyncPerRendering;


    GuiTick* m_pGuiTick;
};


#endif // VSYNCTHREAD_H
