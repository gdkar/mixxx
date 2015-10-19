_Pragma("once")
#include "controlobject.h"
#include <atomic>
#include <QTimer>

class ControlObjectSlave;

class ControlIndicator : public ControlObject {
    Q_OBJECT
  public:
    enum class BlinkValue {
        Off  = 0,
        On   = 1,
        Slow = 2, // used for Pioneer play/pause
        Fast = 3, // used for Pioneer cue
    };
    Q_ENUM(BlinkValue);
    Q_PROPERTY(BlinkValue blinkValue READ blinkValue WRITE setBlinkValue NOTIFY blinkValueChanged);
    ControlIndicator(ConfigKey key);
    virtual ~ControlIndicator();
    virtual void setBlinkValue(BlinkValue bv);
    BlinkValue blinkValue() const;
    static void initialize();
  public slots:
    void onTick();
  signals:
    void blinkValueChanged();
  private:
    void set(double value);
    std::atomic<BlinkValue> m_blinkValue;
    static std::atomic<int> s_tick;
    static QTimer           s_timer;
};
