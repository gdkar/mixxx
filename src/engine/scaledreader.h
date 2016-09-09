#ifndef SCALEDREADER_H
#define SCALEDREADER_H

#include <QtGlobal>
#include <QObject>
#include <QThread>
#include <array>
#include <deque>
#include <algorithm>
#include <utility>
#include <memory>
#include <numeric>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <rubberband/RubberBandStretcher.h>
#include "sources/soundsourceproxy.h"
#include "util/audiosignal.h"


using RubberBand::RubberBandStretcher;
/**
  *@author Tue & Ken Haste Andersen
  */

class ScaledReader : public QObject {
    Q_OBJECT
    Q_PROPERTY(SINT   sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged);
    Q_PROPERTY(double pitch READ pitchRatio WRITE setPitchRatio NOTIFY pitchRatioChanged);
    Q_PROPERTY(double tempo READ tempoRatio WRITE setTempoRatio NOTIFY tempoRatioChanged);
    Q_PROPERTY(double position READ position WRITE setPosition NOTIFY positionChanged);
    Q_PROPERTY(bool   reverse READ reverse NOTIFY reverseChanged);
    Q_PROPERTY(mixxx::AudioSourcePointer audioSource READ audioSource WRITE setAudioSource NOTIFY audioSourceChanged);
  protected:
    static const SINT kForwardBlockSize = 1024u;
    static const SINT kReverseBlockSize = 16384u;
    SINT block_size() const;
    bool decode_next();
    bool retrieve();
    SINT retrieve_drop(SINT count);

    mixxx::AudioSourcePointer   m_audioSource{};
    SINT                        m_src_pos{};

    std::vector<CSAMPLE>        m_decode_buf{};
    std::deque<CSAMPLE>         m_output_buf{};
    std::vector<CSAMPLE>        m_planar_storage{};
    std::vector<CSAMPLE*>       m_planar_buf{};
    std::vector<CSAMPLE>        m_retrieve_storage{4096};
    std::vector<CSAMPLE*>       m_retrieve_buf{m_retrieve_storage.data(),m_retrieve_storage.data() + 2048};

    SINT   m_sampleRate{44100};
    double m_baseRate  {1.0};
    double m_tempoRatio{1.0};
    double m_pitchRatio{1.0};
    double m_position{};
    std::unique_ptr<RubberBand::RubberBandStretcher> m_rb{};

  public:
    Q_INVOKABLE ScaledReader( QObject *pParent);
    virtual ~ScaledReader();

    // Sets the scaling parameters.
    // * The base rate (ratio of track sample rate to output sample rate).
    // * The tempoRatio describes the tempo change in fraction of
    //   original tempo. Put another way, it is the ratio of track seconds to
    //   real second. For example, a tempo of 1.0 is no change. A
    //   tempo of 2 is a 2x speedup (2 track seconds pass for every 1
    //   real second).
    // * The pitchRatio describes the pitch adjustment in fraction of
    //   the original pitch. For example, a pitch adjustment of 1.0 is no change and a
    //   pitch adjustment of 2.0 is a full octave shift up.
    //
    // If parameter settings are outside of acceptable limits, each setting will
    // be set to the value it was clamped to.
    Q_INVOKABLE virtual void setScaleParameters(double pitch_ratio,
                                    double tempo_ratio,
                                    double pos);
    // Called from EngineBuffer when seeking, to ensure the buffers are flushed */
    Q_INVOKABLE virtual void clear() ;
    // Scale buffer
    // Returns the number of frames that have bean read from the unscaled
    // input buffer The number of frames copied to the output buffer is always
    // an integer value, while the number of frames read from the unscaled
    // input buffer might be partial number!
    // The size of the output buffer is given in samples, i.e. twice the number
    // of frames for an interleaved stereo signal.
    virtual double tempoRatio() const { return m_tempoRatio;}
    virtual double pitchRatio() const { return m_pitchRatio;}
    virtual SINT   sampleRate() const { return m_sampleRate;}
    virtual bool   reverse()    const { return tempoRatio() < 0; }
    virtual double position()   const { return m_position;}
    virtual mixxx::AudioSourcePointer audioSource() const;
    Q_INVOKABLE virtual SINT readSampleFrames(SINT count, CSAMPLE *data);
    Q_INVOKABLE virtual SINT skipSampleFrames(SINT count);
    virtual void setAudioSource(mixxx::AudioSourcePointer _src);
    virtual void setSampleRate(SINT _rate);
    virtual void setTempoRatio(double);
    virtual void setPitchRatio(double);
    virtual void setPosition  (double);
  signals:
    void tempoRatioChanged(double);
    void pitchRatioChanged(double);
    void positionChanged  (double);
    void sampleRateChanged(SINT);
    void reverseChanged(bool);
    void audioSourceChanged(mixxx::AudioSourcePointer);

};

#endif
