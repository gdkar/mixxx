#ifndef RGBWAVEFORMWIDGET_H
#define RGBWAVEFORMWIDGET_H

#include <QWidget>

#include "waveformwidgetabstract.h"

class RGBWaveformWidget : public QWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
   ~RGBWaveformWidget() override;

    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::RGBWaveform; }

    static QString getWaveformWidgetName() { return tr("RGB"); }
    static bool useOpenGl() { return false; }
    static bool useOpenGLShaders() { return false; }
    static bool developerOnly() { return false; }

  protected:
    virtual void castToQWidget();
    virtual void paintEvent(QPaintEvent* event);

  private:
    RGBWaveformWidget(const char* group, QWidget* parent);
    friend class WaveformWidgetFactory;
};

#endif // RGBWAVEFORMWIDGET_H
