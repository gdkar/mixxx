_Pragma("once")
#include <QObject>
#include <QTime>
#include <QTimer>
#include <QVector>
#include <memory>

#include "util/singleton.h"
#include "configobject.h"
#include "waveform/widgets/waveformwidget.h"
#include "waveform/waveform.h"
#include "skin/skincontext.h"

class WWaveformViewer;
class QTimer;
class MixxxMainWindow;

//########################################

class WaveformWidgetFactory : public QObject, public Singleton<WaveformWidgetFactory> {
    Q_OBJECT
  public:
    //TODO merge this enum woth the waveform analyser one
    enum FilterIndex { All = 0, Low = 1, Mid = 2, High = 3, FilterCount = 4};
    bool setConfig(ConfigObject<ConfigValue>* config);
    //creates the waveform widget and bind it to the viewer
    //clean-up every thing if needed
    bool setWaveformWidget(WWaveformViewer* viewer,const QDomElement &node, const SkinContext& context);
    void setFrameRate(int frameRate);
    int getFrameRate() const { return m_frameRate;}
//    bool getVSync() const { return m_vSyncType;}
    void setEndOfTrackWarningTime(int endTime);
    int getEndOfTrackWarningTime() const { return m_endOfTrackWarningTime;}
    bool setWidgetType(WaveformWidget::RenderType type);
    WaveformWidget::RenderType getType() const { return m_type;}
    void setDefaultZoom(int zoom);
    int getDefaultZoom() const { return m_defaultZoom;}

    void setZoomSync(bool sync);
    int isZoomSync() const { return m_zoomSync;}

    void setVisualGain(FilterIndex index, double gain);
    double getVisualGain(FilterIndex index) const;

    void setOverviewNormalized(bool normalize);
    int isOverviewNormalized() const { return m_overviewNormalized;}
    void addTimerListener(QWidget* pWidget);
    void startVSync(MixxxMainWindow* mixxxApp);
    void setVSyncType(int vsType);
    int getVSyncType();
    void notifyZoomChange(WWaveformViewer *viewer);
    WaveformWidget::RenderType autoChooseWidgetType() const;
  signals:
    void waveformUpdateTick();
    void waveformMeasured(float frameRate, int droppedFrames);
    void renderTypeChanged(WaveformWidget::RenderType type);
    void zoomChanged(int);
    void preRender(int);
    void render();
  protected:
    WaveformWidgetFactory();
    virtual ~WaveformWidgetFactory();
    friend class Singleton<WaveformWidgetFactory>;
  private slots:
    void onRender();
  private:
    //Currently in use widgets/visual/node
    WaveformWidget::RenderType m_type;
    ConfigObject<ConfigValue>* m_config;
    bool m_skipRender;
    int m_frameRate;
    int m_endOfTrackWarningTime;
    int m_defaultZoom;
    bool m_zoomSync;
    double m_visualGain[FilterCount];
    bool m_overviewNormalized;
    QTimer m_updateTimer;
    //Debug
    QTime m_time;
    float m_frameCnt;
    double m_actualFrameRate;
};
