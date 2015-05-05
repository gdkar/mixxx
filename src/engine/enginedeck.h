/***************************************************************************
                          enginedeck.h  -  description
                             -------------------
    begin                : Sun Apr 28 2002
    copyright            : (C) 2002 by
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ENGINEDECK_H
#define ENGINEDECK_H

#include "configobject.h"
#include "control/controlobjectslave.h"
#include "control/controlpushbutton.h"
#include "engine/engineobject.h"
#include "engine/enginechannel.h"

#include "soundmanagerutil.h"

class EngineBuffer;
class EnginePregain;
class EngineBuffer;
class EngineMaster;
class EngineVuMeter;
class EffectsManager;
class EngineEffectsManager;
class ControlPushButton;

class EngineDeck : public EngineChannel, public AudioSink {
    Q_OBJECT
  public:
    explicit EngineDeck(const ChannelHandleAndGroup& handle_group, ConfigObject<ConfigValue>* pConfig,
               EngineMaster* pMixingEngine, EffectsManager* pEffectsManager,
               EngineChannel::ChannelOrientation defaultOrientation = CENTER, QObject *pParent=0);
    virtual ~EngineDeck();

    virtual void process(CSAMPLE* pOutput, const int iBufferSize);
    virtual void postProcess(const int iBufferSize);

    // TODO(XXX) This hack needs to be removed.
    virtual EngineBuffer* getEngineBuffer();

    virtual bool isActive();

    // This is called by SoundManager whenever there are new samples from the
    // configured input to be processed. This is run in the callback thread of
    // the soundcard this AudioSink was registered for! Beware, in the
    // case of multiple soundcards, this method is not re-entrant but it may be
    // concurrent with EngineMaster processing.
    virtual void receiveBuffer(AudioInput input, const CSAMPLE* pBuffer,
                               unsigned int nFrames);

    // Called by SoundManager whenever the passthrough input is connected to a
    // soundcard input.
    virtual void onInputConfigured(AudioInput input);

    // Called by SoundManager whenever the passthrough input is disconnected
    // from a soundcard input.
    virtual void onInputUnconfigured(AudioInput input);

    // Return whether or not passthrough is active
    bool isPassthroughActive() const;

  public slots:
    void onPassingToggle(double v);

  private:
    ConfigObject<ConfigValue>*         m_pConfig;
    QSharedPointer<EngineBuffer>       m_pBuffer;
    QSharedPointer<EnginePregain>      m_pPregain;
    QSharedPointer<EngineVuMeter>      m_pVUMeter;
    EngineEffectsManager*              m_pEngineEffectsManager;
    QSharedPointer<ControlObjectSlave> m_pSampleRate;

    // Begin vinyl passthrough fields
    QSharedPointer<ControlPushButton>  m_pPassing;
    const CSAMPLE* volatile            m_sampleBuffer;
    bool                               m_bPassthroughIsActive;
    bool                               m_bPassthroughWasActive;
};
Q_DECLARE_TYPEINFO(EngineDeck,Q_COMPLEX_TYPE);
#endif
