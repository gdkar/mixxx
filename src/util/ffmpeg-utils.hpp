
_Pragma("once")
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
#   include <libavfilter/avfilter.h>
#   include <libavdevice/avdevice.h>
#   include <libavutil/avutil.h>
#   include <libavutil/mathematics.h>
#   include <libavutil/common.h>
#   include <libavutil/dict.h>
#   include <libavutil/opt.h>
};
namespace mixxx {

inline QString av_strerror(int errnum)
{
    char buf[AV_ERROR_MAX_STRING_SIZE + 1];
    ::av_strerror(errnum, buf, sizeof(buf));
    return QString{buf};
}
inline std::ostream &operator << (std::ostream &ost, const AVMediaType &type)
{
    return (ost << av_get_media_type_string(type));
}
inline std::ostream &operator << (std::ostream &ost, const AVCodecID id)
{
    return (ost << avcodec_get_name(id));
}
inline std::ostream &operator << (std::ostream &ost, AVCodecContext &enc)
{
    char buf[1024];
    avcodec_string(buf,sizeof(buf),&enc,av_codec_is_encoder(enc.codec));
    return (ost << buf);
}
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
    void close() { swr_close(m_d);}
    bool initialized() const { return swr_is_initialized(m_d);}
    int delay(int rate)
    {
        return swr_get_delay(m_d,rate);
    }
    int config(AVFrame *to, AVFrame *from)
    {
        return swr_config_frame(m_d, to, from);
    }
    int convert(AVFrame *to, AVFrame *from)
    {
        if(!to || !from)
            return AVERROR(EFAULT);
        auto err = 0;
        if((err = swr_convert_frame(m_d, to, from))>= 0
          ||((err = config(to,from))<0))
            return err;
        return swr_convert_frame(m_d,to,from);
    }
    int convert(uint8_t **out, int out_count, const uint8_t **in, int in_count)
    {
        return swr_convert(m_d, out, out_count,in, in_count);
    }
    int64_t next_pts(int64_t pts)
    {
        return swr_next_pts(m_d, pts);
    }
    int set_compensation(int sample_delta, int comp_dist)
    {
        return swr_set_compensation(m_d,sample_delta,comp_dist);
    }
    int drop_output(int count) { return swr_drop_output(m_d,count);}
    int inject_silence(int count) { return swr_inject_silence(m_d,count);}
    int out_samples(int in_samples) { return swr_get_out_samples(m_d, in_samples);}
    friend void swap(swr_context &lhs, swr_context &rhs) noexcept { lhs.swap(rhs);}
};
struct avpacket {
    AVPacket *m_d{nullptr};
    AVPacket       *operator ->()       { return m_d;}
    const AVPacket *operator ->() const { return m_d;}
            AVPacket &operator  *()     { return *m_d;}
    const AVPacket &operator *() const  { return *m_d;}
    operator const AVPacket *() const   { return m_d;}
    operator       AVPacket *()         { return m_d;}
    operator bool() const               { return !!m_d; }
    bool operator !() const             { return !m_d; }
    avpacket() : m_d{av_packet_alloc()} {}
    avpacket(avpacket && o) noexcept    { swap(o); }
    avpacket(const avpacket &o) : avpacket() { av_packet_ref(m_d,o.m_d); }
    avpacket &operator =(avpacket && o) noexcept
    {
        swap(o);
        return *this;
    }
    avpacket &operator =(const avpacket &o)
    {
        ref(o.m_d);
        return *this;
    }
   ~avpacket() { av_packet_free(&m_d); }
    void swap(avpacket &o) noexcept
    {
        using std::swap;
        swap(m_d, o.m_d);
    }
    void unref() { if(m_d) av_packet_unref(m_d); }
    void ref(const AVPacket *pkt)
    {
        if(pkt != m_d) {
            unref();
            if(pkt) {
                if(!m_d)
                    m_d = av_packet_alloc();
                av_packet_ref(m_d, pkt);
            }
        }
    }
    friend void swap(avpacket &lhs, avpacket &rhs) noexcept
    {
        lhs.swap(rhs);
    }
};
struct avframe{
    AVFrame *m_d{nullptr};

