_Pragma("once")
#include "waveformwidgetabstract.h"

class RGBWaveformWidget : public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    virtual ~RGBWaveformWidget();
    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::RGBWaveform; }
    static inline QString getWaveformWidgetName() { return tr("RGB"); }
    static inline bool useOpenGl() { return false; }
    static inline bool useOpenGLShaders() { return false; }
    static inline bool developerOnly() { return false; }
  private:
    RGBWaveformWidget(const char* group, QWidget* parent);
    friend class WaveformWidgetFactory;
};
