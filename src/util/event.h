_Pragma("once")
#include <QtGlobal>
#include <QString>

#include "util/stat.h"
#include "util/duration.h"

class Event {
  public:
    Event() = default;
    using EventType = typename Stat::StatType;

    QString m_tag;
    EventType m_type{Stat::UNSPECIFIED};
    mixxx::Duration m_time;
    static bool event(const QString& tag, Event::EventType type = Stat::EVENT)
    {
        return Stat::track(tag, type, Stat::experimentFlags(Stat::COUNT), 0.0);
    }
    static bool start(const QString& tag)
    {
        return event(tag, Stat::EVENT_START);
    }
    static bool end(const QString& tag)
    {
        return event(tag, Stat::EVENT_END);
    }
};
