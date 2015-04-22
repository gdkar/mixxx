#include "waveformrenderersimplesignal.h"

#include "waveformwidgetrenderer.h"
#include "waveform/waveform.h"
#include "waveform/waveformwidgetfactory.h"
#include "controlobjectslave.h"
#include "waveform/path_simplify.h"
#include "widget/wskincolor.h"
#include "trackinfoobject.h"
#include "widget/wwidget.h"
#include "util/math.h"

WaveformRendererSimpleSignal::WaveformRendererSimpleSignal(
        WaveformWidgetRenderer* waveformWidgetRenderer)
    : WaveformRendererSignalBase(waveformWidgetRenderer) {
}

WaveformRendererSimpleSignal::~WaveformRendererSimpleSignal() {
}

void WaveformRendererSimpleSignal::onResize() {
  m_upper.clear();
  m_lower.clear();  
}

void WaveformRendererSimpleSignal::onSetup(const QDomNode& node) {
    Q_UNUSED(node);
}

void WaveformRendererSimpleSignal::draw(QPainter* painter,
                                          QPaintEvent* /*event*/) {
    const TrackPointer trackInfo = m_waveformRenderer->getTrackInfo();
    if (!trackInfo) {
        return;
    }

    ConstWaveformPointer waveform = trackInfo->getWaveform();
    if (waveform.isNull()) {
        return;
    }

    if(waveform!=m_wf){
      const int dataSize = waveform->getDataSize();
      if (dataSize <= 1) {
          return;
      }

      const WaveformData* data = waveform->data();
      if (data == NULL) {
          return;
      }
      m_path = QPainterPath();
      quint64 size = dataSize * 64 / waveform->getVisualSampleRate();
      m_upper = simplify_waveform(waveform,size, FilterIndex::All,ChannelIndex::Left);
      m_lower = simplify_waveform(waveform,size, FilterIndex::All,ChannelIndex::Right);
      m_path.moveTo(m_upper.front());
      for(int i = 0; i < m_upper.size();i++){
        m_path.lineTo(m_upper[i]);
      }
      for(int i = m_lower.size()-1;i>=0;i--){
        QPointF pt = m_lower[i];
        pt.setY(-pt.y());
        m_path.lineTo(pt);
      }
      m_path.closeSubpath();
    }
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing, false);
    painter->setRenderHints(QPainter::HighQualityAntialiasing, false);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, false);
    painter->setWorldMatrixEnabled(false);
    painter->resetTransform();
    
    // Represents the # of waveform data points per horizontal pixel.

    // Per-band gain from the EQ knobs.
    float allGain(1.0), lowGain(1.0), midGain(1.0), highGain(1.0);
    getGains(&allGain, &lowGain, &midGain, &highGain);

    const float halfHeight = (float)m_waveformRenderer->getHeight()/2.0;

    const float heightFactor = m_alignment == Qt::AlignCenter
            ? allGain*halfHeight/255.0
            : allGain*m_waveformRenderer->getHeight()/255.0;

    //draw reference line
    if (m_alignment == Qt::AlignCenter) {
        painter->setPen(m_pColors->getAxesColor());
        painter->drawLine(0,halfHeight,m_waveformRenderer->getWidth(),halfHeight);
    }

    painter->setPen(QPen(QBrush(m_pColors->getSignalColor()), 1));
    painter->scale(m_waveformRenderer->getWidth()/(m_waveformRenderer->getLastDisplayedPosition()-m_waveformRenderer->getFirstDisplayedPosition()),m_waveformRenderer->getHeight()*allGain);
    painter->translate(-m_waveformRenderer->getFirstDisplayedPosition(),halfHeight);
    painter->drawPath(m_path);
    painter->restore();
}
