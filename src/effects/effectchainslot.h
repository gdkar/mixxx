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
    EffectSlotPointer getEffectSlot(unsigned int onNumber);

    void loadEffectChain(EffectChainPointer pEffectChain);
    EffectChainPointer getEffectChain() const;

    void registerChannel(const ChannelHandleAndGroup& handle_group);

    double getSuperParameter() const;
    void setSuperParameter(double value);
    void setSuperParameterDefaultValue(double value);

    // Unload the loaded EffectChain.
    void clear();

    unsigned int getChainSlotNumber() const;

    const QString& getGroup() const {
        return m_group;
    }

  signals:
    // Indicates that the effect pEffect has been loaded into onNumber of
    // EffectChainSlot chainNumber. pEffect may be an invalid pointer, which
    // indicates that a previously loaded effect was removed from the slot.
    void effectLoaded(EffectPointer pEffect, unsigned int chainNumber,
                      unsigned int onNumber);

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
    void onChainEffectsChanged(bool shouldEmit=true);
    void onChainNameChanged(const QString& name);
    void onChainSuperParameterChanged(double parameter);
    void onChainEnabledChanged(bool enabled);
    void onChainMixChanged(double mix);
    void onChainInsertionTypeChanged(EffectChain::InsertionType type);
    void onChainChannelStatusChanged(const QString& group, bool enabled);

    void onEffectLoaded(EffectPointer pEffect, unsigned int onNumber);
    // Clears the effect in the given position in the loaded EffectChain.
    void onClearEffect(unsigned int iEffectSlotNumber);

    void onControlClear(double v);
    void onControlNumEffects(double v);
    void onControlNumEffectSlots(double v);
    void onControlChainLoaded(double v);
    void onControlChainEnabled(double v);
    void onControlChainMix(double v);
    void onControlChainSuperParameter(double v);
    void onControlChainInsertionType(double v);
    void onControlChainSelector(double v);
    void onControlChainNextPreset(double v);
    void onControlChainPrevPreset(double v);
    void onChannelStatusChanged(const QString& group);

  private:
    QString debugString() const {
        return QString("EffectChainSlot(%1)").arg(m_group);
    }

    const unsigned int m_iChainSlotNumber;
    const QString m_group;
    EffectRack* m_pEffectRack;

    EffectChainPointer m_pEffectChain;

    ControlPushButton* m_pControlClear;
    ControlObject* m_pControlNumEffects;
    ControlObject* m_pControlNumEffectSlots;
    ControlObject* m_pControlChainLoaded;
    ControlPushButton* m_pControlChainEnabled;
    ControlObject* m_pControlChainMix;
    ControlObject* m_pControlChainSuperParameter;
    ControlPushButton* m_pControlChainInsertionType;
    ControlObject* m_pControlChainSelector;
    ControlPushButton* m_pControlChainNextPreset;
    ControlPushButton* m_pControlChainPrevPreset;

    struct ChannelInfo {
        // Takes ownership of pEnabled.
        ChannelInfo(const ChannelHandleAndGroup& handle_group, ControlObject* pEnabled)
                : handle_group(handle_group),
                  pEnabled(pEnabled) {

        }
        ~ChannelInfo() {
            delete pEnabled;
        }
        ChannelHandleAndGroup handle_group;
        ControlObject* pEnabled;
    };
    QMap<QString, ChannelInfo*> m_channelInfoByName;

    QList<EffectSlotPointer> m_slots;
    QSignalMapper m_channelStatusMapper;

    DISALLOW_COPY_AND_ASSIGN(EffectChainSlot);
};


#endif /* EFFECTCHAINSLOT_H */
