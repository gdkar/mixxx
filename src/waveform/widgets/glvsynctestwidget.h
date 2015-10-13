_Pragma("once")
#include <QGLWidget>

#include "waveformwidgetabstract.h"

class GLVSyncTestWidget : public QGLWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    GLVSyncTestWidget(const char* group, QWidget* parent);
    virtual ~GLVSyncTestWidget();

    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::GLVSyncTest; }

    static QString getWaveformWidgetName() { return tr("VSyncTest"); }
    static bool useOpenGl() { return true; }
    static bool useOpenGLShaders() { return false; }
    static bool developerOnly() { return true; }

  protected:
    virtual void castToQWidget();
    virtual void paintEvent(QPaintEvent* event);
    virtual int render();

  private:
    friend class WaveformWidgetFactory;
};
