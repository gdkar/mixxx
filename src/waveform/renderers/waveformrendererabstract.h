_Pragma("once")
#include <QDomNode>
#include <QPaintEvent>
#include <QPainter>

#include "skin/skincontext.h"

class WaveformWidgetRenderer;

class WaveformRendererAbstract {
  public:
    explicit WaveformRendererAbstract(WaveformWidgetRenderer* waveformWidgetRenderer);
    virtual ~WaveformRendererAbstract();
    virtual bool init();
    virtual void setup(const QDomNode& node, const SkinContext& context) = 0;
    virtual void draw(QPainter* painter, QPaintEvent* event) = 0;
    virtual void onResize() ;
    virtual void onSetTrack() ;
  protected:
    bool isDirty() const;
    void setDirty(bool dirty = true);
    WaveformWidgetRenderer* m_waveformRenderer = nullptr;
    bool m_dirty = true;
    friend class WaveformWidgetRenderer;
};
