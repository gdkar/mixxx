#include <QtDebug>
#include <functional>
#include <tuple>
#include <utility>

#include "engine/keycontrol.h"

#include "control/controlobject.h"
#include "control/controlpushbutton.h"
#include "control/controlpotmeter.h"
#include "engine/enginebuffer.h"
#include "track/keyutils.h"

static const double kLockOriginalKey = 0;
static const double kLockCurrentKey = 1;

KeyControl::KeyControl(QString group,
                       UserSettingsPointer pConfig)
        : EngineControl(group, pConfig) {
    m_pitchRateInfo.pitchRatio = 1.0;
    m_pitchRateInfo.tempoRatio = 1.0;
    m_pitchRateInfo.pitchTweakRatio = 1.0;
    m_pitchRateInfo.keylock = false;

    // pitch is the distance to the original pitch in semitones
    // knob in semitones; 9.4 ct per midi step allowOutOfBounds = true;
    auto pot = new ControlPotmeter(ConfigKey(group, "pitch"),this, -6.0, 6.0, true);
    m_pPitch = pot;
    // Course adjust by full semitone steps.
    pot->setStepCount(12);
    // Fine adjust with semitone / 10 = 10 ct;.
    pot->setSmallStepCount(120);
    connect(m_pPitch, SIGNAL(valueChanged(double)),
            this, SLOT(slotPitchChanged(double)),
            Qt::AutoConnection);

    // pitch_adjust is the distance to the linear pitch in semitones
    // set by the speed slider or to the locked key.
    // pitch_adjust knob in semitones; 4.7 ct per midi step; allowOutOfBounds = true;
    m_pPitchAdjust = pot = new ControlPotmeter(ConfigKey(group, "pitch_adjust"),this, -3.0, 3.0, true);
    // Course adjust by full semitone steps.
    pot->setStepCount(6);
    // Fine adjust with semitone / 10 = 10 ct;.
    pot->setSmallStepCount(60);
    connect(m_pPitchAdjust, SIGNAL(valueChanged(double)),
            this, SLOT(slotPitchAdjustChanged(double)),
            Qt::AutoConnection);

    m_pButtonSyncKey = new ControlObject(ConfigKey(group, "sync_key"),this);
    connect(m_pButtonSyncKey, SIGNAL(valueChanged(double)),
            this, SLOT(slotSyncKey(double)),
            Qt::AutoConnection);

    m_pButtonResetKey = new ControlObject(ConfigKey(group, "reset_key"),this);
    connect(m_pButtonResetKey, SIGNAL(valueChanged(double)),
            this, SLOT(slotResetKey(double)),
            Qt::AutoConnection);

    m_pFileKey = new ControlObject(ConfigKey(group, "file_key"),this);
    connect(m_pFileKey, SIGNAL(valueChanged(double)),
            this, SLOT(slotFileKeyChanged(double)),
            Qt::AutoConnection);

    m_pEngineKey = new ControlObject(ConfigKey(group, "key"),this);
    connect(m_pEngineKey, SIGNAL(valueChanged(double)),
            this, SLOT(slotSetEngineKey(double)),
            Qt::AutoConnection);

    m_pEngineKeyDistance = pot = new ControlPotmeter(ConfigKey(group, "visual_key_distance"),this,-0.5, 0.5);
    connect(m_pEngineKeyDistance, SIGNAL(valueChanged(double)),
            this, SLOT(slotSetEngineKeyDistance(double)),
            Qt::AutoConnection);
    auto button = new ControlPushButton(ConfigKey(group, "keylockMode"),this);
    m_keylockMode = button;
    button->setButtonMode(ControlPushButton::TOGGLE);

    // In case of vinyl control "rate" is a filtered mean value for display
    m_pRateSlider = new ControlObject(ConfigKey(group, "rate"),this);
    connect(m_pRateSlider, SIGNAL(valueChanged(double)),this, SLOT(slotRateChanged()),Qt::AutoConnection);
    connect(m_pRateSlider, SIGNAL(valueChangedFromEngine(double)),this, SLOT(slotRateChanged()),Qt::AutoConnection);

    m_pRateRange = new ControlObject(ConfigKey(group, "rateRange"),this);
    connect(m_pRateRange, SIGNAL(valueChanged(double)),
            this, SLOT(slotRateChanged()),
            Qt::AutoConnection);
    connect(m_pRateRange, SIGNAL(valueChangedFromEngine(double)),
            this, SLOT(slotRateChanged()),
            Qt::AutoConnection);

    m_pRateDir = new ControlObject(ConfigKey(group, "rate_dir"),this);
    connect(m_pRateDir, SIGNAL(valueChanged(double)),
            this, SLOT(slotRateChanged()),
            Qt::AutoConnection);
    connect(m_pRateDir, SIGNAL(valueChangedFromEngine(double)),
            this, SLOT(slotRateChanged()),
            Qt::AutoConnection);

    m_pVCEnabled = new ControlObject(ConfigKey(group, "vinylcontrol_enabled"),this);
    if (m_pVCEnabled) {
        connect(m_pVCEnabled, SIGNAL(valueChanged(double)),
                this, SLOT(slotRateChanged()),
                Qt::AutoConnection);
        connect(m_pVCEnabled, SIGNAL(valueChangedFromEngine(double)),
                this, SLOT(slotRateChanged()),
                Qt::AutoConnection);
    }

    m_pVCRate = new ControlObject(ConfigKey(group, "vinylcontrol_rate"),this);
    if (m_pVCRate) {
        connect(m_pVCRate, SIGNAL(valueChanged(double)),
                this, SLOT(slotRateChanged()),
                Qt::AutoConnection);
        connect(m_pVCRate, SIGNAL(valueChangedFromEngine(double)),
                this, SLOT(slotRateChanged()),
                Qt::AutoConnection);
    }

    m_pKeylock = new ControlObject(ConfigKey(group, "keylock"),this);
    connect(m_pKeylock, SIGNAL(valueChanged(double)),
            this, SLOT(slotRateChanged()),
            Qt::AutoConnection);
    connect(m_pKeylock, SIGNAL(valueChangedFromEngine(double)),
            this, SLOT(slotRateChanged()),
            Qt::AutoConnection);
}

