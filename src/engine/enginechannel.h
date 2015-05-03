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
#include "control/statevariable.h"
#include "configobject.h"

class ControlObject;
class EngineBuffer;
class EnginePregain;
class EngineFilterBlock;
class EngineVuMeter;
class ControlPushButton;

class EngineChannel : public EngineObject {
    Q_OBJECT
    Q_ENUMS(ChannelOrientation);
    Q_PROPERTY(QString group READ getGroup NOTIFY groupChanged);
    Q_PROPERTY(bool pflEnabled READ isPflEnabled WRITE setPfl NOTIFY pflEnabledChanged);
    Q_PROPERTY(bool masterEnabled READ isMasterEnabled WRITE setMaster NOTIFY masterEnabledChanged);
    Q_PROPERTY(bool talkoverEnabled READ isTalkoverEnabled  WRITE setTalkover NOTIFY talkoverEnabledChanged);
  public:
    enum ChannelOrientation {
        LEFT = 0,
        CENTER,
        RIGHT,
    };
    EngineChannel(const ChannelHandleAndGroup& handle_group,ChannelOrientation defaultOrientation = CENTER, QObject *pParent=0);
    virtual ~EngineChannel();
  public slots:
    virtual void setOrientation(ChannelOrientation o);
    virtual ChannelOrientation getOrientation() const;
    inline const ChannelHandle& getHandle() const {return m_group.handle();}
    virtual const QString& getGroup() const {return m_group.name();}
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
    virtual EngineBuffer* getEngineBuffer() {return NULL;}
  signals:
    void groupChanged(const QString &);
    void orientationChanged(ChannelOrientation);
    void pflEnabledChanged(bool);
    void masterEnabledChanged(bool);
    void talkoverEnabledChanged(bool);
  private slots:
    void onOrientationLeft(double v);
    void onOrientationRight(double v);
    void onOrientationCenter(double v);
  private:
    const ChannelHandleAndGroup m_group;
    ControlPushButton* m_pMaster;
    ControlPushButton* m_pPFL;
//    StateVar<enum ChannelOrientation>  m_orientation;
    ControlPushButton* m_pOrientation;
    ControlPushButton* m_pOrientationLeft;
    ControlPushButton* m_pOrientationRight;
    ControlPushButton* m_pOrientationCenter;
    ControlPushButton* m_pTalkover;
};
Q_DECLARE_METATYPE(EngineChannel::ChannelOrientation);
Q_DECLARE_TYPEINFO(EngineChannel::ChannelOrientation,Q_PRIMITIVE_TYPE);
#endif
