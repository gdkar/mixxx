_Pragma("once")

#include "sources/soundsource.h"
#include "sources/soundsourceprovider.h"

// Needed to ensure that macros in <stdint.h> get defined.

#include <cstdint>
#include <vector>
#include <deque>
#include <atomic>

extern "C"{
#   include <libavcodec/avcodec.h>
#   include <libavformat/avformat.h>
#   include <libavcodec/avcodec.h>
#   include <libswresample/swresample.h>
#   include <libavdevice/avdevice.h>
#   include <libavutil/avutil.h>
};
//#include <libavutil/opt.h>
//#include <libavutil/mathematics.h>


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
    struct packet {
        AVPacket *m_d{nullptr};
        AVPacket       *operator ->()       { return m_d;}
        const AVPacket *operator ->() const { return m_d;}
              AVPacket &operator  *()       { return *m_d;}
        const AVPacket &operator  *() const { return *m_d;}
        operator const AVPacket *  () const{ return m_d;}
        operator       AVPacket *  ()      { return m_d;}
        operator bool() const { return !!m_d;}
        bool operator !() const { return !m_d;}
        packet() : m_d{av_packet_alloc()}{}
        packet(packet && o) noexcept { swap(o);}
        packet &operator =(packet && o) noexcept { swap(o);return *this;}
       ~packet() { av_packet_free(&m_d);}
        void swap(packet &o) noexcept { std::swap(m_d, o.m_d);}
        void unref() { av_packet_unref(m_d);}
    };
    struct frame{
        AVFrame *m_d{nullptr};
        AVFrame *operator ->()       { return m_d;}
        const AVFrame *operator ->() const { return m_d;}
              AVFrame &operator  *()       { return *m_d;}
        const AVFrame &operator  *() const { return *m_d;}
        operator const AVFrame *  () const{ return m_d;}
        operator       AVFrame *  ()      { return m_d;}
        operator bool() const { return !!m_d;}
        bool operator !() const { return !m_d;}
        frame() : m_d{av_frame_alloc()}{}
        frame(frame && o) noexcept { swap(o);}
        frame &operator =(frame && o) noexcept { swap(o);return *this;}
       ~frame() { av_frame_free(&m_d);}
        void swap(frame &o) noexcept { std::swap(m_d, o.m_d);}
        void unref() { av_frame_unref(m_d);}
    };
    OpenResult tryOpen(const AudioSourceConfig& audioSrcCfg) override;

    bool next( );
    AVFormatContext      *m_fmt_ctx    = nullptr;
    AVCodecContext       *m_codec_ctx    = nullptr;
    AVCodec              *m_codec        = nullptr;
    AVStream             *m_stream     = nullptr;
    SwrContext           *m_swr        = nullptr;
    frame                 m_frame_dec{};
    frame                 m_frame_swr{};
    packet                m_packet{};
    std::vector<packet>   m_first_packets{};
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
