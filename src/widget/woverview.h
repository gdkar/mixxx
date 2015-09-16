//
// C++ Interface: woverview
//
// Description:
//
//
// Author: Tue Haste Andersen <haste@diku.dk>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//
_Pragma("once")
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPixmap>
#include <QColor>
#include <QList>

#include "trackinfoobject.h"
#include "widget/wwidget.h"

#include "waveform/renderers/waveformsignalcolors.h"
#include "waveform/renderers/waveformmarkset.h"
#include "waveform/renderers/waveformmarkrange.h"
#include "skin/skincontext.h"

// Waveform overview display
// @author Tue Haste Andersen
class Waveform;

class WOverview : public WWidget {
    Q_OBJECT
  public:
    WOverview(const char* pGroup, ConfigObject<ConfigValue>* pConfig, QWidget* parent=nullptr);
    virtual ~WOverview();
    void setup(QDomNode node, const SkinContext& context);
  public slots:
    void onConnectedControlChanged(double dParameter, double dValue);
    void slotLoadNewTrack(TrackPointer pTrack);
    void slotTrackLoaded(TrackPointer pTrack);
    void slotUnloadTrack(TrackPointer pTrack);
  signals:
    void trackDropped(QString filename, QString group);
  protected:
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dropEvent(QDropEvent* event);
    ConstWaveformPointer getWaveform() const;
    QImage* m_pWaveformSourceImage;
    QImage m_waveformImageScaled;
    WaveformSignalColors m_signalColors;
    // Hold the last visual sample processed to generate the pixmap
    double m_actualCompletion;
    bool m_pixmapDone;
    float m_waveformPeak;
    int m_diffGain;
  private slots:
    void onEndOfTrackChange(double v);

    void onMarkChanged(double v);
    void onMarkRangeChange(double v);

    void slotWaveformSummaryUpdated();
    void slotAnalyserProgress(double progress);

  private:
    // Append the waveform overview pixmap according to available data in waveform
    virtual bool drawNextPixmapPart() = 0;
    void paintText(const QString &text, QPainter *painter);
    int valueToPosition(double value) const;
    double positionToValue(int position) const;
    const QString m_group;
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
    ControlObjectSlave* m_endOfTrackControl = nullptr;
    double m_endOfTrack = 0.0;
    ControlObjectSlave* m_trackSamplesControl = nullptr;
    ControlObjectSlave* m_playControl = nullptr;
    // Current active track
    TrackPointer m_pCurrentTrack;
    ConstWaveformPointer m_pWaveform;
    // True if slider is dragged. Only used when m_bEventWhileDrag is false
    bool m_bDrag = false;
    // Internal storage of slider position in pixels
    int m_iPos = 0;
    QPixmap m_backgroundPixmap;
    QString m_backgroundPixmapPath;
    QColor m_qColorBackground;
    QColor m_endOfTrackColor;
    WaveformMarkSet m_marks;
    std::vector<WaveformMarkRange> m_markRanges;
    // Coefficient value-position linear transposition
    double m_a = 0;
    double m_b = 0;
    double m_dAnalyserProgress = 0;
    bool m_bAnalyserFinalizing = false;
    bool m_trackLoaded         = false;
};
