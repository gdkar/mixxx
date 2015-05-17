#include "soundsourcempg123.h"

#include "sampleutil.h"

#ifdef __WINDOWS__
#include <io.h>
#include <fcntl.h>
#endif




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
Result SoundSourceMPG123::tryOpen(const AudioSourceConfig &config) {
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
void SoundSourceMPG123::close() {}

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

bool SoundSourceMPG123::configureAudioStream(const Mixxx::AudioSourceConfig& audioSrcCfg){
  HRESULT hr(S_OK);
}
QString SoundSourceProviderMPG123::getName() const{return "libmpg123";}

QStringList SoundSourceProviderMPG123::getSupportedFileTypes()const{
    QStringList strl;
    strl<< "mp3" << "mp2" << "audio/mpeg";
    return strl;
}
extern "C" MIXXX_SOUNDSOURCEPLUGINAPI_EXPORT
Mixxx::SoundSourceProviderPointer Mixxx_SoundSourcdPluginAPI_getSoundSourceProvider(){
  return Mixxx::SoundSourceProviderPointer(new Mixxx::SoundSourceProviderMPG123);
}
