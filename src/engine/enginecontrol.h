// enginecontrol.h
// Created 7/5/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <QObject>
#include <QList>

#include "preferences/usersettings.h"
#include "trackinfoobject.h"
#include "control/controlvalue.h"
#include "engine/effects/groupfeaturestate.h"
#include "cachingreader.h"

class EngineMaster;
class EngineBuffer;

const double kNoTrigger = -1;

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

    struct SampleOfTrack {
        double current;
        double total;
    };
    ControlValueAtomic<SampleOfTrack> m_sampleOfTrack;
    EngineMaster* m_pEngineMaster{nullptr};
    EngineBuffer* m_pEngineBuffer{nullptr};

  protected:
    void seek(double fractionalPosition);
    void seekAbs(double sample);
    // Seek to an exact sample and don't allow quantizing adjustment.
    void seekExact(double sample);
    EngineBuffer* pickSyncTarget();

    UserSettingsPointer getConfig();
    EngineMaster* getEngineMaster();
    EngineBuffer* getEngineBuffer();

    QString m_group;
    UserSettingsPointer m_pConfig;
  public:
    EngineControl(QString group,UserSettingsPointer _config);
    EngineControl(QObject *pParent, QString group,UserSettingsPointer _config);
    virtual ~EngineControl();
    // Called by EngineBuffer::process every latency period. See the above
    // comments for information about guarantees that hold during this call. An
    // EngineControl can perform any upkeep operations that are necessary during
    // this call. If the EngineControl would like to request the playback
    // position to be altered, it should return the sample to seek to from this
    // method. Otherwise it should return kNoTrigger.
  public slots:
    virtual double process(const double dRate,
                           const double dCurrentSample,
                           const double dTotalSamples,
                           const int iBufferSize);

    virtual double nextTrigger(const double dRate,
                               const double dCurrentSample,
                               const double dTotalSamples,
                               const int iBufferSize);

    virtual double getTrigger(const double dRate,
                              const double dCurrentSample,
                              const double dTotalSamples,
                              const int iBufferSize);
  public:
    // hintReader allows the EngineControl to provide hints to the reader to
    // indicate that the given portion of a song is a potential imminent seek
    // target.
    virtual void setEngineMaster(EngineMaster* pEngineMaster);
    void setEngineBuffer(EngineBuffer* pEngineBuffer);
    virtual void setCurrentSample(const double dCurrentSample, const double dTotalSamples);
    double getCurrentSample() const;
    double getTotalSamples() const;
    bool atEndPosition() const;
    QString getGroup() const;
    // Called whenever a seek occurs to allow the EngineControl to respond.
  public slots:
    // Called to collect player features for effects processing.
    virtual void collectFeatureState(GroupFeatureState* pGroupFeatures) const ;
    virtual void notifySeek(double dNewPlaypo);
    virtual void trackLoaded(TrackPointer pTrack);
    virtual void trackUnloaded(TrackPointer pTrack);
    virtual void hintReader(HintVector* pHintList);
};
