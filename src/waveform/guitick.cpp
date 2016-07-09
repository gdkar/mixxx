#include <QTimer>

#include "guitick.h"
#include "control/controlobject.h"

GuiTick::GuiTick(QObject* pParent)
        : QObject(pParent),
     m_pCOGuiTickTime(new ControlObject(ConfigKey("[Master]", "guiTickTime"))),
     m_pCOGuiTick50ms(new ControlObject(ConfigKey("[Master]", "guiTick50ms")))
{
    m_pCOGuiTickTime->setParent(this);
    m_pCOGuiTick50ms->setParent(this);
    m_cpuTimer.start();
}

GuiTick::~GuiTick() = default;

// this is called from the VSyncThread
// with the configured waveform frame rate
void GuiTick::process()
{
    m_cpuTimeLastTick += m_cpuTimer.restart();
    auto cpuTimeLastTickSeconds = m_cpuTimeLastTick.toDoubleSeconds();
    m_pCOGuiTickTime->set(cpuTimeLastTickSeconds);
    if (m_cpuTimeLastTick - m_lastUpdateTime >= mixxx::Duration::fromMillis(50)) {
        m_lastUpdateTime = m_cpuTimeLastTick;
        m_pCOGuiTick50ms->set(cpuTimeLastTickSeconds);
    }
}
