// engineaux.h
// created 4/8/2011 by Bill Good (bkgood@gmail.com)
// unapologetically copied from enginemicrophone.h from RJ

#ifndef ENGINEAUX_H
#define ENGINEAUX_H

#include <QScopedPointer>

#include "control/controlproxy.h"
#include "control/controlpushbutton.h"
#include "engine/enginechannel.h"
#include "engine/enginevumeter.h"
#include "soundio/soundmanagerutil.h"

class EffectsManager;
class EngineEffectsManager;
class ControlAudioTaperPot;

// EngineAux is an EngineChannel that implements a mixing source whose
// samples are fed directly from the SoundManager
class EngineAux : public EngineChannel, public AudioDestination {
    Q_OBJECT
  public:
    EngineAux(QObject *p, const ChannelHandleAndGroup& handle_group, EffectsManager* pEffectsManager);
    virtual ~EngineAux();

    bool isActive() override;

    // Called by EngineMaster whenever is requesting a new buffer of audio.
    void process(CSAMPLE* pOutput, const int iBufferSize) override;
    void postProcess(const int iBufferSize) override { Q_UNUSED(iBufferSize) }

    // This is called by SoundManager whenever there are new samples from the
    // configured input to be processed. This is run in the callback thread of
    // the soundcard this AudioDestination was registered for! Beware, in the
    // case of multiple soundcards, this method is not re-entrant but it may be
    // concurrent with EngineMaster processing.
    void receiveBuffer(AudioInput input, const CSAMPLE* pBuffer,
                               unsigned int nFrames) override;

    // Called by SoundManager whenever the aux input is connected to a
    // soundcard input.
    void onInputConfigured(AudioInput input) override;

    // Called by SoundManager whenever the aux input is disconnected from
    // a soundcard input.
    void onInputUnconfigured(AudioInput input) override;

  private slots:
    // Reject all change requests for input configured.
    void slotInputConfiguredChangeRequest(double) {}

  private:
    ControlAudioTaperPot* m_pPregain;
    const CSAMPLE* volatile m_sampleBuffer;
};

#endif // ENGINEAUX_H
