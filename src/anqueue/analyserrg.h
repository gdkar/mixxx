/*
 * analyserrg.h
 *
 *  Created on: 13/ott/2010
 *      Author: Vittorio Colao
 *       */

#ifndef ANALYSERRG_H_
#define ANALYSERRG_H_

#include "analyser.h"
#include "configobject.h"

class ReplayGain;

class AnalyserGain : public Analyser {
  public:
    AnalyserGain(ConfigObject<ConfigValue> *_config);
    virtual ~AnalyserGain();

    bool initialise(TrackPointer tio, int sampleRate, int totalSamples);
    bool loadStored(TrackPointer tio) const;
    void process(const CSAMPLE *pIn, const int iLen);
    void cleanup(TrackPointer tio);
    void finalise(TrackPointer tio);
  private:
    bool m_bStepControl;
    ConfigObject<ConfigValue> *m_pConfigReplayGain = nullptr;
    CSAMPLE* m_pLeftTempBuffer = nullptr;
    CSAMPLE* m_pRightTempBuffer = nullptr;
    ReplayGain *m_pReplayGain = nullptr;
    int m_iBufferSize = 0;
};

#endif /* ANALYSERRG_H_ */
