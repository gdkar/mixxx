_Pragma("once")
#include "waveformrendererabstract.h"
#include "waveformsignalcolors.h"
#include "skin/skincontext.h"

class ControlObject;

class WaveformRendererSignalBase : public WaveformRendererAbstract {
public:
    explicit WaveformRendererSignalBase(WaveformWidgetRenderer* waveformWidgetRenderer);
    virtual ~WaveformRendererSignalBase();
    virtual bool init();
    virtual void setup(const QDomNode& node, const SkinContext& context);
    virtual bool onInit();
    virtual void onSetup(const QDomNode &node) = 0;
  protected:
    void deleteControls();
    void getGains(float* pAllGain, float* pLowGain, float* pMidGain,float* highGain);
  protected:
    ControlObject* m_pEQEnabled                = nullptr;
    ControlObject* m_pLowFilterControlObject   = nullptr;
    ControlObject* m_pMidFilterControlObject   = nullptr;
    ControlObject* m_pHighFilterControlObject  = nullptr;
    ControlObject* m_pLowKillControlObject     = nullptr;
    ControlObject* m_pMidKillControlObject     = nullptr;
    ControlObject* m_pHighKillControlObject    = nullptr;

    Qt::Alignment m_alignment = Qt::AlignCenter;

    const WaveformSignalColors* m_pColors           = nullptr;
    qreal m_axesColor_r = 0
        , m_axesColor_g = 0
        , m_axesColor_b = 0
        , m_axesColor_a = 0;
    qreal m_signalColor_r = 0
        , m_signalColor_g = 0
        , m_signalColor_b = 0;
    qreal m_lowColor_r = 0
        , m_lowColor_g = 0
        , m_lowColor_b = 0;
    qreal m_midColor_r = 0
        , m_midColor_g = 0
        , m_midColor_b = 0;
    qreal m_highColor_r = 0
        , m_highColor_g = 0
        , m_highColor_b = 0;
    qreal m_rgbLowColor_r = 0
        , m_rgbLowColor_g
        , m_rgbLowColor_b = 0;
    qreal m_rgbMidColor_r = 0
        , m_rgbMidColor_g = 0
        , m_rgbMidColor_b = 0;
    qreal m_rgbHighColor_r = 0
        , m_rgbHighColor_g = 0
        , m_rgbHighColor_b = 0;
};
