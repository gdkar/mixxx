// engineaux.h
// created 4/8/2011 by Bill Good (bkgood@gmail.com)
// unapologetically copied from enginemicrophone.h from RJ

#ifndef ENGINEAUX_H
#define ENGINEAUX_H

#include "engine/enginechannel.h"
#include "engine/enginevumeter.h"
#include "soundmanagerutil.h"

class EffectsManager;
class EngineEffectsManager;
class ControlAudioTaperPot;
class ControlObjectSlave;
// EngineAux is an EngineChannel that implements a mixing source whose
// samples are fed directly from the SoundManager
class EngineAux : public EngineChannel, public AudioDestination {
    Q_OBJECT
  public:
    EngineAux(const ChannelHandleAndGroup& handle_group, EffectsManager* pEffectsManager);
    virtual ~EngineAux();

    bool isActive();

    // Called by EngineMaster whenever is requesting a new buffer of audio.
    virtual void process(CSAMPLE* pOutput, const int iBufferSize);
    virtual void postProcess(const int iBufferSize) { Q_UNUSED(iBufferSize) }

    // This is called by SoundManager whenever there are new samples from the
    // configured input to be processed. This is run in the callback thread of
    // the soundcard this AudioDestination was registered for! Beware, in the
    // case of multiple soundcards, this method is not re-entrant but it may be
    // concurrent with EngineMaster processing.
    virtual void receiveBuffer(AudioInput input, const CSAMPLE* pBuffer,unsigned int nFrames);
    // Called by SoundManager whenever the aux input is connected to a
    // soundcard input.
    virtual void onInputConfigured(AudioInput input);
    // Called by SoundManager whenever the aux input is disconnected from
    // a soundcard input.
    virtual void onInputUnconfigured(AudioInput input);
  private:
    EngineEffectsManager* m_pEngineEffectsManager = nullptr;
    EngineVuMeter m_vuMeter;
    ControlObject* m_pEnabled = nullptr;
    ControlAudioTaperPot* m_pPregain = nullptr;
    ControlObjectSlave* m_pSampleRate = nullptr;
    const CSAMPLE* volatile m_sampleBuffer = nullptr;
    bool m_wasActive = false;
};

#endif // ENGINEAUX_H
