_Pragma("once")
#include <QWidget>
#include <QString>

#include "waveform/renderers/waveformwidgetrenderer.h"
#include "trackinfoobject.h"

class VSyncThread;

class WaveformWidget: public QWidget, public WaveformWidgetRenderer
{
    Q_OBJECT
  public:
    enum RenderType
    {
        Empty= 0,
        Simple,
        Filtered,
        HSV,
        RGB
    };
    Q_ENUM(RenderType);
    WaveformWidget(const char* group,QWidget*);
    virtual ~WaveformWidget();
    //Type is use by the factory to safely up-cast waveform widgets
    virtual RenderType getType() const;
    virtual bool isValid() const;
    virtual void setRenderType(RenderType type);
    virtual int render();
    virtual void resize(int width, int height);
    virtual void paintEvent(QPaintEvent *e);
  protected:
    RenderType m_type = Empty;
    friend class WaveformWidgetFactory;
};
