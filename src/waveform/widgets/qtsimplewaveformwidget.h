#ifndef QTSIMPLEWAVEFORMWIDGET_H
#define QTSIMPLEWAVEFORMWIDGET_H

#include <QWidget>

#include "waveformwidgetabstract.h"

class QtSimpleWaveformWidget : public QWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    QtSimpleWaveformWidget(const char* group, QWidget* parent);
    virtual ~QtSimpleWaveformWidget();

    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::GLSimpleWaveform; }

    static QString getWaveformWidgetName() { return tr("Simple") + " - Qt"; }
    static bool useOpenGl() { return false; }
    static bool useOpenGLShaders() { return false; }
    static bool developerOnly() { return false; }

  protected:
    virtual void castToQWidget();
    virtual void paintEvent(QPaintEvent* event);

  private:
    friend class WaveformWidgetFactory;
};

#endif // QTSIMPLEWAVEFORMWIDGET_H
