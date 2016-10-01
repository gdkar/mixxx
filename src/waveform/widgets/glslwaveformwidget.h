#ifndef GLWAVEFORMWIDGETSHADER_H
#define GLWAVEFORMWIDGETSHADER_H

#include <QGLWidget>

#include "waveformwidgetabstract.h"

class GLSLWaveformRendererFilteredSignal;

class GLSLWaveformWidget : public QGLWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    GLSLWaveformWidget(const char* group, QWidget* parent);
    virtual ~GLSLWaveformWidget();

    virtual void resize(int width, int height);

  protected:
    virtual void paintEvent(QPaintEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual mixxx::Duration render();

  private:
    GLSLWaveformRendererFilteredSignal* signalRenderer_;

    friend class WaveformWidgetFactory;
};

class GLSLFilteredWaveformWidget : public GLSLWaveformWidget {
  public:
    Q_INVOKABLE GLSLFilteredWaveformWidget(const char* group, QWidget* parent);
    virtual ~GLSLFilteredWaveformWidget() {}

    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::GLSLFilteredWaveform; }

    static QString getWaveformWidgetName() { return tr("Filtered"); }
    static bool useOpenGl() { return true; }
    static bool useOpenGLShaders() { return true; }
    static bool developerOnly() { return false; }
};
#endif // GLWAVEFORMWIDGETSHADER_H
