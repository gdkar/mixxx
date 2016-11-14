#ifndef SOUNDDEVICENOTFOUND_H
#define SOUNDDEVICENOTFOUND_H

#include <QString>

#include "soundio/sounddevice.h"


class SoundManager;
class EngineNetworkStream;

// This is a fake device, constructed from SoundMamagerConfig data.
// It is used for error reporting only when there is no real data from the
// sound API

class SoundDeviceNotFound : public SoundDevice {
    Q_OBJECT
  public:
    SoundDeviceNotFound(QString internalName, SoundManager *sm);
   ~SoundDeviceNotFound() override = default;
    SoundDeviceError open(bool isClkRefDevice, int syncBuffers) override;
    bool isOpen() const  override;
    SoundDeviceError close() override;
    void readProcess() override;
    void writeProcess() override;
    QString getError() const override;
    unsigned int getDefaultSampleRate() const override;
};

#endif // SOUNDDEVICENOTFOUND_H
