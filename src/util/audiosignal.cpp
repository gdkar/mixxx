#include <QtDebug>

#include "util/audiosignal.h"

namespace mixxx {

bool AudioSignal::verifyReadable() const {
    bool result = true;
    if (!hasValidChannelCount()) {
        qWarning() << "Invalid number of channels:"
                << getChannelCount()
                << "is out of range ["
                << kChannelCountMin
                << ","
                << kChannelCountMax
                << "]";
        result = false;
    }
    if (!hasValidSampleRate()) {
        qWarning() << "Invalid sampling rate [Hz]:"
                << getSampleRate()
                << "is out of range ["
                << kSampleRateMin
                << ","
                << kSampleRateMax
                << "]";
        result = false;
    }
    return result;
}

} // namespace mixxx
