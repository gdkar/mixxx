// enginecontrol.cpp
// Created 7/5/2009 by RJ Ryan (rryan@mit.edu)

#include "engine/enginecontrol.h"
#include "engine/enginemaster.h"
#include "engine/enginebuffer.h"
#include "engine/sync/enginesync.h"
#include "mixer/playermanager.h"

EngineControl::EngineControl(QString group,
                             UserSettingsPointer pConfig)
        : m_group(group),
          m_pConfig(pConfig),
          m_pEngineMaster(NULL),
          m_pEngineBuffer(NULL) {
    setCurrentSample(0.0, 0.0);
}

EngineControl::~EngineControl() {
}

double EngineControl::process(const double,
                              const double,
                              const double,
                              const int) {
    return kNoTrigger;
}

double EngineControl::nextTrigger(const double,
                                  const double,
                                  const double,
                                  const int) {
    return kNoTrigger;
}

double EngineControl::getTrigger(const double,
                                 const double,
                                 const double,
                                 const int) {
    return kNoTrigger;
}

void EngineControl::trackLoaded(TrackPointer pNewTrack, TrackPointer pOldTrack) {
    Q_UNUSED(pNewTrack);
    Q_UNUSED(pOldTrack);
}

void EngineControl::hintReader(HintVector*) {
}

void EngineControl::setEngineMaster(EngineMaster* pEngineMaster) {
    m_pEngineMaster = pEngineMaster;
}

void EngineControl::setCurrentSample(const double dCurrentSample, const double dTotalSamples) {
    SampleOfTrack sot;
    sot.current = dCurrentSample;
    sot.total = dTotalSamples;
    m_sampleOfTrack = sot;
}

double EngineControl::getCurrentSample() const {
    return m_sampleOfTrack.current;
}

double EngineControl::getTotalSamples() const {
    return m_sampleOfTrack.total;
}

bool EngineControl::atEndPosition() const {
    auto sot = m_sampleOfTrack;
    return (sot.current >= sot.total);
}

QString EngineControl::getGroup() const {
    return m_group;
}

UserSettingsPointer EngineControl::getConfig() {
    return m_pConfig;
}

EngineMaster* EngineControl::getEngineMaster() {
    return m_pEngineMaster;
}

EngineBuffer* EngineControl::getEngineBuffer() {
    if(auto buf = qobject_cast<EngineBuffer*>(parent()))
        return buf;
    if(auto master = getEngineMaster()) {
        if(auto buf = master->findChild<EngineBuffer*>(getGroup()))
            return buf;
    }
    return nullptr;
}

void EngineControl::seekAbs(double playPosition) {
    if (auto buf = getEngineBuffer()) {
        buf->slotControlSeekAbs(playPosition);
    }
}

void EngineControl::seekExact(double playPosition)
{
    if (auto buf = getEngineBuffer()) {
        buf->slotControlSeekExact(playPosition);
    }
}

void EngineControl::seek(double sample)
{
    if (auto buf = getEngineBuffer()) {
        buf->slotControlSeek(sample);
    }
}
void EngineControl::notifySeek(double dNewPlaypos)
{
    Q_UNUSED(dNewPlaypos);
}
EngineBuffer* EngineControl::pickSyncTarget()
{
    if(auto pMaster = getEngineMaster()) {

        EngineSync* pEngineSync = pMaster->getEngineSync();
        if (pEngineSync == NULL) {
            return NULL;
        }

        // TODO(rryan): Remove. This is a linear search over groups in
        // EngineMaster. We should pass the EngineChannel into EngineControl.
        auto pThisChannel = pMaster->getChannel(getGroup());
        auto pChannel = pEngineSync->pickNonSyncSyncTarget(pThisChannel);
        return pChannel ? pChannel->getEngineBuffer() : NULL;
    }
    return nullptr;
}
