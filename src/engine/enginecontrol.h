// enginecontrol.h
// Created 7/5/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <QObject>
#include <QList>

#include "trackinfoobject.h"
#include "control/controlvalue.h"
#include "engine/effects/groupfeaturestate.h"
#include "cachingreader.h"

class EngineMaster;
class EngineBuffer;
class ControlObjectSlave;
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
  public:
    EngineControl(QString group, ConfigObject<ConfigValue>* _config, QObject * p = nullptr);
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
    virtual void setEngineBuffer(EngineBuffer* pEngineBuffer);
    virtual void setCurrentSample(double dCurrentSample, double dTotalSamples);
    virtual double getCurrentSample() const;
    virtual double getTotalSamples() const;
    virtual bool atEndPosition() const;
    virtual QString getGroup() const;

    // Called to collect player features for effects processing.
    virtual void collectFeatureState(GroupFeatureState* pGroupFeatures) const {
        Q_UNUSED(pGroupFeatures);
    }

    // Called whenever a seek occurs to allow the EngineControl to respond.
    virtual void notifySeek(double dNewPlaypo);
  public slots:
    virtual void trackLoaded(TrackPointer pTrack);
    virtual void trackUnloaded(TrackPointer pTrack);

  protected:
    void seek(double fractionalPosition);
    void seekAbs(double sample);
    // Seek to an exact sample and don't allow quantizing adjustment.
    void seekExact(double sample);
    EngineBuffer* pickSyncTarget();

    ConfigObject<ConfigValue>* getConfig();
    EngineMaster* getEngineMaster();
    EngineBuffer* getEngineBuffer();

    QString m_group;
    ConfigObject<ConfigValue>* m_pConfig = nullptr;

  private:
    struct SampleOfTrack {
      union{
        long double aligned;
        struct{
          double current;
          double total;
        };
      };
    };
    ControlValueAtomic<SampleOfTrack> m_sampleOfTrack;
    EngineMaster* m_pEngineMaster = nullptr;
    EngineBuffer* m_pEngineBuffer = nullptr;
    ControlObjectSlave* m_numDecks = nullptr;
};

