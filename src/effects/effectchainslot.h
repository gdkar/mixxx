#ifndef EFFECTCHAINSLOT_H
#define EFFECTCHAINSLOT_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QSignalMapper>

#include "util.h"
#include "effects/effect.h"
#include "effects/effectslot.h"
#include "effects/effectchain.h"
#include "engine/channelhandle.h"

class ControlObject;
class ControlPushButton;
class EffectChainSlot;
class EffectRack;
typedef QSharedPointer<EffectChainSlot> EffectChainSlotPointer;

class EffectChainSlot : public QObject {
    Q_OBJECT
  public:
    EffectChainSlot(EffectRack* pRack,
                    const QString& group,
                    const unsigned int iChainNumber);
    virtual ~EffectChainSlot();

    // Get the ID of the loaded EffectChain
    QString id() const;

    unsigned int numSlots() const;
    EffectSlotPointer addEffectSlot(const QString& group);
    EffectSlotPointer getEffectSlot(unsigned int slotNumber);

    void loadEffectChain(EffectChainPointer pEffectChain);
    EffectChainPointer getEffectChain() const;

    void registerChannel(const ChannelHandleAndGroup& handle_group);

    double getSuperParameter() const;
    void setSuperParameter(double value);
    void setSuperParameterDefaultValue(double value);

    // Unload the loaded EffectChain.
    void clear();
    unsigned int getChainSlotNumber() const;
    const QString& getGroup() const {return m_group;}
  signals:
    // Indicates that the effect pEffect has been loaded into slotNumber of
    // EffectChainSlot chainNumber. pEffect may be an invalid pointer, which
    // indicates that a previously loaded effect was removed from the slot.
    void effectLoaded(EffectPointer pEffect, unsigned int chainNumber,
                      unsigned int slotNumber);

    // Indicates that the given EffectChain was loaded into this
    // EffectChainSlot
    void effectChainLoaded(EffectChainPointer pEffectChain);

    // Signal that whoever is in charge of this EffectChainSlot should load the
    // next EffectChain into it.
    void nextChain(unsigned int iChainSlotNumber,
                   EffectChainPointer pEffectChain);

    // Signal that whoever is in charge of this EffectChainSlot should load the
    // previous EffectChain into it.
    void prevChain(unsigned int iChainSlotNumber,
                   EffectChainPointer pEffectChain);

    // Signal that whoever is in charge of this EffectChainSlot should clear
    // this EffectChain (by removing the chain from this EffectChainSlot).
    void clearChain(unsigned int iChainNumber, EffectChainPointer pEffectChain);

    // Signal that whoever is in charge of this EffectChainSlot should load the
    // next Effect into the specified EffectSlot.
    void nextEffect(unsigned int iChainSlotNumber,
                    unsigned int iEffectSlotNumber,
                    EffectPointer pEffect);

    // Signal that whoever is in charge of this EffectChainSlot should load the
    // previous Effect into the specified EffectSlot.
    void prevEffect(unsigned int iChainSlotNumber,
                    unsigned int iEffectSlotNumber,
                    EffectPointer pEffect);

    // Signal that indicates that the EffectChainSlot has been updated.
    void updated();


  private slots:
    void slotChainEffectsChanged(bool shouldEmit=true);
    void slotChainNameChanged(const QString& name);
    void slotChainSuperParameterChanged(double parameter);
    void slotChainEnabledChanged(bool enabled);
    void slotChainMixChanged(double mix);
    void slotChainInsertionTypeChanged(EffectChain::InsertionType type);
    void slotChainChannelStatusChanged(const QString& group, bool enabled);

    void slotEffectLoaded(EffectPointer pEffect, unsigned int slotNumber);
    // Clears the effect in the given position in the loaded EffectChain.
    void slotClearEffect(unsigned int iEffectSlotNumber);

    void slotControlClear(double v);
    void slotControlNumEffects(double v);
    void slotControlNumEffectSlots(double v);
    void slotControlChainLoaded(double v);
    void slotControlChainEnabled(double v);
    void slotControlChainMix(double v);
    void slotControlChainSuperParameter(double v);
    void slotControlChainInsertionType(double v);
    void slotControlChainSelector(double v);
    void slotControlChainNextPreset(double v);
    void slotControlChainPrevPreset(double v);
    void slotChannelStatusChanged(const QString& group);
  private:
    QString debugString() const;
    const unsigned int m_iChainSlotNumber;
    const QString m_group;
    EffectRack* m_pEffectRack                       = nullptr;
    EffectChainPointer m_pEffectChain{nullptr};
    ControlPushButton* m_pControlClear              = nullptr;
    ControlObject* m_pControlNumEffects             = nullptr;
    ControlObject* m_pControlNumEffectSlots         = nullptr;
    ControlObject* m_pControlChainLoaded            = nullptr;
    ControlPushButton* m_pControlChainEnabled       = nullptr;
    ControlObject* m_pControlChainMix               = nullptr;
    ControlObject* m_pControlChainSuperParameter    = nullptr;
    ControlPushButton* m_pControlChainInsertionType = nullptr;
    ControlObject* m_pControlChainSelector       = nullptr;
    ControlPushButton* m_pControlChainNextPreset = nullptr;
    ControlPushButton* m_pControlChainPrevPreset = nullptr;
    class ChannelInfo {
        public:
        // Takes ownership of pEnabled.
        ChannelInfo(const ChannelHandleAndGroup& handle_group, ControlObject* pEnabled);
        ~ChannelInfo();
        ChannelHandleAndGroup handle_group;
        ControlObject* pEnabled = nullptr;
    };
    QMap<QString, ChannelInfo*> m_channelInfoByName;
    QList<EffectSlotPointer> m_slots;
    QSignalMapper m_channelStatusMapper;
    DISALLOW_COPY_AND_ASSIGN(EffectChainSlot);
};


#endif /* EFFECTCHAINSLOT_H */
