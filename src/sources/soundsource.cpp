#include "sources/soundsource.h"
#include "metadata/trackmetadata.h"
#include "metadata/trackmetadatataglib.h"
#include "sampleutil.h"
#include "samplebuffer.h"
namespace Mixxx {

void SoundSource::clampFrameInterval(
        SINT& pMinFrameIndexOfInterval,
        SINT& pMaxFrameIndexOfInterval,
        SINT maxFrameIndexOfAudioSource)
{
    if (pMinFrameIndexOfInterval < 0) pMinFrameIndexOfInterval = 0;
    if (pMaxFrameIndexOfInterval > maxFrameIndexOfAudioSource) pMaxFrameIndexOfInterval = maxFrameIndexOfAudioSource;
    if (pMaxFrameIndexOfInterval < pMinFrameIndexOfInterval) pMaxFrameIndexOfInterval = pMinFrameIndexOfInterval;
}
SoundSource::SoundSource(QUrl url)
        : m_url(url),
          m_type(getFileExtensionFromUrl(url)),
          m_channelCount(kChannelCountDefault),
          m_frameRate(kFrameRateDefault),
          m_frameCount(kFrameCountDefault),
          m_bitrate(kBitrateDefault)
{
}
SoundSource::SoundSource(QUrl url, QString type)
        : m_url(url),
          m_channelCount(kChannelCountDefault),
          m_frameRate(kFrameRateDefault),
          m_frameCount(kFrameCountDefault),
          m_bitrate(kBitrateDefault),
          m_type(type)
{
    DEBUG_ASSERT(getUrl().isValid());
}
/*static*/ QString SoundSource::getFileExtensionFromUrl(QUrl url) {
    return url.toString().section(".", -1).toLower().trimmed();
}
void SoundSource::setChannelCount(SINT channelCount) {
    DEBUG_ASSERT(isValidChannelCount(channelCount));
    m_channelCount = channelCount;
}
void SoundSource::setFrameRate(SINT frameRate) {
    DEBUG_ASSERT(isValidFrameRate(frameRate));
    m_frameRate = frameRate;
}
void SoundSource::setFrameCount(SINT frameCount) {
    DEBUG_ASSERT(isValidFrameCount(frameCount));
    m_frameCount = frameCount;
}
void SoundSource::setBitrate(SINT bitrate) {
    DEBUG_ASSERT(isValidBitrate(bitrate));
    m_bitrate = bitrate;
}
SINT SoundSource::getSampleBufferSize( SINT numberOfFrames, bool readStereoSamples) const {
    if (readStereoSamples) { return numberOfFrames * kChannelCountStereo;}
    else { return frames2samples(numberOfFrames); }
}
SINT SoundSource::readSampleFramesStereo( SINT numberOfFrames, CSAMPLE* sampleBuffer, SINT sampleBufferSize) {
    DEBUG_ASSERT(getSampleBufferSize(numberOfFrames, true) <= sampleBufferSize);
    switch (getChannelCount()) {
        case 1: // mono channel
        {
            auto readFrameCount = readSampleFrames( numberOfFrames, sampleBuffer);
            SampleUtil::doubleMonoToDualMono(sampleBuffer, readFrameCount);
            return readFrameCount;
        }
        case 2: // stereo channel(s)
        {
            return readSampleFrames(numberOfFrames, sampleBuffer);
        }
        default: // multiple (3 or more) channels
        {
            auto numberOfSamplesToRead = frames2samples(numberOfFrames);
            if (numberOfSamplesToRead <= sampleBufferSize) {
                // efficient in-place transformation
                auto readFrameCount = readSampleFrames( numberOfFrames, sampleBuffer);
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
                auto readFrameCount = readSampleFrames( numberOfFrames, tempBuffer.data());
                SampleUtil::copyMultiToStereo(sampleBuffer, tempBuffer.data(),readFrameCount, getChannelCount());
                return readFrameCount;
            }
        }
    }
}
SINT SoundSource::readSampleFramesStereo( SINT numberOfFrames, SampleBuffer* pSampleBuffer)
{
        if (pSampleBuffer) { return readSampleFramesStereo(numberOfFrames, pSampleBuffer->data(), pSampleBuffer->size()); }
        else { return skipSampleFrames(numberOfFrames); }
}
SINT SoundSource::readSampleFrames(SINT numberOfFrames, SampleBuffer*pSampleBuffer)
{
  if ( pSampleBuffer ) {
    numberOfFrames = std::min<SINT>(numberOfFrames,pSampleBuffer->size());
    return readSampleFrames(numberOfFrames,pSampleBuffer->data());
  }else{return skipSampleFrames(numberOfFrames);}
}
SINT SoundSource::skipSampleFrames( SINT numberOfFrames )
{
  return readSampleFrames(numberOfFrames,static_cast<CSAMPLE*>(nullptr));
}
SINT SoundSource::getMinFrameIndex()const
{
  return kFrameIndexMin;
}
SINT SoundSource::getMaxFrameIndex() const 
{
  return getMinFrameIndex() + getFrameCount();
}
bool SoundSource::isValidFrameIndex(SINT frameIndex ) const
{
  return (getMinFrameIndex() <= frameIndex ) && ( frameIndex <= getMaxFrameIndex());
}
SINT SoundSource::getChannelCount()const{return m_channelCount;}
SINT SoundSource::getFrameRate()const{return m_frameRate;}
bool SoundSource::isValid() const{return hasChannelCount() && hasFrameRate();}
SINT SoundSource::getFrameCount() const { return m_frameCount;}
bool SoundSource::isEmpty() const{return getFrameCount() <= 0;}
bool SoundSource::hasDuration() const{return isValid() && !isEmpty();}
double SoundSource::getDuration() const{ if ( !getFrameRate() ) return 0; else return getFrameCount()/getFrameRate();}
bool SoundSource::hasBitrate() const{return isValidBitrate(m_bitrate);}
/* static */
bool SoundSource::isValidBitrate(SINT bitrate){return bitrate > 0;}
SINT SoundSource::getBitrate()const{return m_bitrate;}
SINT SoundSource::frames2samples(SINT frameCount) const
{
  return frameCount * getChannelCount();
}
SINT SoundSource::samples2frames(SINT sampleCount) const
{
  DEBUG_ASSERT ( hasChannelCount() );
  DEBUG_ASSERT ( sampleCount % getChannelCount() == 0 );
  return sampleCount / getChannelCount();
}
/* static */
bool SoundSource::isValidChannelCount(SINT channelCount){return channelCount>0;}
bool SoundSource::hasChannelCount()const{return isValidChannelCount(getChannelCount());}
/*static*/
bool SoundSource::isValidFrameRate(SINT frameRate){return frameRate>0;}
bool SoundSource::hasFrameRate() const{return isValidFrameRate(getFrameRate());}
/*static*/
bool SoundSource::isValidFrameCount(SINT n){return n>0;}
QUrl SoundSource::getUrl() const
{
  return m_url;
}
QString SoundSource::getUrlString() const
{
  return m_url.toString();
}
bool SoundSource::isLocalFile() const
{
  return getUrl().isLocalFile();
}
QString SoundSource::getLocalFileName() const
{
  if ( isLocalFile() ) return getUrl().toLocalFile();
  else return QString{};
}
QByteArray SoundSource::getLocalFileNameBytes() const
{
  return getLocalFileName().toLocal8Bit();
}
QString SoundSource::getType() const
{
  return m_type;
}
bool SoundSource::open(SoundSourceConfig audioSrcCfg) {
    close(); // reopening is not supported
    auto result = false;
    try { result = tryOpen(audioSrcCfg); }
    catch (...) {
        close();
        throw;
    }
    if (!result) { close(); }
    return result;
}
bool SoundSource::parseTrackMetadataAndCoverArt( TrackMetadata* pTrackMetadata, QImage* pCoverArt) const {
    return readTrackMetadataAndCoverArtFromFile(pTrackMetadata, pCoverArt, getLocalFileName());
}

}
