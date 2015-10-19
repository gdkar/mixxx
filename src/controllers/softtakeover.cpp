/***************************************************************************
                          softtakeover.cpp  -  description
                          ----------------
    begin                : Sat Mar 26 2011
    copyright            : (C) 2011 by Sean M. Pappalardo
    email                : spappalardo@mixxx.org
 ***************************************************************************/

#include <QDateTime>

#include "controllers/softtakeover.h"
#include "controlpotmeter.h"
#include "util/math.h"
#include "util/time.h"

SoftTakeoverCtrl::SoftTakeoverCtrl() = default;
SoftTakeoverCtrl::~SoftTakeoverCtrl()
{
    QHashIterator<ControlObject*, SoftTakeover*> i(m_softTakeoverHash);
    while (i.hasNext())
    {
        i.next();
        delete i.value();
    }
}

void SoftTakeoverCtrl::enable(ControlObject* control)
{
    auto cpo = dynamic_cast<ControlPotmeter*>(control);
    if (!cpo ) return;
    // Initialize times
    if (!m_softTakeoverHash.contains(control)) m_softTakeoverHash.insert(control, new SoftTakeover());
}

void SoftTakeoverCtrl::disable(ControlObject* control)
{
    if (!control) return;
    auto pSt = m_softTakeoverHash.take(control);
    if (pSt) delete pSt;
}
bool SoftTakeoverCtrl::ignore(ControlObject* control, double newParameter)
{
    if (!control ) return false;
    auto ignore = false;
    auto pSt = m_softTakeoverHash.value(control);
    if (pSt) ignore = pSt->ignore(control, newParameter);
    return ignore;
}
SoftTakeover::SoftTakeover()
    : m_time(0),
      m_prevParameter(0),
      m_dThreshold(kDefaultTakeoverThreshold)
{
}
const double SoftTakeover::kDefaultTakeoverThreshold = 3.0 / 128;
void SoftTakeover::setThreshold(double threshold)
{
    m_dThreshold = threshold;
}

bool SoftTakeover::ignore(ControlObject* control, double newParameter)
{
    auto ignore = false;
    // We only want to ignore the controller when all of the following are true:
    //  - its previous and new values are far away from and on the same side
    //      of the current value of the control
    //  - it's been awhile since the controller last affected this control

    auto currentTime = Time::elapsedMsecs();
    // We will get a sudden jump if we don't ignore the first value.
    if (!m_time)
    {
        ignore = true;
        // Change the stored time (but keep it far away from the current time)
        //  so this block doesn't run again.
        m_time = 1;
        //qDebug() << "ignoring the first value" << newParameter;
    }
    else if ((currentTime - m_time) > SUBSEQUENT_VALUE_OVERRIDE_TIME_MILLIS)
    {
        // don't ignore value if a previous one was not ignored in time
        auto currentParameter = control->getParameter();
        auto difference = currentParameter - newParameter;
        auto prevDiff = currentParameter - m_prevParameter;
        if ((prevDiff < 0 && difference < 0) || (prevDiff > 0 && difference > 0))
        {
            // On same site (still on ignore site)
            if (std::abs(difference) > m_dThreshold && std::abs(prevDiff) > m_dThreshold) ignore = true;
        }
    }
    if (!ignore) m_time = currentTime;
    // Update the previous value every time
    m_prevParameter = newParameter;
    return ignore;
}
void SoftTakeover::ignoreNext()
{
    m_time = 0;
}
