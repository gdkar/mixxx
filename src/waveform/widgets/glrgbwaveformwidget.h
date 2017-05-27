#ifndef GLRGBWAVEFORMWIDGET_H
#define GLRGBWAVEFORMWIDGET_H

#include <QGLWidget>

#include "waveformwidgetabstract.h"

class GLRGBWaveformWidget : public QGLWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    GLRGBWaveformWidget(const char* group, QWidget* parent);
    virtual ~GLRGBWaveformWidget();

    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::GLRGBWaveform; }

    static QString getWaveformWidgetName() { return tr("RGB"); }
    static bool useOpenGl() { return true; }
    static bool useOpenGLShaders() { return false; }
    static bool developerOnly() { return false; }

  protected:
    virtual void castToQWidget();
    virtual void paintEvent(QPaintEvent* event);
    virtual mixxx::Duration render();

  private:
    friend class WaveformWidgetFactory;
};

#endif // GLRGBWAVEFORMWIDGET_H
