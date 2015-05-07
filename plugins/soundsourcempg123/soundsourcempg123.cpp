#include "soundsourcempg123.h"

#include "sampleutil.h"

#ifdef __WINDOWS__
#include <io.h>
#include <fcntl.h>
#endif


namespace Mixxx {


QList<QString> SoundSourceMPG123::supportedFileExtensions() {
    QList<QString> list;
    list.push_back("mp3");
    list.push_back("mp2");
    list.push_back("mpeg");
    return list;
}

SoundSourceMPG123::SoundSourceMPG123(QUrl url)
        : SoundSourcePlugin(url, "mpg123"),
          m_h(0),
          m_curFrameIndex(0) {
  mpg123_init();
  m_h = mpg123_new(NULL,NULL);

}

SoundSourceMPG123::~SoundSourceMPG123() {
  mpg123_close(m_h);
  mpg123_delete(m_h);
  mpg123_exit();
}

Result SoundSourceMPG123::tryOpen(const AudioSourceConfig &audioSrcCfg) {
  int ret;
    if(mpg123_param(m_h,MPG123_ADD_FLAGS,MPG123_FORCE_STEREO|MPG123_FORCE_FLOAT|MPG123_GAPLESS|MPG123_SKIP_ID3V2,0.0f)!=MPG123_OK||
      mpg123_param(m_h,MPG123_VERBOSE,3,0.0f)!=MPG123_OK||
      mpg123_param(m_h,MPG123_INDEX_SIZE,-1,0.0f))
      return ERR;
    if((ret=mpg123_open(m_h,getLocalFileNameBytes().constData()))<0)
      return ERR;
    int nch, enc;
    long rate;
    if( mpg123_getformat(m_h,&rate,&nch,&enc)!=MPG123_OK||
        mpg123_format_none(m_h)!=MPG123_OK)return ERR;
    setFrameRate(rate);
    setChannelCount(nch);
    if(enc!=MPG123_ENC_FLOAT_32)
      return ERR;
    if( mpg123_format(m_h,rate,nch,enc)!=MPG123_OK||
        mpg123_scan(m_h)!=MPG123_OK)
      return ERR;
    setFrameCount(mpg123_length(m_h));
    return OK;
}

void SoundSourceMPG123::close() {
}

SINT SoundSourceMPG123::seekSampleFrame(SINT frameIndex) {
    mpg123_seek(m_h,frameIndex,SEEK_SET);
    return (m_curFrameIndex=mpg123_tell(m_h));
}

SINT SoundSourceMPG123::readSampleFrames(
        SINT numberOfFrames, CSAMPLE* sampleBuffer) {
    size_t done=0;
    int ret;
    size_t outmemsize = getSampleBufferSize(numberOfFrames,false)*sizeof(float);
    unsigned char *outmem = (unsigned char*)sampleBuffer;
    do{
      ret = mpg123_read(m_h,outmem,outmemsize,&done);
      if(ret==MPG123_OK){
        outmem     += done;
        outmemsize -= done;
      }
    }while(outmemsize && done && ret==MPG123_OK);
    return numberOfFrames-samples2frames(outmemsize/sizeof(float));
}

} // namespace Mixxx

extern "C" MY_EXPORT const char* getMixxxVersion() {
    return VERSION;
}

extern "C" MY_EXPORT int getSoundSourceAPIVersion() {
    return MIXXX_SOUNDSOURCE_API_VERSION;
}

extern "C" MY_EXPORT Mixxx::SoundSource* getSoundSource(QString fileName) {
    return new Mixxx::SoundSourceMPG123(fileName);
}

extern "C" MY_EXPORT char** supportedFileExtensions() {
    const QList<QString> supportedFileExtensions(
            Mixxx::SoundSourceMPG123::supportedFileExtensions());
    return Mixxx::SoundSourcePlugin::allocFileExtensions(
            supportedFileExtensions);
}

extern "C" MY_EXPORT void freeFileExtensions(char** fileExtensions) {
    Mixxx::SoundSourcePlugin::freeFileExtensions(fileExtensions);
}
