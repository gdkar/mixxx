_Pragma("once")
#include "sources/soundsourceprovider.h"

// Needed to ensure that macros in <stdint.h> get defined.
#include "util/ffmpeg-utils.hpp"
#include <cstdint>
#include <vector>
#include <deque>
#include <atomic>
//#include <libavutil/opt.h>
//#include <libavutil/mathematics.h>


namespace mixxx {

class SoundSourceFFmpeg : public SoundSource {
  public:
    explicit SoundSourceFFmpeg(QUrl url);
   ~SoundSourceFFmpeg();
    void close() override;
    int64_t seekSampleFrame(int64_t frameIndex) override;
    int64_t readSampleFrames(int64_t numberOfFrames, CSAMPLE* sampleBuffer) override;
    Result parseTrackMetadataAndCoverArt(
            TrackMetadata* pTrackMetadata,
            QImage* pCoverArt) const override;

  private:
    OpenResult tryOpen(const AudioSourceConfig &audioSrcCfg) override;
    bool    decode_next_frame();
    format_context         m_format_ctx{};
    AVStream              *m_stream{};
    int                    m_stream_index{ -1};
    codec_context          m_codec_ctx{};
    AVCodec               *m_codec = nullptr;
    avpacket               m_packet{};
    std::deque<avpacket>   m_pkt_array{0};
    int64_t                m_pkt_index    = 0;
    avframe                m_orig_frame{};
    avframe                m_frame{};
    swr_context            m_swr{};
    AVRational             m_stream_tb    = { 0, 1 };
    AVRational             m_codec_tb     = { 0, 1 };
    AVRational             m_output_tb    = { 0, 1 };
    int64_t                m_offset       = 0;
    int64_t                m_first_pts    = 0;
};
class SoundSourceProviderFFmpeg: public SoundSourceProvider {
  public:
    QString getName() const override;
    QStringList getSupportedFileExtensions() const override;
    bool canOpen(QUrl url) override;
    bool canOpen(QString path) override;
    SoundSourcePointer newSoundSource(QUrl url) override;
};
} // namespace mixxx
