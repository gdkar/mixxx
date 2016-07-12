#ifndef GLWAVEFORMWIDGETSHADER_H
#define GLWAVEFORMWIDGETSHADER_H

#include "waveform/renderers/waveformwidgetrenderer.h"

class GLSLFilteredWaveformWidget : public WaveformWidgetRenderer {
    Q_OBJECT
  public:
    GLSLFilteredWaveformWidget(const char* group, QWidget* parent);
    virtual ~GLSLFilteredWaveformWidget();
  protected:
    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::GLSLFilteredWaveform; }
    static inline QString getWaveformWidgetName() { return tr("Filtered"); }
    static inline bool useOpenGl() { return true; }
    static inline bool useOpenGLShaders() { return true; }
    static inline bool developerOnly() { return false; }
  private:
    friend class WaveformWidgetFactory;
};
#endif // GLWAVEFORMWIDGETSHADER_H
