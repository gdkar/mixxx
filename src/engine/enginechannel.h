/***************************************************************************
                          enginechannel.h  -  description
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

#ifndef ENGINECHANNEL_H
#define ENGINECHANNEL_H

#include "engine/engineobject.h"
#include "engine/channelhandle.h"
#include "preferences/usersettings.h"
#include "soundio/soundmanagerutil.h"

class EffectsManager;
class ControlObject;
class ControlProxy;
class EngineBuffer;
class EnginePregain;
class EngineFilterBlock;
class EngineEffectsManager;
class EngineVuMeter;
class ControlPushButton;

class EngineChannel : public EngineObject {
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
    Q_PROPERTY(bool input_configure READ isActive NOTIFY activeChanged)
    Q_PROPERTY(bool pflEnabled READ isPflEnabled WRITE setPfl NOTIFY pflEnabledChanged)
    Q_PROPERTY(bool masterEnabled READ isMasterEnabled WRITE setMaster NOTIFY masterEnabledChanged)
    Q_PROPERTY(bool talkoverEnabled READ isTalkoverEnabled WRITE setTalkover NOTIFY talkoverEnabledChanged)
    Q_PROPERTY(ChannelOrientation orientation
        READ getOrientation
        WRITE setOrientation
        NOTIFY orientationChanged);
  public:
    enum ChannelOrientation {
        LEFT = 0,
        CENTER,
        RIGHT,
    };
    Q_ENUM(ChannelOrientation);

    EngineChannel(QObject *p, const ChannelHandleAndGroup& handle_group,
                  ChannelOrientation defaultOrientation = CENTER, EffectsManager *pEffectsManager = nullptr);
    virtual ~EngineChannel();
    virtual void setOrientation(ChannelOrientation o);

    virtual ChannelOrientation getOrientation() const;

    const ChannelHandle& getHandle() const {
        return m_group.handle();
    }

    virtual const QString& getGroup() const {
        return m_group.name();
    }

    virtual bool isActive() = 0;
    void setPfl(bool enabled);
    virtual bool isPflEnabled() const;
    void setMaster(bool enabled);
    virtual bool isMasterEnabled() const;
    void setTalkover(bool enabled);
    virtual bool isTalkoverEnabled() const;

    virtual void process(CSAMPLE* pOut, const int iBufferSize) = 0;
    virtual void postProcess(const int iBuffersize) = 0;

    // TODO(XXX) This hack needs to be removed.
    virtual EngineBuffer* getEngineBuffer() {
        return NULL;
    }
  public slots:
    // Called by SoundManager whenever the passthrough input is connected to a
    // soundcard input.
    virtual void onInputConfigured(AudioInput input) = 0;

    // Called by SoundManager whenever the passthrough input is disconnected
    // from a soundcard input.
    virtual void onInputUnconfigured(AudioInput input) = 0;

    // Return whether or not passthrough is active

  signals:
    void activeChanged(bool);
    void pflEnabledChanged(bool);
    void masterEnabledChanged(bool);
    void talkoverEnabledChanged(bool);
    void orientationChanged();
  private slots:
    void slotOrientationLeft(double v);
    void slotOrientationRight(double v);
    void slotOrientationCenter(double v);
  protected:
    EngineEffectsManager* m_pEngineEffectsManager;
    ControlObject* m_pInputConfigured;
    ControlProxy* m_pSampleRate;
    EngineVuMeter* m_pVUMeter;
    bool m_wasActive;

  private:
    const ChannelHandleAndGroup m_group;
    ControlPushButton* m_pMaster;
    ControlPushButton* m_pPFL;
    ControlPushButton* m_pOrientation;
    ControlPushButton* m_pOrientationLeft;
    ControlPushButton* m_pOrientationRight;
    ControlPushButton* m_pOrientationCenter;
    ControlPushButton* m_pTalkover;
};

#endif
