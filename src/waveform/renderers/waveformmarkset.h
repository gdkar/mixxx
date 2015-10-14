_Pragma("once")
#include <QList>

#include "waveformmark.h"
#include "skin/skincontext.h"

class WaveformWidgetRenderer;

class WaveformMarkSet {
  public:
    WaveformMarkSet();
    virtual ~WaveformMarkSet();

    void setup(const QString& group, const QDomNode& node,
               const SkinContext& context,
               const WaveformSignalColors& signalColors);
    void clear();
    int size() const;
    WaveformMark& operator[] (int i);
    const WaveformMark& at(int i );
    const WaveformMark& getDefaultMark() const;
  private:
    WaveformMark m_defaultMark;
    QList<WaveformMark> m_marks;
};
