#ifndef QTWAVEFORMWIDGET_H
#define QTWAVEFORMWIDGET_H

#include <QWidget>

#include "waveform/renderers/waveformwidgetrenderer.h"

class QtWaveformWidget : public WaveformWidgetRenderer {
    Q_OBJECT
  public:
    QtWaveformWidget(const char* group, QWidget* parent);
    virtual ~QtWaveformWidget();
    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::QtWaveform; }
    static inline QString getWaveformWidgetName() { return tr("Filtered") + " - Qt"; }
    static inline bool useOpenGl() { return false; }
    static inline bool useOpenGLShaders() { return false; }
    static inline bool developerOnly() { return false; }
  private:
    friend class WaveformWidgetFactory;
};

#endif // QTWAVEFORMWIDGET_H
