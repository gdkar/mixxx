#ifndef SOUNDDEVICENETWORK_H
#define SOUNDDEVICENETWORK_H

#include <QString>

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
    virtual ~SoundDeviceNetwork();

    virtual Result open(bool isClkRefDevice, int syncBuffers) override;
    virtual bool isOpen() const override ;
    virtual Result close() override;
    virtual void readProcess() override;
    virtual void writeProcess() override;
    virtual QString getError() const override;

    virtual unsigned int getDefaultSampleRate() const override{
        return 44100;
    }

  private:
    QSharedPointer<EngineNetworkStream> m_pNetworkStream;
    FIFO<CSAMPLE>* m_outputFifo;
    FIFO<CSAMPLE>* m_inputFifo;
    bool m_outputDrift;
    bool m_inputDrift;
    static std::atomic<int> m_underflowHappened;
};

#endif // SOUNDDEVICENETWORK_H
