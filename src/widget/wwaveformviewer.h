_Pragma("once")
#include <QDateTime>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QList>
#include <QMutex>

#include "trackinfoobject.h"
#include "widget/wwidget.h"
#include "waveform/widgets/waveformwidget.h"
#include "skin/skincontext.h"

class ControlObject;
class WaveformWidget;
class ControlPotmeter;

class WWaveformViewer : public WWidget {
    Q_OBJECT
  public:
    WWaveformViewer(const char *group, ConfigObject<ConfigValue>* pConfig, QWidget *parent=0);
    virtual ~WWaveformViewer();
    const char* getGroup() const { return m_pGroup;}
    void setup(QDomNode node, const SkinContext& context);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
signals:
    void trackDropped(QString filename, QString group);
public slots:
    void onTrackLoaded(TrackPointer track);
    void onTrackUnloaded(TrackPointer track);
    void setRenderType(WaveformWidget::RenderType);
    void onZoomChanged(double);
protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
private slots:
    void slotWidgetDead() {m_waveformWidget = nullptr;}
private:
    WaveformWidget* getWaveformWidget() const;
    //direct access to let factory sync/set default zoom
    void setZoom(int zoom);
private:
    const char* m_pGroup;
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
    int m_zoomZoneWidth = 20;
    bool                m_cacheValid = false;
    QDomNode            m_skinNodeCache{};
    SkinContext         m_skinContextCache{nullptr,QString{}};
    ControlObject* m_pZoom = nullptr;
    ControlObject* m_pScratchPositionEnable = nullptr;
    ControlObject* m_pScratchPosition = nullptr;
    ControlObject* m_pWheel = nullptr;
    bool m_bScratching = false;
    bool m_bBending = false;
    QPoint m_mouseAnchor;
    WaveformWidget* m_waveformWidget = nullptr;
    friend class WaveformWidgetFactory;
};