KeyControl::~KeyControl()
{
    delete m_pPitch;
    delete m_pPitchAdjust;
    delete m_pButtonSyncKey;
    delete m_pButtonResetKey;
    delete m_pFileKey;
    delete m_pEngineKey;
    delete m_pEngineKeyDistance;
    delete m_keylockMode;
}

KeyControl::PitchTempoRatio KeyControl::getPitchTempoRatio()
{
    // TODO(XXX) remove code duplication by adding this
    // "Update pending" atomic flag to the ControlObject API
    if (m_updatePitchRequest.exchange(false))
        updatePitch();
    if (m_updatePitchAdjustRequest.exchange(false))
        updatePitchAdjust();
    if (m_updateRateRequest.exchange(false))
        updateRate();
    return m_pitchRateInfo;
}

double KeyControl::getKey()
{
    return m_pEngineKey->get();
}

void KeyControl::slotRateChanged()
{
    m_updateRateRequest = 1;
    updateRate();
}

void KeyControl::updateRate()
{
    //qDebug() << "KeyControl::slotRateChanged 1" << m_pitchRateInfo.pitchRatio;

    // If rate is not 1.0 then we have to try and calculate the octave change
    // caused by it.

    if(m_pVCEnabled && m_pVCEnabled->toBool()) {
        m_pitchRateInfo.tempoRatio = m_pVCRate->get();
    } else {
        m_pitchRateInfo.tempoRatio = 1.0 + m_pRateDir->get() * m_pRateRange->get() * m_pRateSlider->get();
    }

    if (m_pitchRateInfo.tempoRatio == 0) {
        // no transport, no pitch
        // so we can skip pitch calculation
        return;
    }


    // |-----------------------|-----------------|
    //   SpeedSliderPitchRatio   pitchTweakRatio
    //
    // |-----------------------------------------|
    //   m_pitchRatio
    //
    //                         |-----------------|
    //                           m_pPitchAdjust
    //
    // |-----------------------------------------|
    //   m_pPitch

    auto speedSliderPitchRatio = m_pitchRateInfo.pitchRatio / m_pitchRateInfo.pitchTweakRatio;

    if (m_pKeylock->toBool()) {
        if (!m_pitchRateInfo.keylock) {
            // Enabling Keylock
            if (m_keylockMode->get() == kLockCurrentKey) {
                // Lock at current pitch
                speedSliderPitchRatio = m_pitchRateInfo.tempoRatio;
            } else {
                // kOffsetScaleLockOriginalKey
                // Lock at original track pitch
                speedSliderPitchRatio = 1.0;
            }
            m_pitchRateInfo.keylock = true;
        }
    } else {
        // !bKeylock
        if (m_pitchRateInfo.keylock) {
            // Disabling Keylock
            if (m_keylockMode->get() == kLockCurrentKey) {
                // reset to linear pitch
                m_pitchRateInfo.pitchTweakRatio = 1.0;
                // For not resetting to linear pitch:
                // Adopt speedPitchRatio change as pitchTweakRatio
                //pitchRateInfo.pitchTweakRatio *= (m_speedSliderPitchRatio / pitchRateInfo.tempoRatio);
            }
            m_pitchRateInfo.keylock = false;
        }
        speedSliderPitchRatio = m_pitchRateInfo.tempoRatio;
    }

    m_pitchRateInfo.pitchRatio = m_pitchRateInfo.pitchTweakRatio * speedSliderPitchRatio;

    auto pitchOctaves = KeyUtils::powerOf2ToOctaveChange(m_pitchRateInfo.pitchRatio);
    auto dFileKey = m_pFileKey->get();
    updateKeyCOs(dFileKey, pitchOctaves);

    // qDebug() << "KeyControl::slotRateChanged 2" << m_pitchRatio << m_speedSliderPitchRatio;
}

