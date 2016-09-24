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

#include <QScopedPointer>

#include "preferences/usersettings.h"
#include "control/controlproxy.h"
#include "control/controlpushbutton.h"
#include "engine/engineobject.h"
#include "engine/enginechannel.h"

#include "soundio/soundmanagerutil.h"

class EngineBuffer;
class EnginePregain;
class EngineBuffer;
class EngineMaster;
class EngineVuMeter;
class EffectsManager;
class EngineEffectsManager;
class ControlPushButton;

class EngineDeck : public EngineChannel, public AudioDestination {
    Q_OBJECT
  public:
    EngineDeck(QObject *p, const ChannelHandleAndGroup& handle_group, UserSettingsPointer pConfig,
               EngineMaster* pMixingEngine, EffectsManager* pEffectsManager,
               EngineChannel::ChannelOrientation defaultOrientation = CENTER);
    virtual ~EngineDeck();

    void process(CSAMPLE* pOutput, int iBufferSize) override;
    void postProcess(int iBufferSize) override;

    // TODO(XXX) This hack needs to be removed.
    virtual EngineBuffer* getEngineBuffer();

    bool isActive() override;

    // This is called by SoundManager whenever there are new samples from the
    // configured input to be processed. This is run in the callback thread of
    // the soundcard this AudioDestination was registered for! Beware, in the
    // case of multiple soundcards, this method is not re-entrant but it may be
    // concurrent with EngineMaster processing.
    void receiveBuffer(AudioInput input, const CSAMPLE* pBuffer,
                               unsigned int nFrames) override;

    // Called by SoundManager whenever the passthrough input is connected to a
    // soundcard input.
    void onInputConfigured(AudioInput input) override;

    // Called by SoundManager whenever the passthrough input is disconnected
    // from a soundcard input.
    void onInputUnconfigured(AudioInput input) override;

    // Return whether or not passthrough is active
    bool isPassthroughActive() const;

  public slots:
    void slotPassingToggle(double v);

  private slots:
    // Reject all change requests for input configured.
    void slotInputConfiguredChangeRequest(double) {}

  private:
    UserSettingsPointer m_pConfig;
    EngineBuffer* m_pBuffer;
    EnginePregain* m_pPregain;

    // Begin vinyl passthrough fields
    ControlPushButton* m_pPassing;
    const CSAMPLE* volatile m_sampleBuffer;
    bool m_bPassthroughIsActive;
    bool m_bPassthroughWasActive;
};

#endif
