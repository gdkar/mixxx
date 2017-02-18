#ifndef SOUNDDEVICENETWORK_H
#define SOUNDDEVICENETWORK_H

#include <QString>
#include <atomic>

#include "soundio/sounddevice.h"

#define CPU_USAGE_UPDATE_RATE 30 // in 1/s, fits to display frame rate
#define CPU_OVERLOAD_DURATION 500 // in ms

class SoundManager;
class EngineNetworkStream;

class SoundDeviceNetwork : public SoundDevice {
    Q_OBJECT
  public:
    SoundDeviceNetwork(UserSettingsPointer config,
                       SoundManager *sm,
                       QSharedPointer<EngineNetworkStream> pNetworkStream);
   ~SoundDeviceNetwork() override;

    SoundDeviceError open(bool isClkRefDevice, int syncBuffers) override;
    bool isOpen() const override;
    SoundDeviceError close() override;
    void readProcess() override;
    void writeProcess() override;
    virtual QString getError() const;

    unsigned int getDefaultSampleRate() const override
    {
        return 44100;
    }

  private:
    QSharedPointer<EngineNetworkStream> m_pNetworkStream;
    std::unique_ptr<FIFO<CSAMPLE> > m_outputFifo;
    std::unique_ptr< FIFO<CSAMPLE> > m_inputFifo;
    bool m_outputDrift;
    bool m_inputDrift;
    static std::atomic<int> m_underflowHappened;
};

#endif // SOUNDDEVICENETWORK_H