void KeyControl::slotFileKeyChanged(double value) {
    updateKeyCOs(value,  m_pPitch->get() / 12);
}

void KeyControl::updateKeyCOs(double fileKeyNumeric, double pitchOctaves) {
    //qDebug() << "updateKeyCOs 1" << pitchOctaves;
    auto fileKey =KeyUtils::keyFromNumericValue(fileKeyNumeric);

    auto adjusted = KeyUtils::scaleKeyOctaves(fileKey, pitchOctaves);
    m_pEngineKey->set(KeyUtils::keyToNumericValue(adjusted.first));
    auto diff_to_nearest_full_key = adjusted.second;
    m_pEngineKeyDistance->set(diff_to_nearest_full_key);
    m_pPitch->set(pitchOctaves * 12);
    //qDebug() << "updateKeyCOs 2" << diff_to_nearest_full_key;
}


void KeyControl::slotSetEngineKey(double key) {
    // Always set to a full key, reset key_distance
    setEngineKey(key, 0.0);
}

void KeyControl::slotSetEngineKeyDistance(double key_distance) {
    setEngineKey(m_pEngineKey->get(), key_distance);
}

void KeyControl::setEngineKey(double key, double key_distance) {
    auto thisFileKey = KeyUtils::keyFromNumericValue(m_pFileKey->get());
    auto newKey = KeyUtils::keyFromNumericValue(key);
    if (thisFileKey == mixxx::track::io::key::INVALID ||
        newKey == mixxx::track::io::key::INVALID) {
        return;
    }
    auto stepsToTake = KeyUtils::shortestStepsToKey(thisFileKey, newKey);
    auto pitchToTakeOctaves = (stepsToTake + key_distance) / 12.0;

    m_pPitch->set(pitchToTakeOctaves * 12);
    slotPitchChanged(pitchToTakeOctaves * 12);
    return;
}

void KeyControl::slotPitchChanged(double pitch)
{
    Q_UNUSED(pitch)
    m_updatePitchRequest = 1;
    updatePitch();
}

void KeyControl::updatePitch() {
    auto pitch = m_pPitch->get();

    //qDebug() << "KeyControl::slotPitchChanged 1" << pitch <<
    //        m_pitchRateInfo.pitchRatio <<
    //        m_pitchRateInfo.pitchTweakRatio <<
    //        m_pitchRateInfo.tempoRatio;

    auto speedSliderPitchRatio =
            m_pitchRateInfo.pitchRatio / m_pitchRateInfo.pitchTweakRatio;
    // speedSliderPitchRatio must be unchanged
    auto pitchKnobRatio = KeyUtils::semitoneChangeToPowerOf2(pitch);
    m_pitchRateInfo.pitchRatio = pitchKnobRatio;
    m_pitchRateInfo.pitchTweakRatio = pitchKnobRatio / speedSliderPitchRatio;

    auto dFileKey = m_pFileKey->get();
    m_pPitchAdjust->set(
            KeyUtils::powerOf2ToSemitoneChange(m_pitchRateInfo.pitchTweakRatio));
    updateKeyCOs(dFileKey, KeyUtils::powerOf2ToOctaveChange(pitchKnobRatio));

    //qDebug() << "KeyControl::slotPitchChanged 2" << pitch <<
    //        m_pitchRateInfo.pitchRatio <<
    //        m_pitchRateInfo.pitchTweakRatio <<
    //        m_pitchRateInfo.tempoRatio;
}

