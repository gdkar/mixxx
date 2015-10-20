_Pragma("once")
#include <QTimer>
#include "controlpushbutton.h"
class ControlDoublePrivate;
class ControlBehavior {
  public:
    virtual ~ControlBehavior() = default;
    // Returns true if the set should occur. Mutates dValue if the value should
    // be changed.
    virtual bool setFilter(double* dValue);
    virtual double valueToParameter(double dValue);
    virtual double parameterToValue(double dParam);
    virtual void setValueFromParameter(double dParam, ControlDoublePrivate* pControl);
};
class PotmeterBehavior : public ControlBehavior {
  public:
    PotmeterBehavior(double dMinValue, double dMaxValue, bool allowOutOfBounds);
    virtual ~PotmeterBehavior();
    virtual bool setFilter(double* dValue);
    virtual double valueToParameter(double dValue);
    virtual double parameterToValue(double dParam);
  protected:
    double m_dMinValue;
    double m_dMaxValue;
    double m_dValueRange;
    bool m_bAllowOutOfBounds;
};
class LogPotmeterBehavior : public PotmeterBehavior {
  public:
    LogPotmeterBehavior(double dMinValue, double dMaxValue, double minDB);
    virtual ~LogPotmeterBehavior();
    virtual double valueToParameter(double dValue);
    virtual double parameterToValue(double dParam);
  protected:
    double m_minDB;
    double m_minOffset;
};
class LinPotmeterBehavior : public PotmeterBehavior {
  public:
    LinPotmeterBehavior(double dMinValue, double dMaxValue, bool allowOutOfBounds);
    virtual ~LinPotmeterBehavior();
};
class AudioTaperPotBehavior : public PotmeterBehavior {
  public:
    AudioTaperPotBehavior(double minDB, double maxDB, double neutralParameter);
    virtual ~AudioTaperPotBehavior();
    virtual double valueToParameter(double dValue);
    virtual double parameterToValue(double dParam);
    virtual void setValueFromParameter(double dParam, ControlDoublePrivate* pControl);
  protected:
    // a knob position between 0 and 1 where the gain is 1 (0dB)
    double m_neutralParameter;
    // the Start value of the pure db scale it cranked to -Infinity by the
    // linear part of the AudioTaperPot
    double m_minDB;
    // maxDB is the upper gain Value
    double m_maxDB;
    // offset at knob position 0 (Parameter = 0) to reach -Infinity
    double m_offset;
};
class TTRotaryBehavior : public ControlBehavior {
  public:
    virtual double valueToParameter(double dValue);
    virtual double parameterToValue(double dParam);
};
class PushButtonBehavior : public ControlBehavior {
  public:
    using ButtonMode = typename ControlPushButton::ButtonMode;
    static const int kPowerWindowTimeMillis;
    static const int kLongPressLatchingTimeMillis;
    // TODO(XXX) Duplicated from ControlPushButton. It's complicated and
    // annoying to share them so I just copied them.
    PushButtonBehavior(ButtonMode buttonMode, int iNumStates);
    virtual ~PushButtonBehavior();
    virtual void setValueFromParameter(double dParam, ControlDoublePrivate* pControl);
  private:
    ButtonMode m_buttonMode;
    int m_iNumStates;
    QTimer m_pushTimer;
};
