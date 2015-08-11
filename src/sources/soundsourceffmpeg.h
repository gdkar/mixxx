_Pragma("once")
#include "sources/soundsourceprovider.h"

#include <encoder/encoderffmpegresample.h>

// Needed to ensure that macros in <stdint.h> get defined.

#include <cstdint>
#include <vector>
#include <atomic>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
//#include <libavutil/opt.h>
//#include <libavutil/mathematics.h>


namespace Mixxx {

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVStream;
struct SwrContext;
class SoundSourceFFmpeg : public SoundSource {
  public:
    explicit SoundSourceFFmpeg(QUrl url);
    virtual ~SoundSourceFFmpeg();

    void close() override;
    virtual SINT seekSampleFrame(SINT frameIndex) override;
    virtual SINT readSampleFrames(SINT numberOfFrames, CSAMPLE* sampleBuffer) override;
  private:
    Result tryOpen(const AudioSourceConfig& audioSrcCfg) override;

    bool readFramesToCache(unsigned int count, SINT offset);
    bool getBytesFromCache(CSAMPLE* buffer, SINT offset, SINT size);
    SINT getSizeofCache();
    void clearCache();

    AVFormatContext *m_fmt_ctx    = nullptr;
    AVStream        *m_stream     = nullptr;
    int              m_stream_idx = -1;
    AVCodecContext  *m_codec_ctx  = nullptr;
    AVCodec         *m_codec      = nullptr;
    SwrContext      *m_swr        = nullptr;
    vector<AVPacket> m_packets{0};
    AVPacket         m_cur_pkt;
    SINT             m_cur_pkt_idx= 0;
    SINT             m_cur_pts    = 0;
    AVFrame         *m_cur_frame;
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
