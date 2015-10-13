_Pragma("once")
#include <QGLWidget>

#include "waveformwidgetabstract.h"

class GLSimpleWaveformWidget : public QGLWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    GLSimpleWaveformWidget(const char* group, QWidget* parent);
    virtual ~GLSimpleWaveformWidget();
    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::GLSimpleWaveform; }
    static QString getWaveformWidgetName() { return tr("Simple"); }
    static bool useOpenGl() { return true; }
    static bool useOpenGLShaders() { return false; }
    static bool developerOnly() { return false; }

  protected:
    virtual void castToQWidget();
    virtual void paintEvent(QPaintEvent* event);
    virtual int render();
  private:
    friend class WaveformWidgetFactory;
};
