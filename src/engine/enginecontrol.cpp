// enginecontrol.cpp
// Created 7/5/2009 by RJ Ryan (rryan@mit.edu)

#include "engine/enginecontrol.h"
#include "engine/enginemaster.h"
#include "engine/enginebuffer.h"
#include "engine/sync/enginesync.h"
#include "mixer/playermanager.h"

EngineControl::EngineControl(QString group,
                             UserSettingsPointer pConfig, QObject *pParent)
        : QObject(pParent),
          m_group(group),
          m_pConfig(pConfig)
{
    connect(
        this,SIGNAL(notifySeek(double)),
        this,SLOT(onNotifySeek(double)),
        static_cast<Qt::ConnectionType>(Qt::AutoConnection|Qt::UniqueConnection));
    setCurrentSample(0.0, 0.0);

    if(auto p = qobject_cast<EngineBuffer*>(parent())) {
        p->addControl(this);
    }
}
EngineControl::~EngineControl() = default;
void EngineControl::collectFeatureState(GroupFeatureState* pGroupFeatures) const
{
    Q_UNUSED(pGroupFeatures);
}
double EngineControl::process(double,
                              double,
                              double,
                              int) {
    return kNoTrigger;
}

double EngineControl::nextTrigger(double,
                                  double,
                                  double,
                                  int) {
    return kNoTrigger;
}

double EngineControl::getTrigger(double,
                                 double,
                                 double,
                                 int) {
    return kNoTrigger;
}
void EngineControl::trackLoaded(TrackPointer , TrackPointer ) { }
void EngineControl::hintReader(HintVector*) { }
void EngineControl::setEngineMaster(EngineMaster* pEngineMaster)
{
    m_pEngineMaster = pEngineMaster;
}
void EngineControl::setEngineBuffer(EngineBuffer* pEngineBuffer)
{
    if(m_pEngineBuffer == pEngineBuffer)
        return;
    if(m_pEngineBuffer) {
    disconnect(this,
            SIGNAL(seekAbs(double)),
            pEngineBuffer,
            SLOT(slotControlSeekAbs(double))
           );
    disconnect(this,
            SIGNAL(seekExact(double)),
            pEngineBuffer,
            SLOT(slotControlSeekExact(double))
           );
    disconnect(this,
            SIGNAL(seek(double)),
            pEngineBuffer,
            SLOT(slotControlSeek(double))
           );

    }
    m_pEngineBuffer = pEngineBuffer;
    connect(this,
            SIGNAL(seekAbs(double)),
            pEngineBuffer,
            SLOT(slotControlSeekAbs(double)),
            static_cast<Qt::ConnectionType>(Qt::DirectConnection|Qt::AutoConnection)
           );
    connect(this,
            SIGNAL(seekExact(double)),
            pEngineBuffer,
            SLOT(slotControlSeekExact(double)),
            static_cast<Qt::ConnectionType>(Qt::DirectConnection|Qt::AutoConnection)
           );
    connect(this,
            SIGNAL(seek(double)),
            pEngineBuffer,
            SLOT(slotControlSeek(double)),
            static_cast<Qt::ConnectionType>(Qt::DirectConnection|Qt::AutoConnection)
           );
}

void EngineControl::setCurrentSample(double dCurrentSample, double dTotalSamples) {
    m_sampleOfTrack.store(sot{ dCurrentSample,dTotalSamples});
}

double EngineControl::getCurrentSample() const {
    return m_sampleOfTrack.load().current;
}

double EngineControl::getTotalSamples() const {
    return m_sampleOfTrack.load().total;
}
bool EngineControl::atEndPosition() const {
    auto sot = m_sampleOfTrack.load();
    return (sot.current>= sot.total);
}
QString EngineControl::getGroup() const { return m_group; }
UserSettingsPointer EngineControl::getConfig() {
    return m_pConfig;
}
EngineMaster* EngineControl::getEngineMaster() { return m_pEngineMaster; }
EngineBuffer* EngineControl::getEngineBuffer() { return m_pEngineBuffer; }
EngineBuffer* EngineControl::pickSyncTarget()
{
    if(auto pMaster = getEngineMaster()) {
        if(auto pEngineSync = pMaster->getEngineSync()) {
            // TODO(rryan): Remove. This is a linear search over groups in
            // EngineMaster. We should pass the EngineChannel into EngineControl.
            auto pThisChannel = pMaster->getChannel(getGroup());
            auto pChannel = pEngineSync->pickNonSyncSyncTarget(pThisChannel);
            return pChannel ? pChannel->getEngineBuffer() : nullptr;
        }
    }
    return nullptr;
}
