
#include "sources/soundsourceffmpeg.h"
#include <libswresample/swresample.h>

#define AUDIOSOURCEFFMPEG_CACHESIZE 1000
#define AUDIOSOURCEFFMPEG_MIXXXFRAME_TO_BYTEOFFSET (sizeof(CSAMPLE) * getChannelCount())
#define AUDIOSOURCEFFMPEG_FILL_FROM_CURRENTPOS -1

namespace Mixxx {

QStringList SoundSourceProviderFFmpeg::getSupportedFileExtensions() const {
    QStringList list;
    AVInputFormat *l_SInputFmt  = NULL;

    while ((l_SInputFmt = av_iformat_next(l_SInputFmt))) {
        if (l_SInputFmt->name == NULL) {
            break;
        }

        if (!strcmp(l_SInputFmt->name, "flac")) {
            list.append("flac");
        } else if (!strcmp(l_SInputFmt->name, "ogg")) {
            list.append("ogg");
        } else if (!strcmp(l_SInputFmt->name, "mov,mp4,m4a,3gp,3g2,mj2")) {
            list.append("m4a");
            list.append("mp4");
        } else if (!strcmp(l_SInputFmt->name, "mp4")) {
            list.append("mp4");
        } else if (!strcmp(l_SInputFmt->name, "mp3")) {
            list.append("mp3");
        } else if (!strcmp(l_SInputFmt->name, "aac")) {
            list.append("aac");
        } else if (!strcmp(l_SInputFmt->name, "opus") ||
                   !strcmp(l_SInputFmt->name, "libopus")) {
            list.append("opus");
        } else if (!strcmp(l_SInputFmt->name, "wma") or
                   !strcmp(l_SInputFmt->name, "xwma")) {
            list.append("wma");
        }
    }

    return list;
}

SoundSourceFFmpeg::SoundSourceFFmpeg(QUrl url)
    : SoundSource(url),
      m_fmt_ctx(avformat_alloc_context())
{}

SoundSourceFFmpeg::~SoundSourceFFmpeg() {
    close();
}

Result SoundSourceFFmpeg::tryOpen(const AudioSourceConfig& /*audioSrcCfg*/) {
    auto l_format_opts = (AVDictionary *)nullptr;
    const auto qBAFilename = getLocalFileNameBytes();
    qDebug() << "New SoundSourceFFmpeg :" << qBAFilename;

    // Open file and make m_pFormatCtx
    if (avformat_open_input(&m_fmt_ctx, qBAFilename.constData(), nullptr,&l_fmt_opts)) {
        qDebug() << "SoundSourceFFmpeg::tryOpen: cannot open" << qBAFilename;
        return ERR;
    }
    av_dict_free(&l_iFormatOpts);
    // Retrieve stream information
    if (avformat_find_stream_info(m_fmt_ctx, nullptr) < 0) {
        qDebug() << "SoundSourceFFmpeg::tryOpen: cannot open" << qBAFilename;
        return ERR;
    }
    //debug only (Enable if needed)
    //av_dump_format(m_pFormatCtx, 0, qBAFilename.constData(), false);
    
    if((m_stream_idx=av_find_best_stream(m_fmt_ctx,AVMEDIA_TYPE_AUDIO,-1,-1,&m_codec,0))<0){
      qDebug() << "SoundSourceFFmpeg::tryOpen: cannot find audio stream in" << qBAFilename;
      return ERR;
    }
    m_stream    = m_fmt_ctx->streams[m_stream_idx];
    m_codec_ctx = m_stream->codec;
    av_opt_set_int(m_codec_ctx,"refcounted_frames",1,0);
    if(avcodec_open2(m_codec_ctx, m_codec,nullptr)<0){
      qDebug() << "SoundSourceFFmpeg::tryOpen: cannot open codec for" << qBAFilename;
    }
    setChannelCount(m_codec_ctx->channels);
    setFrameRate(m_codec_ctx->sample_rate);
    setFrameCount(qint64
    m_pResample = new EncoderFfmpegResample(m_pCodecCtx);
    m_pResample->openMixxx(m_pCodecCtx->sample_fmt, AV_SAMPLE_FMT_FLT);

    setChannelCount(m_pCodecCtx->channels);
    setFrameRate(m_pCodecCtx->sample_rate);
    setFrameCount(static_cast<qint64>(static_cast<double>(m_format_ctx->duration)
                                    * getFrameRate / AV_TIME_BASE));
    qDebug() << "SoundSourceFFmpeg::tryOpen: Samplerate: " << getFrameRate() <<
             ", Channels: " << getChannelCount() << "\n";

    m_swr = swr_alloc_set_opts(m_swr, 
                               AV_CH_LAYOUT_STEREO,
                               AV_SAMPLE_FMT_FLT,
                               getFrameRate(),
                               m_codec_ctx->channel_layout,
                               m_codec_ctx->format,
                               m_codec_ctx->sample_rate,
                               0,
                               nullptr
      );
    if(swr_init(m_swr)<0){
      qDebug() << "SoundSourceFFmpeg::tryOpen: failed to initialize resampler for " << qBAFilename;
      return ERR;
    }
    auto pkt = AVPacket{0};
    auto ret = 0;
    do{
      ret = av_read_frame(m_fmt_ctx,&pkt);
      if(ret>=0 && pkt.stream == m_stream_idx){
        av_dup_packet(&pkt);
        m_packets.push_back(pkt);
      }else{av_free_packet(&pkt);}
    }while(ret>=0);
    avformat_close_input(&m_fmt_ctx);
    m_fmt_ctx=nullptr;
    return OK;
}

void SoundSourceFFmpeg::close() {
    avcodec_close(m_codec_ctx);
    avformat_close_input(&m_fmt_ctx);
    swr_free(&m_swr);
    for(auto &pkt : m_packets)
      av_free_packet(&pkt);
    m_packets.clear();
    av_frame_free(&m_cur_frame);
}

SINT SoundSourceFFmpeg::seekSampleFrame(SINT frameIndex) {
    DEBUG_ASSERT(isValidFrameIndex(frameIndex));

    int ret = 0;
    qint64 i = 0;
    auto framePts  = qint64(AV_TIME_BASE * frameIndex / getFrameRate());
    auto curPktPts = m_cur_pkt.pts;
    if (!(framePts >= curPktPts && framePts < curPktPts + m_cur_pkt.duration)){
      auto lowPts = framePts < curPktPts ? m_packets.front().pts : curPktPts;
      auto hiPts  = framePts > curPktPts ? m_packets.back().pts  : curPktPts;
      auto lowIdx = framePts < curPktPt  ? SINT{0}               : m_cur_pkt_idx;
      auto hiIdx  = framePts > curPktPt  ? m_packets.size()      : m_cur_pkt_idx;
      while((framePts < curPktPts || framePts >= curPktPts + m_cur_pkt.duration)&&hiIdx>lowIdx+1){
        auto fraction = (double) (framePts - lowPts) / (hiPts-lowPts);
        auto fracIdx  = (SINT) (lowIdx + (hiIdx-lowIdx)*fraction);
        if ( fracIdx <= lowIdx || fracIdx >= hiIdx)
          fracIdx = (lowIdx + hiIdx)/2;
        auto fracPts  = m_packets[fracIdx].pts;
        if ( fracPts > framePts ){
          lowPts        = fracPts;
          lowIdx        = fracIdx;
        }else{
          hiPts         = fracPts;
          hiIdx         = fracIdx;
        }
        m_cur_pkt_idx = fracIdx;
        m_cur_pkt     = m_packets[m_cur_pkt_idx];
        curPktPts     = m_cur_pkt.pts;
      }
      if(framePts < curPktPts){
        framePts = curPktPts;
      }else if(framePts >= curPktPts + m_cur_pkt.duration && m_cur_pkt_idx < m_packets.size()-1){
        m_cur_pkt_idx++;
        m_cur_pkt = m_packets[m_cur_pkt_idx];
        framePts  = m_cur_pkt.pts;
      }
    }
    m_cur_pts = framePts;
    return framePts * getFrameRate() / AV_TIME_BASE;
}

SINT SoundSourceFFmpeg::readSampleFrames(SINT numberOfFrames,CSAMPLE* sampleBuffer) {
    auto numberLeft = numberOfFrames;
    do{
      if(m_cur_frame){
        auto offset = static_cast<SINT>((m_cur_pts - m_cur_frame->pts)*1e-6*static_cast<double>(getFrameRate()));
        auto avail  = m_cur_frame->nb_samples - offset;
        auto chunk  = math_min(avail,numberLeft);
        std::memmove(sampleBuffer,m_cur_frame->extended_data[0] + sizeof(CSAMPLE)*getChannelCount()*offset,
            sizeof(CSAMPLE)*getChannelCount()*chunk);
        sampleBuffer += getChannelCount()*chunk;
        numberLeft   -= chunk;
        m_cur_pts += static_cast<SINT>(chunk * 1e6 / static_cast<double>(getFrameRate()));
        if(chunk >= avail){av_frame_free(&m_cur_frame);}
        continue;
      }else{
        if(m_cur_pkt.size <= 0){
          m_cur_pkt_idx++;
          if(m_cur_pkt_idx >= m_packets.size()) break;
          m_cur_pkt = m_packets[m_cur_pkt_idx];
        }
        auto got_frame = 0;
      }
    }while(numberLeft);
    return numberOfFrames - numberLeft;
}

} // namespace Mixxx
