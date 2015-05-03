// Tue Haste Andersen <haste@diku.dk>, (C) 2003

#include <QTime>

#include "widget/wnumberpos.h"
#include "control/controlobject.h"
#include "control/controlobjectthread.h"
#include "util/math.h"
#include "util/time.h"

WNumberPos::WNumberPos(const char* group, QWidget* parent)
        : WNumber(parent),
          m_dOldValue(0.0),
          m_dTrackSamples(0.0),
          m_dTrackSampleRate(0.0),
          m_bRemain(false) {
    m_pShowTrackTimeRemaining = new ControlObjectThread(
            "[Controls]", "ShowDurationRemaining");
    m_pShowTrackTimeRemaining->connectValueChanged(
            this, SLOT(onSetRemain(double)));
    onSetRemain(m_pShowTrackTimeRemaining->get());

    // We use the engine's playposition value directly because the parameter
    // normalization done by the widget system used to be unusable for this
    // because the range of playposition was -0.14 to 1.14 in 1.11.x. As a
    // result, the <Connection> parameter is no longer necessary in skin
    // definitions, but leaving it in is harmless.
    m_pVisualPlaypos = new ControlObjectThread(group, "playposition");
    m_pVisualPlaypos->connectValueChanged(this, SLOT(onSetValue(double)));

    m_pTrackSamples = new ControlObjectThread(
            group, "track_samples");
    m_pTrackSamples->connectValueChanged(
            this, SLOT(onSetTrackSamples(double)));

    // Tell the CO to re-emit its value since we could be created after it was
    // set to a valid value.
    m_pTrackSamples->emitValueChanged();

    m_pTrackSampleRate = new ControlObjectThread(
            group, "track_samplerate");
    m_pTrackSampleRate->connectValueChanged(
            this, SLOT(onSetTrackSampleRate(double)));

    // Tell the CO to re-emit its value since we could be created after it was
    // set to a valid value.
    m_pTrackSampleRate->emitValueChanged();

    onSetValue(m_pVisualPlaypos->get());
}

WNumberPos::~WNumberPos() {
    delete m_pTrackSampleRate;
    delete m_pTrackSamples;
    delete m_pVisualPlaypos;
    delete m_pShowTrackTimeRemaining;
}

void WNumberPos::mousePressEvent(QMouseEvent* pEvent) {
    bool leftClick = pEvent->buttons() & Qt::LeftButton;

    if (leftClick) {
        setRemain(!m_bRemain);
        m_pShowTrackTimeRemaining->onSet(m_bRemain ? 1.0 : 0.0);
    }
}

void WNumberPos::onSetTrackSamples(double dSamples) {
    m_dTrackSamples = dSamples;
    onSetValue(m_dOldValue);
}

void WNumberPos::onSetTrackSampleRate(double dSampleRate) {
    m_dTrackSampleRate = dSampleRate;
    onSetValue(m_dOldValue);
}

void WNumberPos::setValue(double dValue) {
    // Ignore scaled signals from the skin connection.
    Q_UNUSED(dValue);
    // Update our value with the old value.
    onSetValue(m_dOldValue);
}

void WNumberPos::onSetValue(double dValue) {
    m_dOldValue = dValue;

    double valueMillis = 0.0;
    if (m_dTrackSamples > 0 && m_dTrackSampleRate > 0) {
        double dDuration = m_dTrackSamples / m_dTrackSampleRate / 2.0;
        valueMillis = dValue * 500.0 * m_dTrackSamples / m_dTrackSampleRate;
        double durationMillis = dDuration * Time::kMillisPerSecond;
        if (m_bRemain)
            valueMillis = math_max(durationMillis - valueMillis, 0.0);
    }

    QString valueString;
    if (valueMillis >= 0) {
        valueString = m_skinText % Time::formatSeconds(
                valueMillis / Time::kMillisPerSecond, true);
    } else {
        valueString = m_skinText % QLatin1String("-") % Time::formatSeconds(
                -valueMillis / Time::kMillisPerSecond, true);
    }
    setText(valueString);
}

void WNumberPos::onSetRemain(double remain) {
    setRemain(remain > 0.0);
}

void WNumberPos::setRemain(bool bRemain) {
    m_bRemain = bRemain;

    // Shift display state between showing position and remaining
    if (m_bRemain) {
        m_skinText = "-";
    } else {
        m_skinText = "";
    }
    // Have the widget redraw itself with its current value.
    onSetValue(m_dOldValue);
}
