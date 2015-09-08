#ifndef GUITICK_H
#define GUITICK_H

#include <QObject>

#include "util/performancetimer.h"
class ControlObject;
class QTimer;
class GuiTick : public QObject {
    Q_OBJECT
  public:
    GuiTick(QObject* pParent = nullptr);
    virtual ~GuiTick();
    virtual void process();
    static double cpuTimeLastTick();
  private:
    ControlObject* m_pCOGuiTickTime = nullptr;
    ControlObject* m_pCOGuiTick50ms = nullptr;
    PerformanceTimer m_cpuTimer;
    double m_lastUpdateTime         = 0;
    static double m_cpuTimeLastTick; // Stream Time in seconds
};

#endif // GUITICK_H
