#ifndef WAVEFORMMARK_H
#define WAVEFORMMARK_H

#include <QString>
#include <QImage>
#include <QColor>
#include <qsharedpointer.h>
#include <qatomic.h>
#include <qmath.h>
#include "configobject.h"
class SkinContext;
class ControlObjectSlave;
class QDomNode;
class WaveformSignalColors;

class WaveformMark {
  public:
    WaveformMark();
    ~WaveformMark();
    void setup(const QString& group, const QDomNode& node,
               SkinContext* context,
               const WaveformSignalColors& signalColors);
    void setKeyAndIndex(const ConfigKey& key, int i);

  private:
    QSharedPointer<ControlObjectSlave> m_pointControl;

    QColor               m_color;
    QColor               m_textColor;
    QString              m_text;
    Qt::Alignment        m_align;
    QString              m_pixmapPath;
    QImage               m_image;

    friend class WaveformMarkSet;
    friend class WaveformRenderMark;
    friend class WOverview;
};

#endif // WAVEFORMMARK_H
