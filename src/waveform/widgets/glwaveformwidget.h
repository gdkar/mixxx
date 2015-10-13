_Pragma("once")
#include <QGLWidget>

#include "waveformwidgetabstract.h"

class GLWaveformWidget : public QGLWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    GLWaveformWidget(const char* group, QWidget* parent);
    virtual ~GLWaveformWidget();

    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::GLFilteredWaveform; }

    static QString getWaveformWidgetName() { return tr("Filtered"); }
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
