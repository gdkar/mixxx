_Pragma("once")
#include "waveformwidgetabstract.h"
class QtSimpleWaveformWidget : public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    QtSimpleWaveformWidget(const char* group, QWidget* parent);
    virtual ~QtSimpleWaveformWidget();
    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::QtSimpleWaveform; }
    static inline QString getWaveformWidgetName() { return tr("Simple") + " - Qt"; }
    static inline bool useOpenGl() { return false; }
    static inline bool useOpenGLShaders() { return false; }
    static inline bool developerOnly() { return false; }
  protected:
    virtual void paintEvent(QPaintEvent* event);
  private:
    friend class WaveformWidgetFactory;
};
