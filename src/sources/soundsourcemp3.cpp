
#include "sources/soundsourcemp3.h"
#include "sources/soundsource.h"

#include "util/math.h"

#include <id3tag.h>

namespace mixxx {

namespace {

    // MP3 does only support 1 or 2 channels
    const SINT kChannelCountMax = AudioSource::kChannelCountStereo;

    // Optimization: Reserve initial capacity for seek frame list
    const SINT kMinutesPerFile = 10; // enough for the majority of files (tunable)
    const SINT kSecondsPerMinute = 60; // fixed
    const SINT kMaxMp3FramesPerSecond = 39; // fixed: 1 MP3 frame = 26 ms -> ~ 1000 / 26
    const SINT kSeekFrameListCapacity = kMinutesPerFile
            * kSecondsPerMinute * kMaxMp3FramesPerSecond;
}
SoundSourceMp3::SoundSourceMp3(const QUrl&url)
: SoundSource(url)
, m_h(nullptr)
{
    mpg123_init();
}
SoundSourceMp3::~SoundSourceMp3()
{
    close();
    mpg123_exit();
}

SoundSource::OpenResult SoundSourceMp3::tryOpen(const AudioSourceConfig& /*audioSrcCfg*/)
{
    close();
    m_h = mpg123_new(nullptr,nullptr);

    if(!m_h)
        return OpenResult::FAILED;

    DEBUG_ASSERT(!hasValidChannelCount());
    DEBUG_ASSERT(!hasValidSamplingRate());

    auto maxChannelCount = getChannelCount();

    if(mpg123_param(
        m_h
       ,MPG123_ADD_FLAGS
       ,(MPG123_FORCE_STEREO
        |MPG123_FORCE_FLOAT
        |MPG123_GAPLESS
        |MPG123_SKIP_ID3V2)
       ,0.0f)!=MPG123_OK)
        return OpenResult::FAILED;
    if(mpg123_param(m_h,MPG123_VERBOSE,2,0.0f) != MPG123_OK)
        return OpenResult::FAILED;
    if(mpg123_param(m_h,MPG123_INDEX_SIZE,-1,0.0f) != MPG123_OK)
        return OpenResult::FAILED;
    
    if(mpg123_open(m_h,getLocalFileName().toLocal8Bit().constData()) != MPG123_OK)
        return OpenResult::FAILED;

    auto nch = 0,enc = 0;
    auto rate = 0l;
    if(mpg123_getformat(m_h,&rate,&nch,&enc) != MPG123_OK)
        return OpenResult::FAILED;
    if(mpg123_format_none(m_h)!=MPG123_OK)
        return OpenResult::FAILED;
    if(enc != MPG123_ENC_FLOAT_32)
        return OpenResult::FAILED;
    if(mpg123_format(m_h,rate,nch,enc) != MPG123_OK) 
        return OpenResult::FAILED;
    if(mpg123_scan(m_h)!=MPG123_OK)
        return OpenResult::FAILED;
    setSamplingRate(rate);
    setChannelCount(nch);
    setFrameCount(mpg123_length(m_h));
    return OpenResult::SUCCEEDED;
}

void SoundSourceMp3::close()
{
    if(m_h) {
        mpg123_close(m_h);
        mpg123_delete(m_h);
        m_h = nullptr;
    }
}
SINT SoundSourceMp3::seekSampleFrame(SINT frameIndex)
{
    mpg123_seek(m_h, frameIndex,SEEK_SET);
    return mpg123_tell(m_h);
}
SINT SoundSourceMp3::readSampleFrames(
        SINT numberOfFrames, CSAMPLE* sampleBuffer)
{
    auto done = size_t{0};
    auto ret  = 0;
    auto outmemsize = getSampleBufferSize(numberOfFrames,false) * sizeof(CSAMPLE);
    auto outmem = reinterpret_cast<uint8_t*>(sampleBuffer);
    do{
        ret = mpg123_read(m_h,outmem,outmemsize,&done);
        if(ret == MPG123_OK) {
            outmem += done;
            outmemsize -= done;
        }
    }while(outmemsize && done && ret==MPG123_OK);
    return numberOfFrames - samples2frames(outmemsize/sizeof(CSAMPLE));
}
QString SoundSourceProviderMp3::getName() const
{
    return "mpg123: MPEG 1 Layers 1/2/3 Audio Decoder";
}
QStringList SoundSourceProviderMp3::getSupportedFileExtensions() const
{
    QStringList supportedFileExtensions;
    supportedFileExtensions.append("mp3");
    return supportedFileExtensions;
}

} // namespace mixxx
