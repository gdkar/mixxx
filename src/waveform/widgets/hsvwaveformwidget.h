#ifndef HSVWAVEFORMWIDGET_H
#define HSVWAVEFORMWIDGET_H

#include <QWidget>

#include "waveformwidgetabstract.h"

class HSVWaveformWidget : public QWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    ~HSVWaveformWidget() override;

    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::HSVWaveform; }

    static QString getWaveformWidgetName() { return tr("HSV"); }
    static bool useOpenGl() { return false; }
    static bool useOpenGLShaders() { return false; }
    static bool developerOnly() { return false; }

  protected:
    virtual void castToQWidget();
    virtual void paintEvent(QPaintEvent* event);

  private:
    HSVWaveformWidget(const char* group, QWidget* parent);
    friend class WaveformWidgetFactory;
};

#endif // HSVWAVEFORMWIDGET_H