    AVFrame *operator ->()              { return m_d;}
    const AVFrame *operator ->() const  { return m_d;}
          AVFrame &operator  *()        { return *m_d;}
    const AVFrame &operator  *() const  { return *m_d;}
    operator const AVFrame *  () const  { return m_d;}
    operator       AVFrame *  ()        { return m_d;}
    operator bool() const               { return !!m_d; }
    bool operator !() const             { return !m_d; }
    avframe() : m_d{av_frame_alloc()}   {}
    avframe(avframe && o) noexcept      { swap(o); }
    avframe &operator =(avframe && o) noexcept
    {
        swap(o);
        return *this;
    }
    avframe(const avframe &o) : avframe()   { if(o.m_d) av_frame_ref(m_d,o.m_d); }
    avframe &operator=(const avframe &o)    { ref(o.m_d); return *this; }
   ~avframe()                               { av_frame_free(&m_d); }
    void swap(avframe &o) noexcept          { using std::swap; swap(m_d, o.m_d); }
    void unref()                            { if(m_d) av_frame_unref(m_d); }
    void unref_alloc()
    {
        if(m_d)
            av_frame_unref(m_d);
        else
            m_d = av_frame_alloc();
    }
    void ref(const AVFrame *frm)
    {
        if(frm != m_d) {
            if(!m_d) {
                m_d = av_frame_clone(frm);
            } else {
                av_frame_unref(m_d);
                if(frm)
                    av_frame_ref(m_d,frm);
            }
        }
    }
    int  get_buffer(int _align = 32)
    {
        if(!m_d)
            return AVERROR(EFAULT);

        auto _samples = m_d->nb_samples;
        auto _layout  = m_d->channel_layout;
        auto _channels= m_d->channels;

        auto _width   = m_d->width;
        auto _height  = m_d->height;

        auto _format  = m_d->format;

        unref();
        m_d->nb_samples     = _samples;
        m_d->channel_layout = _layout;
        m_d->channels       = _channels;
        m_d->height         = _height;
        m_d->width          = _width;
        m_d->format         = _format;
        return av_frame_get_buffer(m_d, _align);
    }
    int get_audio_buffer(AVSampleFormat _format, int _channels, int _samples, int _align = 32)
    {
        unref_alloc();
        m_d->format     = static_cast<int>(_format);
        m_d->channels   = _channels;
        m_d->nb_samples = _samples;
        return av_frame_get_buffer(m_d, _align);
    }
    int get_vidio_buffer(AVPixelFormat _format, int _width, int _height, int _align = 32)
    {
        unref_alloc();
        m_d->format = static_cast<int>(_format);
        m_d->width  = _width;
        m_d->height = _height;
        return av_frame_get_buffer(m_d, _align);
    }
    bool writable() const                   { return m_d && av_frame_is_writable(m_d); }
    bool make_writable()                    { return m_d && !av_frame_make_writable(m_d); }
    int64_t best_effort_timestamp() const   { return m_d ? av_frame_get_best_effort_timestamp(m_d) : AV_NOPTS_VALUE; }
    AVSampleFormat format() const           { return m_d ? AVSampleFormat(m_d->format) : AV_SAMPLE_FMT_NONE; }
    bool planar() const                     { return m_d && av_sample_fmt_is_planar(format()); }
    int sample_rate() const                 { return m_d ? av_frame_get_sample_rate(m_d) : 0; }
    int channels() const                    { return m_d ? av_frame_get_channels(m_d) : 0; }
    int samples() const                     { return m_d ? m_d->nb_samples : 0; }
    int64_t pts() const                     { return m_d ? m_d->pts : AV_NOPTS_VALUE; }
    int64_t pkt_dts() const                 { return m_d ? m_d->pkt_dts : AV_NOPTS_VALUE; }
    int64_t pkt_pos() const                 { return m_d ? m_d->pkt_pos : 0; }
    int64_t pkt_duration() const            { return m_d ? m_d->pkt_duration : 0; }
    int64_t channel_layout() const          { return m_d ? m_d->channel_layout : 0; }
    uint8_t * const *data() const           { return m_d ? m_d->extended_data : nullptr; }
    template<class T>
    T * const * data() const
    {
        return m_d ? static_cast<T *const *>(m_d->extended_data) : nullptr;
    }
    friend void swap(avframe &lhs, avframe &rhs) noexcept
    {
        lhs.swap(rhs);
    }
};
struct format_context {
    AVFormatContext  *m_d{avformat_alloc_context()};
    AVFormatContext *operator ->()              { return m_d;}
    const AVFormatContext *operator ->() const  { return m_d;}
            AVFormatContext &operator  *()      { return *m_d;}
    const AVFormatContext &operator  *() const  { return *m_d;}
    operator const AVFormatContext *  () const  { return m_d;}
    operator       AVFormatContext *  ()        { return m_d;}
    operator bool() const                       { return !!m_d;}
    bool operator !() const                     { return !m_d;}

