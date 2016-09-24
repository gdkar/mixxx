// enginemicrophone.h
// created 3/16/2011 by RJ Ryan (rryan@mit.edu)

#ifndef ENGINEMICROPHONE_H
#define ENGINEMICROPHONE_H

#include <QScopedPointer>

#include "control/controlproxy.h"
#include "control/controlpushbutton.h"
#include "engine/enginechannel.h"
#include "engine/enginevumeter.h"

#include "soundio/soundmanagerutil.h"

class EffectsManager;
class EngineEffectsManager;
class ControlAudioTaperPot;

// EngineMicrophone is an EngineChannel that implements a mixing source whose
// samples are fed directly from the SoundManager
class EngineMicrophone : public EngineChannel, public AudioDestination {
    Q_OBJECT
  public:
    EngineMicrophone(QObject *p, const ChannelHandleAndGroup& handle_group,
                     EffectsManager* pEffectsManager);
    virtual ~EngineMicrophone();

    bool isActive() override;

    // Called by EngineMaster whenever is requesting a new buffer of audio.
    void process(CSAMPLE* pOutput, int iBufferSize) override;
    void postProcess(int iBufferSize) override { Q_UNUSED(iBufferSize) }

    // This is called by SoundManager whenever there are new samples from the
    // configured input to be processed. This is run in the callback thread of
    // the soundcard this AudioDestination was registered for! Beware, in the
    // case of multiple soundcards, this method is not re-entrant but it may be
    // concurrent with EngineMaster processing.
    void receiveBuffer(AudioInput input, const CSAMPLE* pBuffer,
                               unsigned int iNumSamples) override;

    // Called by SoundManager whenever the microphone input is connected to a
    // soundcard input.
    void onInputConfigured(AudioInput input) override;

    // Called by SoundManager whenever the microphone input is disconnected from
    // a soundcard input.
    void onInputUnconfigured(AudioInput input) override;

    bool isSolo();
    double getSoloDamping();

  private slots:
    // Reject all change requests for input configured.
    void slotInputConfiguredChangeRequest(double) {}

  private:
    ControlAudioTaperPot* m_pPregain;
    const CSAMPLE* volatile m_sampleBuffer;
};

#endif /* ENGINEMICROPHONE_H */
