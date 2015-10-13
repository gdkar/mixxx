_Pragma("once")
#include "waveformwidgetabstract.h"

class HSVWaveformWidget : public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    virtual ~HSVWaveformWidget();

    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::HSVWaveform; }

    static inline QString getWaveformWidgetName() { return tr("HSV"); }
    static inline bool useOpenGl() { return false; }
    static inline bool useOpenGLShaders() { return false; }
    static inline bool developerOnly() { return false; }
  private:
    HSVWaveformWidget(const char* group, QWidget* parent);
    friend class WaveformWidgetFactory;
};
