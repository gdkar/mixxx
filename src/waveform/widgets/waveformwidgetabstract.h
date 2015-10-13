_Pragma("once")
#include <QWidget>
#include <QString>

#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveformwidgettype.h"
#include "trackinfoobject.h"

class VSyncThread;

class WaveformWidgetAbstract : public QWidget, public WaveformWidgetRenderer {
    Q_OBJECT
  public:
    WaveformWidgetAbstract(const char* group,QWidget*);
    virtual ~WaveformWidgetAbstract();
    //Type is use by the factory to safely up-cast waveform widgets
    virtual WaveformWidgetType::Type getType() const = 0;
    bool isValid() const { return (m_initSuccess); }
    void hold();
    void release();
    virtual void preRender(VSyncThread* vsyncThread);
    virtual int render();
    virtual void resize(int width, int height);
    virtual void paintEvent(QPaintEvent *e);
  protected:
    friend class WaveformWidgetFactory;
};
