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

_Pragma("once")
#include "engine/engineobject.h"
#include "engine/channelhandle.h"
#include "configobject.h"

class ControlObject;
class EngineBuffer;
class EnginePregain;
class EngineFilterBlock;
class EngineVuMeter;
class ControlPushButton;

class EngineChannel : public EngineObject {
    Q_OBJECT
  public:
    enum ChannelOrientation {
        LEFT = 0,
        CENTER,
        RIGHT,
    };
    Q_ENUM(ChannelOrientation);
    EngineChannel(const ChannelHandleAndGroup& handle_group, ChannelOrientation defaultOrientation ,QObject *pParent);
    virtual ~EngineChannel();
    virtual ChannelOrientation getOrientation() const;
    virtual ChannelHandle getHandle() const;
    virtual bool isActive() = 0;
    virtual void setPfl(bool enabled);
    virtual bool isPflEnabled() const;
    virtual void setMaster(bool enabled);
    virtual bool isMasterEnabled() const;
    virtual void setTalkover(bool enabled);
    virtual bool isTalkoverEnabled() const;
    virtual void process(CSAMPLE* pOut, const int iBufferSize) = 0;
    virtual void postProcess(const int iBuffersize) = 0;
    // TODO(XXX) This hack needs to be removed.
    virtual EngineBuffer* getEngineBuffer() const;
  private slots:
    virtual void slotOrientationLeft(double v);
    virtual void slotOrientationRight(double v);
    virtual void slotOrientationCenter(double v);
  private:
    const ChannelHandleAndGroup m_handle_group;
    ControlPushButton* m_pMaster = nullptr;
    ControlPushButton* m_pPFL    = nullptr;
    ControlPushButton* m_pOrientation = nullptr;
    ControlPushButton* m_pOrientationLeft = nullptr;
    ControlPushButton* m_pOrientationRight = nullptr;
    ControlPushButton* m_pOrientationCenter = nullptr;
    ControlPushButton* m_pTalkover = nullptr;
};
