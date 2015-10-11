#include <QtDebug>

#include "visualplayposition.h"
#include "controlobjectslave.h"
#include "controlobject.h"
#include "util/math.h"
#include "waveform/vsyncthread.h"

//static
QMap<QString, QWeakPointer<VisualPlayPosition> > VisualPlayPosition::m_listVisualPlayPosition;
PaStreamCallbackTimeInfo VisualPlayPosition::m_timeInfo = { 0.0, 0.0, 0.0 };
PerformanceTimer VisualPlayPosition::m_timeInfoTime;

VisualPlayPosition::VisualPlayPosition(const QString& key)
        : m_valid(false),
          m_key(key),
          m_invalidTimeInfoWarned(false)
{
    m_audioBufferSize = new ControlObjectSlave("[Master]", "audio_buffer_size");
    m_audioBufferSize->setParent(this);
    m_audioBufferSize->connectValueChanged(this, SLOT(slotAudioBufferSizeChanged(double)));
    m_dAudioBufferSize = m_audioBufferSize->get();
}
VisualPlayPosition::~VisualPlayPosition() {m_listVisualPlayPosition.remove(m_key);}
void VisualPlayPosition::set(double playPos, double rate, double positionStep, double pSlipPosition)
{
    auto pData = QSharedPointer<VisualPlayPositionData>(new VisualPlayPositionData{});
    pData->m_referenceTime = m_timeInfoTime;
    // Time from reference time to Buffer at DAC in µs
    pData->m_callbackEntrytoDac = (m_timeInfo.outputBufferDacTime - m_timeInfo.currentTime) * 1000000;
    pData->m_enginePlayPos = playPos;
    pData->m_rate = rate;
    pData->m_positionStep = positionStep;
    pData->m_pSlipPosition = pSlipPosition;
    if (pData->m_callbackEntrytoDac < 0 || pData->m_callbackEntrytoDac > m_dAudioBufferSize * 1000) {
        // m_timeInfo Invalid, Audio API broken
        if (!m_invalidTimeInfoWarned) {
            qWarning() << "VisualPlayPosition: Audio API provides invalid time stamps,"
                       << "waveform syncing disabled."
                       << "DacTime:" << m_timeInfo.outputBufferDacTime
                       << "EntrytoDac:" << pData->m_callbackEntrytoDac;
            m_invalidTimeInfoWarned = true;
        }
        // Assume we are in time
        pData->m_callbackEntrytoDac = m_dAudioBufferSize * 1000;
    }
    // Atomic write
    m_data.swap(pData);
    m_valid = true;
}
double VisualPlayPosition::getAtNextVSync(VSyncThread* vsyncThread) {
    //static double testPos = 0;
    //testPos += 0.000017759; //0.000016608; //  1.46257e-05;
    //return testPos;
    if (m_valid) {
        auto data = m_data;
        auto usRefToVSync = vsyncThread->usFromTimerToNextSync(&data->m_referenceTime);
        auto offset = usRefToVSync - data->m_callbackEntrytoDac;
        auto playPos = data->m_enginePlayPos;  // load playPos for the first sample in Buffer
        // add the offset for the position of the sample that will be transfered to the DAC
        // When the next display frame is displayed
        playPos += data->m_positionStep * offset * data->m_rate / m_dAudioBufferSize / 1000;
        //qDebug() << "delta Pos" << playPos - m_playPosOld << offset;
        //m_playPosOld = playPos;
        return playPos;
    }
    return -1;
}
void VisualPlayPosition::getPlaySlipAt(int usFromNow, double* playPosition, double* slipPosition) {
    //static double testPos = 0;
    //testPos += 0.000017759; //0.000016608; //  1.46257e-05;
    //return testPos;
    if (m_valid) {
        auto data = m_data;
        auto usElapsed = data->m_referenceTime.elapsed() / 1000;
        auto dacFromNow = usElapsed - data->m_callbackEntrytoDac;
        auto offset = dacFromNow - usFromNow;
        auto playPos = data->m_enginePlayPos;  // load playPos for the first sample in Buffer
        playPos += data->m_positionStep * offset * data->m_rate / m_dAudioBufferSize / 1000;
        *playPosition = playPos;
        *slipPosition = data->m_pSlipPosition;
    }
}
double VisualPlayPosition::getEnginePlayPos() {
    if (m_valid)
    {
        auto data = m_data;
        return data->m_enginePlayPos;
    } else {return -1;}
}
void VisualPlayPosition::slotAudioBufferSizeChanged(double size) {m_dAudioBufferSize = size;}
//static
QSharedPointer<VisualPlayPosition> VisualPlayPosition::getVisualPlayPosition(QString group)
{
    auto vpp = m_listVisualPlayPosition.value(group).toStrongRef();
    if (vpp.isNull()) {
        vpp = QSharedPointer<VisualPlayPosition>::create(group);
        m_listVisualPlayPosition.insert(group, vpp);
    }
    return vpp;
}
//static
void VisualPlayPosition::setTimeInfo(const PaStreamCallbackTimeInfo* timeInfo) {
    // the timeInfo is valid only just NOW, so measure the time from NOW for
    // later correction
    m_timeInfoTime.start();
    m_timeInfo = *timeInfo;
    //qDebug() << "TimeInfo" << (timeInfo->currentTime - floor(timeInfo->currentTime)) << (timeInfo->outputBufferDacTime - floor(timeInfo->outputBufferDacTime));
}
