#include "sources/audiosource.h"

#include "sampleutil.h"
#include "samplebuffer.h"
namespace Mixxx {

void AudioSource::clampFrameInterval(
        SINT& pMinFrameIndexOfInterval,
        SINT& pMaxFrameIndexOfInterval,
        SINT maxFrameIndexOfAudioSource) {
    if (pMinFrameIndexOfInterval < 0) { pMinFrameIndexOfInterval = 0; }
    if (pMaxFrameIndexOfInterval > maxFrameIndexOfAudioSource) { pMaxFrameIndexOfInterval = maxFrameIndexOfAudioSource; }
    if (pMaxFrameIndexOfInterval < pMinFrameIndexOfInterval) { pMaxFrameIndexOfInterval = pMinFrameIndexOfInterval; }
}
AudioSource::AudioSource(const QUrl& url)
        : UrlResource(url),
          m_channelCount(kChannelCountDefault),
          m_frameRate(kFrameRateDefault),
          m_frameCount(kFrameCountDefault),
          m_bitrate(kBitrateDefault) {
}
void AudioSource::setChannelCount(SINT channelCount) {
    DEBUG_ASSERT(isValidChannelCount(channelCount));
    m_channelCount = channelCount;
}
void AudioSource::setFrameRate(SINT frameRate) {
    DEBUG_ASSERT(isValidFrameRate(frameRate));
    m_frameRate = frameRate;
}
void AudioSource::setFrameCount(SINT frameCount) {
    DEBUG_ASSERT(isValidFrameCount(frameCount));
    m_frameCount = frameCount;
}
void AudioSource::setBitrate(SINT bitrate) {
    DEBUG_ASSERT(isValidBitrate(bitrate));
    m_bitrate = bitrate;
}
SINT AudioSource::getSampleBufferSize( SINT numberOfFrames, bool readStereoSamples) const {
    if (readStereoSamples) { return numberOfFrames * kChannelCountStereo;}
    else { return frames2samples(numberOfFrames); }
}
SINT AudioSource::readSampleFramesStereo( SINT numberOfFrames, CSAMPLE* sampleBuffer, SINT sampleBufferSize) {
    DEBUG_ASSERT(getSampleBufferSize(numberOfFrames, true) <= sampleBufferSize);
    switch (getChannelCount()) {
        case 1: // mono channel
        {
            const SINT readFrameCount = readSampleFrames( numberOfFrames, sampleBuffer);
            SampleUtil::doubleMonoToDualMono(sampleBuffer, readFrameCount);
            return readFrameCount;
        }
        case 2: // stereo channel(s)
        {
            return readSampleFrames(numberOfFrames, sampleBuffer);
        }
        default: // multiple (3 or more) channels
        {
            const SINT numberOfSamplesToRead = frames2samples(numberOfFrames);
            if (numberOfSamplesToRead <= sampleBufferSize) {
                // efficient in-place transformation
                const SINT readFrameCount = readSampleFrames( numberOfFrames, sampleBuffer);
                SampleUtil::copyMultiToStereo(sampleBuffer, sampleBuffer, readFrameCount, getChannelCount());
                return readFrameCount;
            } else {
                // inefficient transformation through a temporary buffer
                qDebug() << "Performance warning:"
                        << "Allocating a temporary buffer of size"
                        << numberOfSamplesToRead << "for reading stereo samples."
                        << "The size of the provided sample buffer is"
                        << sampleBufferSize;
                SampleBuffer tempBuffer(numberOfSamplesToRead);
                const SINT readFrameCount = readSampleFrames( numberOfFrames, tempBuffer.data());
                SampleUtil::copyMultiToStereo(sampleBuffer, tempBuffer.data(),readFrameCount, getChannelCount());
                return readFrameCount;
            }
        }
    }
}
SINT AudioSource::readSampleFramesStereo( SINT numberOfFrames, SampleBuffer* pSampleBuffer)
{
        if (pSampleBuffer) { return readSampleFramesStereo(numberOfFrames, pSampleBuffer->data(), pSampleBuffer->size()); }
        else { return skipSampleFrames(numberOfFrames); }
}
SINT AudioSource::readSampleFrames(SINT numberOfFrames, SampleBuffer*pSampleBuffer)
{
  if ( pSampleBuffer ) {
    numberOfFrames = std::min<SINT>(numberOfFrames,pSampleBuffer->size());
    return readSampleFrames(numberOfFrames,pSampleBuffer->data());
  }else{return skipSampleFrames(numberOfFrames);}
}
SINT AudioSource::skipSampleFrames( SINT numberOfFrames )
{
  return readSampleFrames(numberOfFrames,static_cast<CSAMPLE*>(nullptr));
}
SINT AudioSource::getMinFrameIndex()const
{
  return kFrameIndexMin;
}
SINT AudioSource::getMaxFrameIndex() const 
{
  return getMinFrameIndex() + getFrameCount();
}
bool AudioSource::isValidFrameIndex(SINT frameIndex ) const
{
  return (getMinFrameIndex() <= frameIndex ) && ( frameIndex <= getMaxFrameIndex());
}
SINT AudioSource::getChannelCount()const{return m_channelCount;}
SINT AudioSource::getFrameRate()const{return m_frameRate;}
bool AudioSource::isValid() const{return hasChannelCount() && hasFrameRate();}
SINT AudioSource::getFrameCount() const { return m_frameCount;}
bool AudioSource::isEmpty() const{return getFrameCount() <= 0;}
bool AudioSource::hasDuration() const{return isValid() && !isEmpty();}
double AudioSource::getDuration() const{ if ( !getFrameRate() ) return 0; else return getFrameCount()/getFrameRate();}
bool AudioSource::hasBitrate() const{return isValidBitrate(m_bitrate);}
/* static */
bool AudioSource::isValidBitrate(SINT bitrate){return bitrate > 0;}
SINT AudioSource::getBitrate()const{return m_bitrate;}
SINT AudioSource::frames2samples(SINT frameCount) const
{
  return frameCount * getChannelCount();
}
SINT AudioSource::samples2frames(SINT sampleCount) const
{
  DEBUG_ASSERT ( hasChannelCount() );
  DEBUG_ASSERT ( sampleCount % getChannelCount() == 0 );
  return sampleCount / getChannelCount();
}
/* static */
bool AudioSource::isValidChannelCount(SINT channelCount){return channelCount>0;}
bool AudioSource::hasChannelCount()const{return isValidChannelCount(getChannelCount());}
/*static*/
bool AudioSource::isValidFrameRate(SINT frameRate){return frameRate>0;}
bool AudioSource::hasFrameRate() const{return isValidFrameRate(getFrameRate());}
/*static*/
bool AudioSource::isValidFrameCount(SINT n){return n>0;}
}
