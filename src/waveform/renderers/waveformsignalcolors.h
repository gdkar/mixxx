_Pragma("once")
#include <QColor>
#include <QDomNode>

#include "skin/skincontext.h"

class WaveformSignalColors
{
  public:
    WaveformSignalColors();
    virtual ~WaveformSignalColors();
    bool setup(const QDomNode &node, const SkinContext& context);
    QColor getSignalColor() const;
    QColor getLowColor() const;
    QColor getMidColor() const;
    QColor getHighColor() const;
    QColor getRgbLowColor() const;
    QColor getRgbMidColor() const;
    QColor getRgbHighColor() const;
    QColor getAxesColor() const;
    QColor getPlayPosColor() const;
    QColor getBgColor() const;
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
