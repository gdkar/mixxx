_Pragma("once")
#include "waveformwidgetabstract.h"

class QtWaveformWidget : public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    QtWaveformWidget(const char* group, QWidget* parent);
    virtual ~QtWaveformWidget();
    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::QtWaveform; }
    static QString getWaveformWidgetName() { return tr("Filtered") + " - Qt"; }
    static bool useOpenGl() { return false; }
    static bool useOpenGLShaders() { return false; }
    static bool developerOnly() { return false; }
  protected:
    virtual void paintEvent(QPaintEvent* event);
  private:friend class WaveformWidgetFactory;
};
