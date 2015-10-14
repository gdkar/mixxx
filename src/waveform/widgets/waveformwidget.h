_Pragma("once")
#include <QWidget>
#include <QString>

#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveformwidgettype.h"
#include "trackinfoobject.h"

class VSyncThread;

class WaveformWidget: public QWidget, public WaveformWidgetRenderer {
    Q_OBJECT
  public:
    WaveformWidget(const char* group,QWidget*);
    virtual ~WaveformWidget();
    //Type is use by the factory to safely up-cast waveform widgets
    virtual WaveformWidgetType::Type getType() const;
    virtual bool isValid() const;
    void hold();
    void release();
    virtual void setRenderType(WaveformWidgetType::Type type);
    virtual void preRender(VSyncThread* vsyncThread);
    virtual int render();
    virtual void resize(int width, int height);
    virtual void paintEvent(QPaintEvent *e);
  protected:
    WaveformWidgetType::Type m_type = WaveformWidgetType::EmptyWaveform;
    friend class WaveformWidgetFactory;
};
