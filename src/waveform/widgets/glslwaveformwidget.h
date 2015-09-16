_Pragma("once")
#include <QGLWidget>

#include "waveformwidgetabstract.h"
class GLSLWaveformRendererSignal;
class GLSLWaveformWidget : public QGLWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    GLSLWaveformWidget(const char* group, QWidget* parent,bool rgbRenderer);
    virtual ~GLSLWaveformWidget();
    virtual void resize(int width, int height);
  protected:
    virtual void castToQWidget();
    virtual void paintEvent(QPaintEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual int render();
  private:
    GLSLWaveformRendererSignal* signalRenderer_;
    friend class WaveformWidgetFactory;
};
class GLSLFilteredWaveformWidget : public GLSLWaveformWidget {
  public:
    GLSLFilteredWaveformWidget(const char* group, QWidget* parent);
    virtual ~GLSLFilteredWaveformWidget() = default;
    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::GLSLFilteredWaveform; }
    static QString getWaveformWidgetName() { return tr("Filtered"); }
    static bool useOpenGl() { return true; }
    static bool useOpenGLShaders() { return true; }
    static bool developerOnly() { return false; }
};

class GLSLRGBWaveformWidget : public GLSLWaveformWidget {
  public:
    GLSLRGBWaveformWidget(const char* group, QWidget* parent);
    virtual ~GLSLRGBWaveformWidget() = default;
    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::GLSLRGBWaveform; }
    static QString getWaveformWidgetName() { return tr("RGB"); }
    static bool useOpenGl() { return true; }
    static bool useOpenGLShaders() { return true; }
    static bool developerOnly() { return false; }
};
