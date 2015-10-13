_Pragma("once")
#include <QTime>
#include <QThread>
#include <QPair>

#if defined(__APPLE__)

#elif defined(__WINDOWS__)

#else
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    #include <qx11info_x11.h>
#endif // QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#endif

#include "util/performancetimer.h"


#if defined(__APPLE__)

#elif defined(__WINDOWS__)

#else
    #include <GL/glx.h>
    #include "GL/glxext.h"
    // clean up after Xlib.h, which #defines values that conflict with QT.
    #undef Bool
    #undef Unsorted
#endif

class GuiTick;
class MixxxMainWindow;

class VSyncThread : public QThread {
    Q_OBJECT
  public:
    VSyncThread(MixxxMainWindow* mixxMainWindow);
    ~VSyncThread();
    void run();
    void stop();
    int elapsed();
    int usToNextSync();
    void setUsSyncIntervalTime(int usSyncTimer);
    void setVSyncType(int mode);
    int droppedFrames();
    void setSwapWait(int sw);
    int usFromTimerToNextSync(PerformanceTimer* timer);
  signals:
    void vsyncRender();
  private:
    bool m_bDoRendering;
    bool m_vSyncTypeChanged;
    int m_usSyncIntervalTime;
    int m_usWaitToSwap;
    bool m_syncOk;
    int m_droppedFrames;
    int m_swapWait;
    PerformanceTimer m_timer;
    double m_displayFrameRate;
    int m_vSyncPerRendering;
    GuiTick* m_pGuiTick;
};
