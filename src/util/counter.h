#ifndef COUNTER_H
#define COUNTER_H

#include "util/stat.h"

class Counter {
  public:
    Counter(const QString& tag) : m_tag(tag) { }
    void increment(int by=1) {
        auto flags = Stat::experimentFlags(
            Stat::COUNT | Stat::SUM | Stat::AVERAGE |
            Stat::SAMPLE_VARIANCE | Stat::MIN | Stat::MAX);
        Stat::track(m_tag, Stat::COUNTER, flags, by);
    }
    Counter& operator+=(int by) {
        increment(by);
        return *this;
    }
    Counter &operator++ (){
        return (*this)+=1;
    }
    Counter operator++(int) { // postfix
        auto retval = *this;
        ++(*this);
        return retval;
    }
  private:
    QString m_tag;
};

#endif /* COUNTER_H */
