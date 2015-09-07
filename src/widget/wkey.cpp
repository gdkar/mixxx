#include "widget/wkey.h"
#include "track/keys.h"
#include "track/keyutils.h"
#include "controlobjectslave.h"
WKey::WKey(const char* group, QWidget* pParent)
        : WLabel(pParent),
          m_dOldValue(0),
          m_preferencesUpdated(new ControlObjectSlave(ConfigKey("[Preferences]", "updated"),this)),
          m_engineKeyDistance(new ControlObjectSlave(ConfigKey(group, "visual_key_distance"),this)) {
    setValue(m_dOldValue);
    connect(m_preferencesUpdated, SIGNAL(valueChanged(double)),this, SLOT(preferencesUpdated(double)));
    connect(m_engineKeyDistance, SIGNAL(valueChanged(double)),this, SLOT(setCents()));
}
WKey::~WKey() {}
void WKey::onConnectedControlChanged(double dParameter, double dValue) {
    Q_UNUSED(dParameter);
    // Enums are not currently represented using parameter space so it doesn't
    // make sense to use the parameter here yet.
    setValue(dValue);
}
void WKey::setup(QDomNode node, const SkinContext& context) {
    WLabel::setup(node, context);
    m_displayCents = context.selectBool(node, "DisplayCents", false);
}
void WKey::setValue(double dValue) {
    m_dOldValue = dValue;
    mixxx::track::io::key::ChromaticKey key =
            KeyUtils::keyFromNumericValue(dValue);
    if (key != mixxx::track::io::key::INVALID) {
        // Render this key with the user-provided notation.
        QString keyStr = KeyUtils::keyToString(key);
        if (m_displayCents) {
            double diff_cents = m_engineKeyDistance->get();
            int cents_to_display = static_cast<int>(diff_cents * 100);
            char sign = ' ';
            if (diff_cents < 0) {
                sign = '-';
            } else if (diff_cents > 0) {
                sign = '+';
            }
            keyStr.append(QString(" %1%2c").arg(sign).arg(qAbs(cents_to_display)));
        }
        setText(keyStr);
    } else {setText("");}
}

void WKey::setCents() {
    setValue(m_dOldValue);
}

void WKey::preferencesUpdated(double dValue) {
    if (dValue > 0) {
        setValue(m_dOldValue);
    }
}
