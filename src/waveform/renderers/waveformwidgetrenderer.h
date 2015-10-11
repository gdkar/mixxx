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
    explicit WaveformWidgetRenderer(const char* group);
    virtual ~WaveformWidgetRenderer();
    bool init();
    virtual bool onInit() {return true;}
    void setup(const QDomNode& node, const SkinContext* context);
    void onPreRender(VSyncThread* vsyncThread);
    void draw(QPainter* painter, QPaintEvent* event);
    const char* getGroup() const { return m_group;}
    const TrackPointer getTrackInfo() const { return m_pTrack;}
    double getFirstDisplayedPosition() const { return m_firstDisplayedPosition;}
    double getLastDisplayedPosition() const { return m_lastDisplayedPosition;}
    void setZoom(int zoom);
    double getVisualSamplePerPixel() const { return m_visualSamplePerPixel;};
    double getAudioSamplePerPixel() const { return m_audioSamplePerPixel;};
    //those function replace at its best sample position to an admissible
    //sample position according to the current visual resampling
    //this make mark and signal deterministic
    void regulateVisualSample(int& sampleIndex) const;
    //this "regulate" against visual sampling to make the position in widget
    //stable and deterministic
    // Transform sample index to pixel in track.
    double transformSampleIndexInRendererWorld(int sampleIndex) const {
        const double relativePosition = (double)sampleIndex / (double)m_trackSamples;
        return transformPositionInRendererWorld(relativePosition);
    }
    // Transform position (percentage of track) to pixel in track.
    double transformPositionInRendererWorld(double position) const {
        return m_trackPixelCount * (position - m_firstDisplayedPosition);
    }
    double getPlayPos() const { return m_playPos;}
    double getPlayPosVSample() const { return m_playPosVSample;}
    double getZoomFactor() const { return m_zoomFactor;}
    double getRateAdjust() const { return m_rateAdjust;}
    double getGain() const { return m_gain;}
    int getTrackSamples() const { return m_trackSamples;}
    void resize(int width, int height);
    int getHeight() const { return m_height;}
    int getWidth() const { return m_width;}
    const WaveformSignalColors* getWaveformSignalColors() const { return &m_colors; };
    template< class T_Renderer>
    T_Renderer* addRenderer() {
        T_Renderer* renderer = new T_Renderer(this);
        m_rendererStack.push_back(renderer);
        return renderer;
    }
    void setTrack(TrackPointer track);
  protected:
    const char* m_group;
    TrackPointer m_pTrack;
    QList<WaveformRendererAbstract*> m_rendererStack;
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
private:
    DISALLOW_COPY_AND_ASSIGN(WaveformWidgetRenderer);
    friend class WaveformWidgetFactory;
};
