#ifndef WAVEFORMSIGNALCOLORS_H
#define WAVEFORMSIGNALCOLORS_H

#include <QColor>
#include <QDomNode>

#include "skin/skincontext.h"

class WaveformSignalColors : public QObject {
    Q_OBJECT
  public:
    WaveformSignalColors( QObject *pParent);
    virtual ~WaveformSignalColors() = default;

    bool setup(const QDomNode &node, const SkinContext& context);

    QColor getSignalColor() const { return m_signalColor; }
    QColor getLowColor() const { return m_lowColor; }
    QColor getMidColor() const { return m_midColor; }
    QColor getHighColor() const { return m_highColor; }
    QColor getRgbLowColor() const { return m_rgbLowColor; }
    QColor getRgbMidColor() const { return m_rgbMidColor; }
    QColor getRgbHighColor() const { return m_rgbHighColor; }
    QColor getAxesColor() const { return m_axesColor; }
    QColor getPlayPosColor() const { return m_playPosColor; }
    QColor getBgColor() const { return m_bgColor; }

  protected:
    void fallBackFromSignalColor();
    void fallBackDefaultColor();

    float stableHue(float hue) const;

  private:
    QColor m_signalColor;
    QColor m_lowColor;
    QColor m_midColor;
    QColor m_highColor;
    QColor m_rgbLowColor;
    QColor m_rgbMidColor;
    QColor m_rgbHighColor;
    QColor m_axesColor;
    QColor m_playPosColor;
    QColor m_bgColor;
};

#endif // WAVEFORMSIGNALCOLORS_H
