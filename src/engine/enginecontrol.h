// enginecontrol.h
// Created 7/5/2009 by RJ Ryan (rryan@mit.edu)

#ifndef ENGINECONTROL_H
#define ENGINECONTROL_H

#include <QObject>
#include <QList>
#include <atomic>
#include <utility>
#include <memory>
#include <algorithm>
#include <iterator>

#include "preferences/usersettings.h"
#include "track/track.h"
#include "control/controlvalue.h"
#include "engine/effects/groupfeaturestate.h"
#include "engine/cachingreader.h"

class EngineMaster;
class EngineBuffer;

constexpr int kNoTrigger = -1;

/**
 * EngineControl is an abstract base class for objects which implement
 * functionality pertaining to EngineBuffer. An EngineControl is meant to be a
 * succinct implementation of a given EngineBuffer feature. Previously,
 * EngineBuffer was an example of feature creep -- this is the result.
 *
 * When writing an EngineControl class, the following two properties hold:
 *
 * EngineControl::process will only be called during an EngineBuffer::process
 * callback from the sound engine. This implies that any ControlObject accesses
 * made in either of these methods are mutually exclusive, since one is
 * exclusively in the call graph of the other.
 */
class EngineControl : public QObject {
    Q_OBJECT
  public:
    EngineControl(QString group,UserSettingsPointer pConfig,QObject *pParent = nullptr);
    virtual ~EngineControl();
    // Called by EngineBuffer::process every latency period. See the above
    // comments for information about guarantees that hold during this call. An
    // EngineControl can perform any upkeep operations that are necessary during
    // this call. If the EngineControl would like to request the playback
    // position to be altered, it should return the sample to seek to from this
    // method. Otherwise it should return kNoTrigger.
    virtual double process(double dRate,
                           double dCurrentSample,
                           double dTotalSamples,
                           int iBufferSize);

    virtual double nextTrigger(double dRate,
                               double dCurrentSample,
                               double dTotalSamples,
                               int iBufferSize);

    virtual double getTrigger(double dRate,
                              double dCurrentSample,
                              double dTotalSamples,
                              int iBufferSize);

    // hintReader allows the EngineControl to provide hints to the reader to
    // indicate that the given portion of a song is a potential imminent seek
    // target.
    virtual void hintReader(HintVector* pHintList);

    virtual void setEngineMaster(EngineMaster* pEngineMaster);
    void setEngineBuffer(EngineBuffer* pEngineBuffer);
    virtual void setCurrentSample(double dCurrentSample, double dTotalSamples);
    double getCurrentSample() const;
    double getTotalSamples() const;
    bool atEndPosition() const;

    QString getGroup() const;
    // Called to collect player features for effects processing.
    virtual void collectFeatureState(GroupFeatureState* pGroupFeatures) const;
    // Called whenever a seek occurs to allow the EngineControl to respond.
  signals:
    virtual void notifySeek(double dNewPlaypo);
    void seek(double fractionalPosition);
    void seekAbs(double sample);
    // Seek to an exact sample and don't allow quantizing adjustment.
    void seekExact(double sample);
  public slots:
    virtual void trackLoaded(TrackPointer pNewTrack, TrackPointer pOldTrack);
    virtual void onNotifySeek(double){}
  protected:
    EngineBuffer* pickSyncTarget();

    UserSettingsPointer getConfig();
    EngineMaster* getEngineMaster();
    EngineBuffer* getEngineBuffer();

    QString m_group;
    UserSettingsPointer m_pConfig;
  private:
    struct sot{
        double current;
        double total;
    };
    std::atomic<sot> m_sampleOfTrack;
    EngineMaster* m_pEngineMaster{nullptr};
    EngineBuffer* m_pEngineBuffer{nullptr};
};

#endif /* ENGINECONTROL_H */