    AVStream *new_stream(const AVCodec *_c)     { return avformat_new_stream(m_d, _c); }
    format_context() {}
    format_context(format_context && o) noexcept: m_d{}         { swap(o); }
    format_context &operator =(format_context && o) noexcept    { swap(o); return *this; }
   ~format_context()
    {
       if(is_input())
           avformat_close_input(&m_d);
       avformat_free_context(m_d);
    }
    void swap(format_context &o) noexcept   { using std::swap; swap(m_d, o.m_d); }
    std::pair<avpacket,int> read_frame()
    {
        auto p = std::make_pair(avpacket{},0);

        if((p.second = read_frame(p.first)) < 0) {
            p.first.unref();
        }
        return std::move(p);
    }
    int read_frame(AVPacket *pkt) { return av_read_frame(m_d,pkt); }
    void close()
    {
        if(is_input())
            avformat_close_input(&m_d);
        if(is_output())
            avformat_free_context(m_d);
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
    int alloc_output(AVOutputFormat *oformat, const char *format_name, const char *filename)
    {
        if(is_input())
            avformat_close_input(&m_d);
        avformat_free_context(m_d);
        m_d = nullptr;
        return avformat_alloc_output_context2(&m_d, oformat, format_name, filename);
    }
    std::pair<AVStream*, AVCodec*> find_best_stream(AVMediaType type)
    {
        auto ret = 0;
        auto res = std::pair<AVStream*,AVCodec*>{};
        if(m_d) {
            if (( ret = av_find_best_stream ( m_d, type, -1, -1, &res.second, 0 )) >= 0 ) {
                res.first = m_d->streams[ret];
                if(!res.second)
                    res.second = avcodec_find_decoder ( res.first->codecpar->codec_id ) ;
                res.first->discard = AVDISCARD_NONE;
            }
        }
        return res;
    }
    int seek_frame(int stream_idx, int64_t ts, int flags)
    {
        return av_seek_frame(m_d, stream_idx, ts, flags);
    }
    int seek_file(int stream_idx, int64_t min_pts, int64_t ts, int64_t max_pts, int flags = 0)
    {
        return avformat_seek_file(m_d, stream_idx, min_pts, ts, max_pts, flags);
    }
    bool is_input() const { return m_d && !!m_d->iformat;}
    bool is_output() const { return m_d && !!m_d->oformat;}
    int flush() { return m_d ? avformat_flush(m_d) : AVERROR(EINVAL);}
    void dump(int index, const char *filename) const
    {
        if(m_d)
            av_dump_format(m_d, index, filename, is_output());
    }
    friend void swap(format_context &lhs, format_context &rhs) noexcept { lhs.swap(rhs);}
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
    void extract_parameters(AVCodecParameters *par)
    {
        avcodec_parameters_from_context(par, m_d);
    }
    void apply_parameters(AVCodecParameters *par)
    {
        avcodec_parameters_to_context(m_d, par);
    }
    int open( AVCodec *codec = nullptr)
    {
        return avcodec_open2(m_d, codec, nullptr);
    }
    void close()
    {
        if ( avcodec_is_open(m_d))
            avcodec_close(m_d);
    }
    codec_context(codec_context && o) noexcept : codec_context(){ swap(o);}
    codec_context &operator =(codec_context && o) noexcept { swap(o);return *this;}
   ~codec_context()
    {
        close();
        avcodec_free_context(&m_d);
    }
    bool is_open()  const { return avcodec_is_open(m_d);}
    bool is_input() const { return av_codec_is_decoder(m_d->codec);}
    bool is_output()const { return av_codec_is_encoder(m_d->codec);}

    int send_frame(AVFrame *frm) { return avcodec_receive_frame(m_d,frm);}
    int receive_packet(AVPacket *pkt) { return avcodec_receive_packet(m_d,pkt); }
    int send_packet(AVPacket *pkt) { return avcodec_send_packet(m_d,pkt); }
    int receive_frame(AVFrame *frm) { return avcodec_receive_frame(m_d,frm);}
    void flush_buffers() { avcodec_flush_buffers(m_d);}
    void swap(codec_context &o) noexcept { using std::swap;swap(m_d, o.m_d);}
    friend void swap(codec_context &lhs, codec_context &rhs) noexcept { lhs.swap(rhs);}
};

inline AVRational operator *(const AVRational &a, const AVRational &b) { return av_mul_q(a,b); }
inline AVRational operator /(const AVRational &a, const AVRational &b) { return av_div_q(a,b); }
inline AVRational operator +(const AVRational &a, const AVRational &b) { return av_add_q(a,b); }
inline AVRational operator -(const AVRational &a, const AVRational &b){ return av_sub_q(a,b); }
inline bool operator ==(const AVRational &a, const AVRational &b) { return !av_cmp_q(a,b); }
inline bool operator !=(const AVRational &a, const AVRational &b) { return !(a==b); }
inline bool operator <(const AVRational &a, const AVRational &b) { return av_cmp_q(a,b)< 0; }
inline bool operator >(const AVRational &a, const AVRational &b) { return av_cmp_q(a,b)> 0; }
inline bool operator <=(const AVRational &a, const AVRational &b) { return av_cmp_q(a,b)<= 0; }
inline bool operator >=(const AVRational &a, const AVRational &b) { return av_cmp_q(a,b)>= 0; }
inline int64_t operator *(int64_t a, const AVRational &q) { return av_rescale(a, q.num, q.den); }
inline int64_t operator *(const AVRational &q, int64_t a) { return av_rescale(a, q.num, q.den); }
}
