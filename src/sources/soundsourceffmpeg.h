_Pragma("once")
#include "sources/soundsourceprovider.h"

// Needed to ensure that macros in <stdint.h> get defined.
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include "sources/ffmpeg_util.h"
#include <cstdint>
#include <vector>
#include <atomic>
//#include <libavutil/opt.h>
//#include <libavutil/mathematics.h>


namespace Mixxx {

class SoundSourceFFmpeg : public SoundSource {
  public:
    explicit SoundSourceFFmpeg(QUrl url);
    virtual ~SoundSourceFFmpeg();
    virtual void close() override;
    virtual SINT seekSampleFrame(SINT frameIndex) override;
    virtual SINT readSampleFrames(SINT numberOfFrames, CSAMPLE* sampleBuffer) override;
  private:
    bool    tryOpen(const AudioSourceConfig& audioSrcCfg) override;
    bool    decode_next_frame();
    AVFormatContext       *m_format_ctx   = nullptr;
    AVStream              *m_stream       = nullptr;
    int                    m_stream_index = -1;
    AVCodecContext        *m_codec_ctx    = nullptr;
    AVCodec               *m_codec        = nullptr;
    AVPacket               m_packet;
    std::vector<AVPacket>  m_pkt_array{0};
    int64_t                m_pkt_index    = 0;
    AVFrame               *m_orig_frame   = nullptr;
    AVFrame               *m_frame        = nullptr;
    SwrContext            *m_swr          = nullptr;
    AVRational             m_stream_tb    = { 0, 1 };
    AVRational             m_codec_tb     = { 0, 1 };
    AVRational             m_output_tb    = { 0, 1 };
    int64_t                m_offset       = 0;
    int64_t                m_first_pts    = 0;
};
class SoundSourceProviderFFmpeg: public SoundSourceProvider {
  public:
    QString getName() const override {return "FFmpeg";}
    QStringList getSupportedFileExtensions() const override;
    SoundSourcePointer newSoundSource(const QUrl& url) override {
        return SoundSourcePointer(new SoundSourceFFmpeg(url));
    }
};
} // namespace Mixxx
