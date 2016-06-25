_Pragma("once")
#include "sources/soundsourceprovider.h"

#include <encoder/encoderffmpegresample.h>

// Needed to ensure that macros in <stdint.h> get defined.

#include <cstdint>
#include <vector>
#include <atomic>
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
};
//#include <libavutil/opt.h>
//#include <libavutil/mathematics.h>


namespace mixxx {

class SoundSourceFFmpeg : public SoundSource {
  public:
    explicit SoundSourceFFmpeg(QUrl url);
    virtual ~SoundSourceFFmpeg();

    void close() override;
    virtual SINT seekSampleFrame(SINT frameIndex) override;
    virtual SINT readSampleFrames(SINT numberOfFrames, CSAMPLE* sampleBuffer) override;
  private:
    Result tryOpen(const AudioSourceConfig& audioSrcCfg) override;

    bool getNextFrame( );
    AVFormatContext      *m_fmt_ctx    = nullptr;
    AVCodecContext       *m_dec_ctx    = nullptr;
    AVCodec              *m_dec        = nullptr;
    AVStream             *m_stream     = nullptr;
    int                   m_stream_idx = -1;
    AVRational            m_stream_tb  { 0, 1 };
    SwrContext           *m_swr        = nullptr;
    AVFrame              *m_frame  = nullptr;
    AVPacket              m_packet;
    AVPacket              m_packet_free;
    SINT                  m_offset_base = 0;
    SINT                  m_offset_cur  = 0;
};
class SoundSourceProviderFFmpeg: public SoundSourceProvider {
  public:
    QString getName() const override {return "FFmpeg";}
    QStringList getSupportedFileExtensions() const override;
    SoundSourcePointer newSoundSource(const QUrl& url) override {
        return SoundSourcePointer(new SoundSourceFFmpeg(url));
    }
};

} // namespace mixxx

#endif // MIXXX_SOUNDSOURCEFFMPEG_H
