_Pragma("once")
#include <QString>
#include <QImage>
#include <QColor>

#include "configobject.h"
#include "skin/skincontext.h"

class ControlObject;
class QDomNode;
class WaveformSignalColors;

class WaveformMark {
  public:
    WaveformMark();
    ~WaveformMark();
    void setup(const QString& group, const QDomNode& node,const SkinContext& context,const WaveformSignalColors& signalColors);
    void setKeyAndIndex(const ConfigKey& key, int i);
  private:
    ControlObject* m_pointControl = nullptr;
    QColor m_color;
    QColor m_textColor;
    QString m_text;
    Qt::Alignment m_align;
    QString m_pixmapPath;
    QImage m_image;

    friend class WaveformMarkSet;
    friend class WaveformRenderMark;
    friend class WOverview;
};
