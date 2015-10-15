#include <QtDebug>

#include "visualplayposition.h"
#include "controlobjectslave.h"
#include "controlobject.h"
#include "util/math.h"

//static
QMap<QString, QWeakPointer<VisualPlayPosition> > VisualPlayPosition::m_listVisualPlayPosition;
PaStreamCallbackTimeInfo VisualPlayPosition::m_timeInfo = { 0.0, 0.0, 0.0 };
PerformanceTimer VisualPlayPosition::m_timeInfoTime;

VisualPlayPosition::VisualPlayPosition(const QString& key)
        : m_valid(false),
          m_key(key),
          m_invalidTimeInfoWarned(false)
{
    m_audioBufferSize = new ControlObjectSlave("Master", "audio_buffer_size");
    m_audioBufferSize->setParent(this);
    m_audioBufferSize->connectValueChanged(this, SLOT(slotAudioBufferSizeChanged(double)));
    m_dAudioBufferSize = m_audioBufferSize->get();
}

VisualPlayPosition::~VisualPlayPosition() 
{
  m_listVisualPlayPosition.remove(m_key);
}

void VisualPlayPosition::set(double playPos, double rate, double positionStep, double pSlipPosition)
{
    VisualPlayPositionData data;
    data.m_referenceTime = m_timeInfoTime;
    // Time from reference time to Buffer at DAC in µs
    data.m_callbackEntrytoDac = (m_timeInfo.outputBufferDacTime - m_timeInfo.currentTime) * 1000000;
    data.m_enginePlayPos = playPos;
    data.m_rate = rate;
    data.m_positionStep = positionStep;
    data.m_pSlipPosition = pSlipPosition;

    if (data.m_callbackEntrytoDac < 0 || data.m_callbackEntrytoDac > m_dAudioBufferSize * 1000)
    {
        // m_timeInfo Invalid, Audio API broken
        if (!m_invalidTimeInfoWarned)
        {
            qWarning() << "VisualPlayPosition: Audio API provides invalid time stamps,"
                       << "waveform syncing disabled."
                       << "DacTime:" << m_timeInfo.outputBufferDacTime
                       << "EntrytoDac:" << data.m_callbackEntrytoDac;
            m_invalidTimeInfoWarned = true;
        }
        // Assume we are in time
        data.m_callbackEntrytoDac = m_dAudioBufferSize * 1000;
    }
    // Atomic write
    m_data.setValue(data);
    m_valid = true;
}

double VisualPlayPosition::getEffectiveTime(int remaining)
{
    if (m_valid)
    {
        auto data = m_data.getValue();
//        auto usRefToVSync = vsyncThread->usFromTimerToNextSync(&data.m_referenceTime);
        auto offset = remaining - data.m_callbackEntrytoDac;
        auto playPos = data.m_enginePlayPos;  // load playPos for the first sample in Buffer
        // add the offset for the position of the sample that will be transfered to the DAC
        // When the next display frame is displayed
        playPos += data.m_positionStep * offset * data.m_rate / m_dAudioBufferSize * 1e-3;
        //qDebug() << "delta Pos" << playPos - m_playPosOld << offset;
        //m_playPosOld = playPos;
        return playPos;
    }
    return -1;
}
void VisualPlayPosition::getPlaySlipAt(int usFromNow, double* playPosition, double* slipPosition)
{
    //static double testPos = 0;
    //testPos += 0.000017759; //0.000016608; //  1.46257e-05;
    //return testPos;
    if (m_valid)
    {
        VisualPlayPositionData data = m_data.getValue();
        int usElapsed = data.m_referenceTime.elapsed() * 1e-3;
        int dacFromNow = usElapsed - data.m_callbackEntrytoDac;
        int offset = dacFromNow - usFromNow;
        double playPos = data.m_enginePlayPos;  // load playPos for the first sample in Buffer
        playPos += data.m_positionStep * offset * data.m_rate / m_dAudioBufferSize * 1e-3;
        *playPosition = playPos;
        *slipPosition = data.m_pSlipPosition;
    }
}
double VisualPlayPosition::getEnginePlayPos()
{
    if (m_valid)
    {
        auto data = m_data.getValue();
        return data.m_enginePlayPos;
    }
    else return -1;
}

void VisualPlayPosition::slotAudioBufferSizeChanged(double size)
{
  m_dAudioBufferSize = size;
}

//static
QSharedPointer<VisualPlayPosition> VisualPlayPosition::getVisualPlayPosition(QString group)
{
    auto vpp = m_listVisualPlayPosition.value(group).toStrongRef();
    if (vpp.isNull())
    {
        vpp = QSharedPointer<VisualPlayPosition>::create(group);
        m_listVisualPlayPosition.insert(group, vpp);
    }
    return vpp;
}
//static
void VisualPlayPosition::setTimeInfo(const PaStreamCallbackTimeInfo* timeInfo)
{
    m_timeInfoTime.start();
    m_timeInfo = *timeInfo;
}
