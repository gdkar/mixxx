#ifndef LEARNINGUTILS_H
#define LEARNINGUTILS_H

#include <QList>
#include <tuple>
#include <functional>

#include "controllers/midi/midimessage.h"

class LearningUtils {
  public:
    static MidiInputMappings guessMidiInputMappings(
        const ConfigKey& control,
        const QList<std::pair<MidiKey, unsigned char> >& messages);
};

#endif /* LEARNINGUTILS_H */
