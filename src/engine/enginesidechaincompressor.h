#ifndef ENGINECOMPRESSOR_H
#define ENGINECOMPRESSOR_H

#include "util/types.h"
#include "engine/engineobject.h"
class EngineSideChainCompressor : public EngineObject {
  Q_OBJECT
  Q_PROPERTY(CSAMPLE threshold READ threshold WRITE setThreshold NOTIFY thresholdChanged);
  Q_PROPERTY(CSAMPLE strength  READ strength  WRITE setStrength NOTIFY strengthChanged);
  Q_PROPERTY(double attackTime READ attackTime WRITE setAttackTime NOTIFY attackTimeChanged);
  Q_PROPERTY(double decayTime  READ decayTime  WRITE setDecayTime NOTIFY decayTimeChanged);
  signals:
    void thresholdChanged(CSAMPLE);
    void strengthChanged(CSAMPLE);
    void attackTimeChanged(double);
    void decayTimeChanged(double);
  public:
    EngineSideChainCompressor(const QString &group,QObject *pParent=nullptr);
    virtual ~EngineSideChainCompressor() { };
    void setParameters(CSAMPLE _threshold, CSAMPLE _strength,double _attack_time, double _decay_time) {
        // TODO(owilliams): There is a race condition here because the parameters
        // are not updated atomically.  This function should instead take a
        // struct.
        bool _thr,_str,_att,_dec;
        _thr = (m_threshold!=_threshold);
        _str = (m_strength!=_strength);
        _att = (m_attackTime!=_attack_time);
        _dec = (m_decayTime!=_decay_time);
        m_threshold = _threshold;
        m_strength = _strength;
        m_attackTime = _attack_time;
        m_decayTime = _decay_time;
        calculateRates();
        if(_thr) emit thresholdChanged(_threshold);
        if(_str) emit strengthChanged(_strength);
        if(_att) emit attackTimeChanged(_attack_time);
        if(_dec) emit decayTimeChanged(_decay_time);
    }
    void setThreshold(CSAMPLE _threshold) {
      if(m_threshold!=_threshold){
        m_threshold = _threshold;
        calculateRates();
        emit(thresholdChanged(_threshold));
      }
    }
    CSAMPLE threshold() const{return m_threshold;}
    void setStrength(CSAMPLE _strength) {
      if(_strength!=m_strength)
      {
          m_strength = _strength;
          calculateRates();
          emit(strengthChanged(_strength));
      }
    }
    CSAMPLE strength() const{return m_strength;}
    void setAttackTime(double attack_time) {
      if(m_attackTime != attack_time)
      {
        m_attackTime = attack_time;
        calculateRates();
        emit(attackTimeChanged(attack_time));
      }
    }
    double attackTime()const{return m_attackTime;}
    void setDecayTime(double decay_time) {
      if(m_decayTime!=decay_time)
      {
        m_decayTime = decay_time;
        calculateRates();
        emit decayTimeChanged(decay_time);
      }
    }
    double decayTime()const{return m_decayTime;}
    // Before calling processKey on multiple channels, first call clearKeys to
    // clear state from the last round of compressor gain calculation.
    void clearKeys();
    // Every loop, before calling process, first call processKey to feed
    // the compressor the input key signal.  It is safe to call this function
    // multiple times for multiple keys, however they will not be summed together
    // so compression will not be triggered unless at least one buffer would
    // have triggered alone.
    virtual void process(CSAMPLE* pIn, const int iBufferSize);
    // Calculates a new gain value based on the current compression ratio
    // over the given number of frames and whether the current input is above threshold.
    double calculateCompressedGain(int frames);
  private:
    // Update the attack and decay rates.
    void calculateRates();
    // The current ratio the signal is being compressed.  This is the same as m_strength
    // when the compressor is at maximum engagement (not attacking or decaying).
    CSAMPLE m_currentMag;
    double m_compressTargetDb;
    double m_compressRatioDb;
    // True if the input signal is above the threshold.
    bool m_bAboveThreshold;
    // The sample value above which the compressor is triggered.
    CSAMPLE m_threshold;
    // The largest ratio the signal can be compressed.
    CSAMPLE m_strength;
    // The length of time, in frames (samples/2), until maximum compression is reached.
    double m_attackTime;
    // The length of time, in frames, until compression is completely off.
    double m_decayTime;
    // These are the delta compression values per sample based on the strengths and timings.
    CSAMPLE m_attackPerFrame;
    CSAMPLE m_decayPerFrame;
};

#endif