void KeyControl::slotPitchAdjustChanged(double pitchAdjust) {
    Q_UNUSED(pitchAdjust);
    m_updatePitchAdjustRequest = 1;
    updatePitchAdjust();
}

void KeyControl::updatePitchAdjust()
{
    auto pitchAdjust = m_pPitchAdjust->get();

    //qDebug() << "KeyControl::slotPitchAdjustChanged 1" << pitchAdjust <<
    //        m_pitchRateInfo.pitchRatio <<
    //        m_pitchRateInfo.pitchTweakRatio <<
    //        m_pitchRateInfo.tempoRatio;

    auto speedSliderPitchRatio = m_pitchRateInfo.pitchRatio / m_pitchRateInfo.pitchTweakRatio;
    // speedSliderPitchRatio must be unchanged
    auto pitchAdjustKnobRatio = KeyUtils::semitoneChangeToPowerOf2(pitchAdjust);

    // pitch_adjust is a offset to the pitch set by the speed controls
    // calc absolute pitch
    m_pitchRateInfo.pitchTweakRatio = pitchAdjustKnobRatio;
    m_pitchRateInfo.pitchRatio = pitchAdjustKnobRatio * speedSliderPitchRatio;

    auto dFileKey = m_pFileKey->get();
    updateKeyCOs(dFileKey, KeyUtils::powerOf2ToOctaveChange(m_pitchRateInfo.pitchRatio));

    //qDebug() << "KeyControl::slotPitchAdjustChanged 2" << pitchAdjust <<
    //        m_pitchRateInfo.pitchRatio <<
    //        m_pitchRateInfo.pitchTweakRatio <<
    //        m_pitchRateInfo.tempoRatio;
}

void KeyControl::slotSyncKey(double v) {
    if (v > 0) {
        auto pOtherEngineBuffer = pickSyncTarget();
        syncKey(pOtherEngineBuffer);
    }
}

void KeyControl::slotResetKey(double v) {
    if (v > 0) {
        slotSetEngineKey(m_pFileKey->get());
    }
}

bool KeyControl::syncKey(EngineBuffer* pOtherEngineBuffer) {
    if (!pOtherEngineBuffer) {
        return false;
    }

    auto thisFileKey =
            KeyUtils::keyFromNumericValue(m_pFileKey->get());

    // Get the sync target's effective key, since that is what we aim to match.
    auto dKey = ControlObject::get(ConfigKey(pOtherEngineBuffer->getGroup(), "key"));
    auto otherKey =
            KeyUtils::keyFromNumericValue(dKey);
    auto otherDistance = ControlObject::get(ConfigKey(pOtherEngineBuffer->getGroup(), "visual_key_distance"));

    if (thisFileKey == mixxx::track::io::key::INVALID ||
        otherKey == mixxx::track::io::key::INVALID) {
        return false;
    }

    int stepsToTake = KeyUtils::shortestStepsToCompatibleKey(thisFileKey, otherKey);
    double pitchToTakeOctaves = (stepsToTake + otherDistance) / 12.0;

    m_pPitch->set(pitchToTakeOctaves * 12);
    slotPitchChanged(pitchToTakeOctaves * 12);
    return true;
}

void KeyControl::collectFeatures(GroupFeatureState* pGroupFeatures) const {
    auto fileKey = KeyUtils::keyFromNumericValue(m_pFileKey->get());
    if (fileKey != mixxx::track::io::key::INVALID) {
        pGroupFeatures->has_file_key = true;
        pGroupFeatures->file_key = fileKey;
    }

    auto key = KeyUtils::keyFromNumericValue(m_pEngineKey->get());
    if (key != mixxx::track::io::key::INVALID) {
        pGroupFeatures->has_key = true;
        pGroupFeatures->key = key;
    }
}
