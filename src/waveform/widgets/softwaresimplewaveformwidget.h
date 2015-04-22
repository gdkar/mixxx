#ifndef SOFTWARESIMPLEWAVEFORMWIDGET_H
#define SOFTWARESIMPLEWAVEFORMWIDGET_H

#include <QWidget>

#include "waveformwidgetabstract.h"

class SoftwareSimpleWaveformWidget : public QWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    virtual ~SoftwareSimpleWaveformWidget();

    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::SoftwareSimpleWaveform; }

    static inline QString getWaveformWidgetName() { return tr("Filtered") + " - " + tr("SoftwareSimple"); }
    static inline bool useOpenGl() { return false; }
    static inline bool useOpenGLShaders() { return false; }
    static inline bool developerOnly() { return false; }

  protected:
    virtual void castToQWidget();
    virtual void paintEvent(QPaintEvent* event);

  private:
    SoftwareSimpleWaveformWidget(const char* group, QWidget* parent);
    friend class WaveformWidgetFactory;
};

#endif // SOFTWARESIMPLEWAVEFORMWIDGET_H
