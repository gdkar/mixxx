#ifndef GLWAVEFORMWIDGET_H
#define GLWAVEFORMWIDGET_H

#include <QOpenGLWidget>

#include "waveformwidgetabstract.h"

class GLWaveformWidget : public QOpenGLWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    GLWaveformWidget(const char* group, QWidget* parent);
    virtual ~GLWaveformWidget();

    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::GLFilteredWaveform; }

    static inline QString getWaveformWidgetName() { return tr("Filtered"); }
    static inline bool useOpenGl() { return true; }
    static inline bool useOpenGLShaders() { return false; }
    static inline bool developerOnly() { return false; }

  protected:
    virtual void castToQWidget();
    virtual void paintEvent(QPaintEvent* event);
    virtual int render();

  private:
    friend class WaveformWidgetFactory;
};

#endif // GLWAVEFORMWIDGET_H
