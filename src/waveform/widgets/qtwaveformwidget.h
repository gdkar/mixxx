#ifndef QTWAVEFORMWIDGET_H
#define QTWAVEFORMWIDGET_H

#include <QGLWidget>

#include "waveformwidgetabstract.h"

class QtWaveformWidget : public QWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    Q_INVOKABLE QtWaveformWidget(const char* group, QWidget* parent);
    virtual ~QtWaveformWidget();

    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::QtWaveform; }

    static inline QString getWaveformWidgetName() { return tr("Filtered") + " - Qt"; }
    static inline bool useOpenGl() { return false; }
    static inline bool useOpenGLShaders() { return false; }
    static inline bool developerOnly() { return false; }

  protected:
    virtual void paintEvent(QPaintEvent* event);

  private:
    friend class WaveformWidgetFactory;
};

#endif // QTWAVEFORMWIDGET_H
