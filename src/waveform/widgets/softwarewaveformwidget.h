_Pragma("once")
#include "waveformwidgetabstract.h"

class SoftwareWaveformWidget : public WaveformWidgetAbstract
{
    Q_OBJECT
  public:
    virtual ~SoftwareWaveformWidget();
    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::SoftwareWaveform; }
    static QString getWaveformWidgetName() { return tr("Filtered") + " - " + tr("Software"); }
    static bool useOpenGl() { return false; }
    static bool useOpenGLShaders() { return false; }
    static bool developerOnly() { return false; }
  private:
    SoftwareWaveformWidget(const char* group, QWidget* parent);
    friend class WaveformWidgetFactory;
};
