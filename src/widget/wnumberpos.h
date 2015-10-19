// Tue Haste Andersen <haste@diku.dk>, (C) 2003

_Pragma("once")
#include <QMouseEvent>

#include "wnumber.h"

class ControlObject;

class WNumberPos : public WNumber {
    Q_OBJECT
  public:
    WNumberPos(const char *group, QWidget *parent=0);
    virtual ~WNumberPos();

    // Set if the display shows remaining time (true) or position (false)
    void setRemain(bool bRemain);

  protected:
    void mousePressEvent(QMouseEvent* pEvent);

  private slots:
    void setValue(double dValue);
    void slotSetValue(double);
    void slotSetRemain(double dRemain);
    void slotSetTrackSampleRate(double dSampleRate);
    void slotSetTrackSamples(double dSamples);

  private:
    // Old value set
    double m_dOldValue;
    double m_dTrackSamples;
    double m_dTrackSampleRate;
    // True if remaining content is being shown
    bool m_bRemain;
    ControlObject* m_pShowTrackTimeRemaining;
    // Pointer to control object for position, rate, and track info
    ControlObject* m_pVisualPlaypos;
    ControlObject* m_pTrackSamples;
    ControlObject* m_pTrackSampleRate;
};
