/***************************************************************************
                     encodervorbis.h  -  vorbis encoder for mixxx
                             -------------------
    copyright            : (C) 2012-2013 by Tuukka Pasanen
                           (C) 2007 by Wesley Stessens
                           (C) 1994 by Xiph.org (encoder example)
                           (C) 1994 Tobias Rafreider (broadcast and recording fixes)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ENCODERFFMPEGCORE_H
#define ENCODERFFMPEGCORE_H

#include <encoder/encoderffmpegresample.h>
#include "util/ffmpeg-utils.hpp"



// Compability
#include <QByteArray>
#include <QBuffer>

#include <QLibrary>

#include "util/types.h"
#include "encoder/encoder.h"
#include "track/track.h"

class EncoderCallback;

class EncoderFfmpegCore : public Encoder {
public:
    EncoderFfmpegCore(EncoderCallback* pCallback=nullptr, AVCodecID codec = AV_CODEC_ID_MP3, const char *example_filename = "output.mp3");
   ~EncoderFfmpegCore();
    int initEncoder(int bitrate, int samplerate) override;
    void encodeBuffer(const CSAMPLE *samples, const int size) override;
    void updateMetaData(char* artist, char* title, char* album) override;
    void flush() override;
protected:
    unsigned int reSample(AVFrame *inframe);
private:
    int  getSerial();
    bool metaDataHasChanged();
    //Call this method in conjunction with broadcast streaming
    int  writeAudioFrame(const CSAMPLE *samples, int size);

    EncoderCallback* m_callback{};
    TrackPointer     m_pMetaData;

    char  *m_strMetaDataTitle{};
    char  *m_strMetaDataArtist{};
    char  *m_strMetaDataAlbum{};
    QFile  m_file;

    AVCodecID      m_codec_id{};
    AVCodec       *m_codec{};;
    codec_context  m_cocec_ctx{};
    AVOutputFormat*m_output_fmt{};;

    avframe        m_frame_orig{};
    avframe        m_frame_swr{};
    swr_context    m_swr{};

    uint64_t       m_next_dts;
    uint64_t       m_next_pts;
    avpacket       m_pkt{};

    AVStream      *m_stream{};
    bool           m_stream_initialized{false};

    format_context m_format_ctx{};

    int m_frame_size{};

    uint32_t m_bitrate{};
    uint32_t m_sampleRate{};
    uint64_t m_recordedBytes{};
};

#endif
