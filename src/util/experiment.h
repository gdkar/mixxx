_Pragma("once")
#include <QtDebug>
#include <QObject>
#include <atomic>
class Experiment : public QObject
{
  public:
    enum class Mode {
        Off = 0,
        Base = 1,
        Experiment = 2
    };
    Q_ENUM(Mode);
    static bool isEnabled();
    static void disable();
    static void setExperiment();
    static void setBase();
    static bool isExperiment();
    static bool isBase();
    static Mode mode();
  private:
    Experiment() = delete;
    static std::atomic<Mode> s_mode;
};
