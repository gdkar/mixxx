// enginecontrol.cpp
// Created 7/5/2009 by RJ Ryan (rryan@mit.edu)

#include "engine/enginecontrol.h"
#include "engine/enginemaster.h"
#include "engine/enginebuffer.h"
#include "engine/sync/enginesync.h"
#include "playermanager.h"
#include "controlobjectslave.h"
EngineControl::EngineControl(QString group,ConfigObject<ConfigValue>* _config, QObject *p)
        : QObject(p),
          m_group(group),
          m_pConfig(_config),
          m_numDecks(new ControlObjectSlave(ConfigKey("Master", "num_decks"),this)) {
    setCurrentSample(0.0, 0.0);
}
EngineControl::~EngineControl() = default;
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

void EngineControl::trackLoaded(TrackPointer) {
}

void EngineControl::trackUnloaded(TrackPointer) {
}

void EngineControl::hintReader(HintVector*) {
}

void EngineControl::setEngineMaster(EngineMaster* pEngineMaster) {
    m_pEngineMaster = pEngineMaster;
}

void EngineControl::setEngineBuffer(EngineBuffer* pEngineBuffer) {
    m_pEngineBuffer = pEngineBuffer;
}

void EngineControl::setCurrentSample(double dCurrentSample, double dTotalSamples) {
    SampleOfTrack sot;
    sot.current = dCurrentSample;
    sot.total = dTotalSamples;
    m_sampleOfTrack.store(sot);
}

double EngineControl::getCurrentSample() const {
    return m_sampleOfTrack.load().current;
}

double EngineControl::getTotalSamples() const {
    return m_sampleOfTrack.load().total;
}

bool EngineControl::atEndPosition() const {
    SampleOfTrack sot = m_sampleOfTrack.load();
    return (sot.current >= sot.total);
}

QString EngineControl::getGroup() const {
    return m_group;
}

ConfigObject<ConfigValue>* EngineControl::getConfig() {
    return m_pConfig;
}

EngineMaster* EngineControl::getEngineMaster() {
    return m_pEngineMaster;
}

EngineBuffer* EngineControl::getEngineBuffer() {
    return m_pEngineBuffer;
}

void EngineControl::seekAbs(double playPosition) {
    if (m_pEngineBuffer) {
        m_pEngineBuffer->slotControlSeekAbs(playPosition);
    }
}

void EngineControl::seekExact(double playPosition) {
    if (m_pEngineBuffer) {
        m_pEngineBuffer->slotControlSeekExact(playPosition);
    }
}

void EngineControl::seek(double sample) {
    if (m_pEngineBuffer) {
        m_pEngineBuffer->slotControlSeek(sample);
    }
}

void EngineControl::notifySeek(double dNewPlaypos) {
    Q_UNUSED(dNewPlaypos);
}

EngineBuffer* EngineControl::pickSyncTarget() {
    EngineMaster* pMaster = getEngineMaster();
    if (!pMaster) {
        return NULL;
    }

    EngineSync* pEngineSync = pMaster->getEngineSync();
    if (pEngineSync == NULL) {
        return NULL;
    }

    // TODO(rryan): Remove. This is a linear search over groups in
    // EngineMaster. We should pass the EngineChannel into EngineControl.
    EngineChannel* pThisChannel = pMaster->getChannel(getGroup());
    EngineChannel* pChannel = pEngineSync->pickNonSyncSyncTarget(pThisChannel);
    return pChannel ? pChannel->getEngineBuffer() : NULL;
}
