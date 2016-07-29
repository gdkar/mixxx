_Pragma("once")

#include "sources/soundsource.h"
#include "sources/soundsourceprovider.h"

// Needed to ensure that macros in <stdint.h> get defined.

#include <cstdint>
#include <functional>
#include <utility>
#include <algorithm>
#include <iterator>
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
struct swr_context {
    SwrContext *m_d{nullptr};
    SwrContext     *operator->()            {return m_d;}
    const SwrContext *operator->() const    {return m_d;}
    SwrContext    &operator *()             {return *m_d;}
    const SwrContext    &operator *() const {return *m_d;}
    operator const SwrContext *() const { return m_d;}
    operator SwrContext *() { return m_d;}
    swr_context() : m_d ( swr_alloc()) {}
    swr_context(swr_context &&o) noexcept : swr_context() { swap(o); }
    swr_context &operator=(swr_context &&o) noexcept { swap(o); return *this;}
    ~swr_context() { swr_free(&m_d);}
    void swap(swr_context & o) noexcept { using std::swap; swap(m_d, o.m_d);}
    void set_opts(
        int64_t        ocl,
        AVSampleFormat osf,
        int            osr,
        int64_t        icl,
        AVSampleFormat isf,
        int            isr,
        int log_off    = 0,
        void *log_ctx  = nullptr )
    {
        m_d = swr_alloc_set_opts(
            m_d,
            ocl, osf, osr,
            icl, isf, isr,
            log_off, log_ctx);
    }
    int init() { return swr_init(m_d);}
    bool initialized() const { return swr_is_initialized(m_d);}
    int delay(int rate) { return swr_get_delay(m_d,rate); }
    int config(AVFrame *to, AVFrame *from) { return swr_config_frame(m_d, to, from); }
    int convert(AVFrame *to, AVFrame *from) { return swr_convert_frame(m_d, to, from); }
    int convert(uint8_t **out, int out_count, const uint8_t **in, int in_count)
    {
        return swr_convert(m_d, out, out_count,in, in_count);
    }
    int64_t next_pts(int64_t pts) { return swr_next_pts(m_d, pts);}
    int set_compensation(int sample_delta, int comp_dist)
    {
        return swr_set_compensation(m_d,sample_delta,comp_dist);
    }
    int drop_output(int count) { return swr_drop_output(m_d,count);}
    int inject_silence(int count) { return swr_inject_silence(m_d,count);}
    int out_samples(int in_samples) { return swr_get_out_samples(m_d, in_samples);}

};
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
    frame(frame && o) noexcept : frame(){ swap(o);}
    frame &operator =(frame && o) noexcept { swap(o);return *this;}
    ~frame() { av_frame_free(&m_d);}
    void swap(frame &o) noexcept { std::swap(m_d, o.m_d);}
    void unref() { av_frame_unref(m_d);}
};
struct format_context{
    AVFormatContext  *m_d{nullptr};
    AVFormatContext *operator ->()       { return m_d;}
    const AVFormatContext *operator ->() const { return m_d;}
            AVFormatContext &operator  *()       { return *m_d;}
    const AVFormatContext &operator  *() const { return *m_d;}
    operator const AVFormatContext *  () const{ return m_d;}
    operator       AVFormatContext *  ()      { return m_d;}
    operator bool() const { return !!m_d;}
    bool operator !() const { return !m_d;}
    format_context() : m_d{avformat_alloc_context()}{}
    format_context(format_context && o) noexcept: format_context() { swap(o);}
    format_context &operator =(format_context && o) noexcept { swap(o);return *this;}
   ~format_context() { avformat_close_input(&m_d);}
    void swap(format_context &o) noexcept { std::swap(m_d, o.m_d);}
    std::pair<packet,int> read_frame() { auto p = std::make_pair(packet{},0); if((p.second = read_frame(p.first)) < 0) { p.first.unref();} return p;}
    int read_frame(AVPacket *pkt) { return av_read_frame(m_d,pkt);}
    void close()
    {
        avformat_close_input(&m_d);
        m_d = avformat_alloc_context();
    }
    int open_input( const char *path)
    {
        auto err = 0;
        if(!(err = avformat_open_input(&m_d, path,nullptr,nullptr))
        && (err = avformat_find_stream_info(m_d,nullptr)) >= 0) {
            std::for_each(&m_d->streams[0],&m_d->streams[m_d->nb_streams],[](auto *stream)
                    { stream->discard = AVDISCARD_ALL;});
        }
        return err;
    }
    std::pair<AVStream*, AVCodec*> find_best_stream(AVMediaType type)
    {
        auto ret = 0;
        auto res = std::pair<AVStream*,AVCodec*>{};
        if ( ( ret = av_find_best_stream ( m_d, type, -1, -1, &res.second, 0 ) ) >= 0 ){
            res.first = m_d->streams[ret];
            if(!res.second)
                res.second = avcodec_find_decoder ( res.first->codecpar->codec_id ) ;
            res.first->discard = AVDISCARD_NONE;
        }
        return res;
    }
    int seek_file(int stream_idx, int64_t min_pts, int64_t pts, int64_t max_pts, int flags = 0)
    {
        return avformat_seek_file(m_d, stream_idx, min_pts, pts, max_pts, flags);
    }
};
struct codec_context{
    AVCodecContext *m_d{nullptr};
    AVCodecContext *operator ->()       { return m_d;}
    const AVCodecContext *operator ->() const { return m_d;}
            AVCodecContext &operator  *()       { return *m_d;}
    const AVCodecContext &operator  *() const { return *m_d;}
    operator const AVCodecContext *  () const{ return m_d;}
    operator       AVCodecContext *  ()      { return m_d;}
    operator bool() const { return !!m_d;}
    bool operator !() const { return !m_d;}
    explicit codec_context(AVCodec *dec= nullptr) : m_d{avcodec_alloc_context3(dec)}{}
    explicit codec_context(AVCodecParameters *par) : codec_context(avcodec_find_decoder(par->codec_id))
    {
        avcodec_parameters_to_context(m_d,par);
    }
    int open( AVCodec *codec = nullptr) { return avcodec_open2(m_d, codec, nullptr); }
    void close() { if ( avcodec_is_open(m_d)) avcodec_close(m_d);}
    codec_context(codec_context && o) noexcept : codec_context(){ swap(o);}
    codec_context &operator =(codec_context && o) noexcept { swap(o);return *this;}
   ~codec_context()
    {
        close();
        avcodec_free_context(&m_d);
    }
    int send_packet(AVPacket *pkt) { return avcodec_send_packet(m_d,pkt); }
    int receive_frame(AVFrame *frm) { return avcodec_receive_frame(m_d,frm);}
    void flush_buffers() { avcodec_flush_buffers(m_d);}
    void swap(codec_context &o) noexcept { std::swap(m_d, o.m_d);}
};


    OpenResult tryOpen(const AudioSourceConfig& audioSrcCfg) override;

    bool next( );
    format_context        m_fmt_ctx{};
    codec_context         m_codec_ctx{};
    AVCodec              *m_codec      = nullptr;
    AVStream             *m_stream     = nullptr;
    swr_context           m_swr;
    frame                 m_frame_dec{};
    frame                 m_frame_swr{};
    packet                m_packet{};
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
