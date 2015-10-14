_Pragma("once")
#include <QPainter>
#include <QTime>
#include <QVector>
#include <QtDebug>

#include "trackinfoobject.h"
#include "util.h"
#include "waveform/renderers/waveformrendererabstract.h"
#include "waveform/renderers/waveformsignalcolors.h"

//#define WAVEFORMWIDGETRENDERER_DEBUG

class TrackInfoObject;
class ControlObjectSlave;
class VisualPlayPosition;
class VSyncThread;

class WaveformWidgetRenderer {
  public:
    static const int s_waveformMinZoom;
    static const int s_waveformMaxZoom;
  public:
    explicit WaveformWidgetRenderer(QString group);
    virtual ~WaveformWidgetRenderer();
    virtual bool init();
    virtual bool onInit();
    virtual void setup(const QDomNode& node, const SkinContext& context);
    virtual void onPreRender(VSyncThread* vsyncThread);
    virtual void draw(QPainter* painter, QPaintEvent* event);
    virtual QString getGroup() const;
    virtual TrackPointer getTrackInfo() const;
    virtual double getFirstDisplayedPosition() const;
    virtual double getLastDisplayedPosition() const;
    virtual void setZoom(int zoom);
    virtual double getVisualSamplePerPixel() const;
    virtual double getAudioSamplePerPixel() const;
    //those function replace at its best sample position to an admissible
    //sample position according to the current visual resampling
    //this make mark and signal deterministic
    void regulateVisualSample(int& sampleIndex) const;
    //this "regulate" against visual sampling to make the position in widget
    //stable and deterministic
    // Transform sample index to pixel in track.
    double transformSampleIndexInRendererWorld(int sampleIndex) const;
    // Transform position (percentage of track) to pixel in track.
    double transformPositionInRendererWorld(double position) const;
    double getPlayPos() const;
    double getPlayPosVSample() const;
    double getZoomFactor() const;
    double getRateAdjust() const;
    double getGain() const;
    int getTrackSamples() const;
    void resize(int width, int height);
    int getHeight() const;
    int getWidth() const;
    const WaveformSignalColors* getWaveformSignalColors() const;
    void setTrack(TrackPointer track);
  protected:
    QString m_group;
    TrackPointer m_pTrack;
    QVector<WaveformRendererAbstract*> m_rendererStack;
    int m_height = 0;
    int m_width  = 0;
    WaveformSignalColors m_colors;

    double m_firstDisplayedPosition = -1;
    double m_lastDisplayedPosition  = -1;
    double m_trackPixelCount        = -1;

    double m_zoomFactor             = 0;
    double m_rateAdjust             = 0;
    double m_visualSamplePerPixel   = 0;
    double m_audioSamplePerPixel    = 0;

    //TODO: vRince create some class to manage control/value
    //ControlConnection
    QSharedPointer<VisualPlayPosition> m_visualPlayPosition;
    double m_playPos = -1;
    int m_playPosVSample = -1;
    ControlObjectSlave* m_pRateControlObject = nullptr;
    double m_rate = 0;
    ControlObjectSlave* m_pRateRangeControlObject = nullptr;
    double m_rateRange = 0;
    ControlObjectSlave* m_pRateDirControlObject = nullptr;
    double m_rateDir = 0;
    ControlObjectSlave* m_pGainControlObject = nullptr;
    double m_gain = 0;
    ControlObjectSlave* m_pTrackSamplesControlObject = nullptr;
    int m_trackSamples = 0;
    bool m_initSuccess = false;
    bool m_haveSetup   = false;
private:
    friend class WaveformWidgetFactory;
};
