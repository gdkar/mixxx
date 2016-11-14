#include "soundio/sounddevicenotfound.h"

SoundDeviceNotFound::SoundDeviceNotFound(QString internalName, SoundManager *sm)
        : SoundDevice(UserSettingsPointer(), sm)
{
    m_strInternalName = internalName;
    m_strDisplayName = internalName;
}
SoundDeviceError SoundDeviceNotFound::open(bool isClkRefDevice, int syncBuffers)
{
    Q_UNUSED(isClkRefDevice);
    Q_UNUSED(syncBuffers);
    return SOUNDDEVICE_ERROR_ERR;
};
bool SoundDeviceNotFound::isOpen() const
{
    return false;
}
SoundDeviceError SoundDeviceNotFound::close()
{
    return SOUNDDEVICE_ERROR_ERR;
};
void SoundDeviceNotFound::readProcess() { };
void SoundDeviceNotFound::writeProcess() { };
QString SoundDeviceNotFound::getError() const { return QObject::tr("Device not found"); };

unsigned int SoundDeviceNotFound::getDefaultSampleRate() const
{
    return 44100;
}

