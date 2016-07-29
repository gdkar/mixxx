_Pragma("once")

#include "sources/soundsource.h"
#include "sources/soundsourceprovider.h"

// Needed to ensure that macros in <stdint.h> get defined.

#include "util/ffmpeg-utils.h"

namespace mixxx {

class SoundSourceFFmpeg : public SoundSource {
  public:
    explicit SoundSourceFFmpeg(QUrl url);
    virtual ~SoundSourceFFmpeg() override;

    void close() override;
    int64_t seekSampleFrame(int64_t frameIndex) override;
    int64_t readSampleFrames(int64_t numberOfFrames, CSAMPLE* sampleBuffer) override;
    int64_t getChannelLayout() const;
  private:


    OpenResult tryOpen(const AudioSourceConfig& audioSrcCfg) override;

    bool next( );
    format_context        m_fmt_ctx{};
    codec_context         m_codec_ctx{};
    AVCodec              *m_codec      = nullptr;
    AVStream             *m_stream     = nullptr;
    swr_context           m_swr;
    avframe                 m_frame_dec{};
    avframe                 m_frame_swr{};
    avpacket                m_packet{};
    AVRational            m_stream_tb  { 0, 1 };
    AVRational            m_output_tb  { 0, 1 };
    int64_t               m_pts_origin    = AV_NOPTS_VALUE;
    int64_t               m_sample_origin = 0;
    int64_t               m_sample_frame  = 0;
    int64_t               m_sample_now    = 0;
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
