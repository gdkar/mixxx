#include "soundsourcempg123.h"
#include "util/math.h"
#include <id3tag.h>

namespace Mixxx {


/* static */ ssize_t SoundSourceMpg123::io_read ( void * vptr, void *buf, size_t size){
  SoundSourceMpg123 *self = reinterpret_cast<SoundSourceMpg123*>(vptr);
  size = math_min<ssize_t>(size,self->m_fileData.size()-self->m_fileTell);
  memmove(buf, self->m_fileData.constData() + self->m_fileTell,size);
  self->m_fileTell+= size;
  return size;
}
/* static */ off_t   SoundSourceMpg123::io_lseek ( void * vptr, off_t offset, int whence ){
  SoundSourceMpg123 *self = reinterpret_cast<SoundSourceMpg123*>(vptr);
  if ( whence == SEEK_CUR ) offset += self->m_fileTell;
  else if ( whence == SEEK_END ) offset += self->m_fileData.size();
  else if ( whence != SEEK_SET ) return self->m_fileTell;
  offset = math_clamp<ssize_t>(offset,0l,(ssize_t)self->m_fileData.size());
  self->m_fileTell = offset;
  return offset;
}
/* static */ void    SoundSourceMpg123::io_cleanup ( void * vptr ){
  SoundSourceMpg123 *self = reinterpret_cast<SoundSourceMpg123*>(vptr);
  self->m_fileData.clear();
}

SoundSourceMpg123::SoundSourceMpg123(const QUrl &url)
        : SoundSourcePlugin(url, "mp3"),
          m_h(nullptr){
  mpg123_init ();
  m_h = mpg123_new ( NULL, NULL );
  mpg123_param ( m_h, MPG123_ADD_FLAGS, MPG123_FORCE_STEREO | MPG123_FORCE_FLOAT , 0.0f );
  mpg123_param ( m_h, MPG123_INDEX_SIZE, -1, 0.0f );
  mpg123_replace_reader_handle ( m_h, &SoundSourceMpg123::io_read, &SoundSourceMpg123::io_lseek, &SoundSourceMpg123::io_cleanup );
}

SoundSourceMpg123::~SoundSourceMpg123() {
    close();
    mpg123_delete ( m_h );
    mpg123_exit ();
}

Result SoundSourceMpg123::tryOpen(const AudioSourceConfig& /*audioSrcCfg*/) {
    QFile l_file(getLocalFileName());
    if (!l_file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file:" << l_file.fileName();
        return ERR;
    }
    // Get a pointer to the file data
    m_fileData = l_file.readAll();
    qDebug() << "Read " << m_fileData.size() << " bytes from " << getLocalFileName();
    l_file.close();
    mpg123_open_handle ( m_h, reinterpret_cast<void*>(this));
    mpg123_getformat ( m_h, &m_rate, &m_nch, &m_enc );
    mpg123_format_none ( m_h );
    m_nch = 2;
    m_enc = MPG123_ENC_FLOAT_32;
    mpg123_format ( m_h, m_rate, m_nch, m_enc );
    // Initialize the AudioSource
    setChannelCount(m_nch);
    setFrameRate (m_rate);
    mpg123_scan ( m_h );
    setFrameCount(mpg123_length ( m_h ) );
    return OK;
}

void SoundSourceMpg123::close() {
    mpg123_close ( m_h );
    m_fileData.clear();
}

SINT SoundSourceMpg123::seekSampleFrame(SINT frameIndex) {
    return mpg123_seek ( m_h, frameIndex, SEEK_SET );
}

SINT SoundSourceMpg123::readSampleFrames(
        SINT numberOfFrames, CSAMPLE* sampleBuffer){
    const SINT numberOfFramesTotal = numberOfFrames;
    uint8_t* pSampleBuffer = reinterpret_cast<uint8_t*>(sampleBuffer);
    SINT numberOfFramesRemaining = numberOfFramesTotal;
    SINT numberOfBytesRemaining = numberOfFramesTotal *getChannelCount()*sizeof(float);
    size_t done;
    int ret = 0;
    do{
      ret = mpg123_read ( m_h, pSampleBuffer, numberOfBytesRemaining, &done );
      if(ret==MPG123_OK){
        numberOfBytesRemaining -= done;
        pSampleBuffer          += done;
        numberOfFramesRemaining-= done / ( getChannelCount()*sizeof(float));
      }
    }while(ret == MPG123_OK && numberOfBytesRemaining > 0 );
    return numberOfFramesTotal - numberOfFramesRemaining;
}

QString SoundSourceProviderMpg123::getName() const {
    return "mpg123: MPEG Audio Layer 1/2/3 Decoder";
}

QStringList SoundSourceProviderMpg123::getSupportedFileExtensions() const {
    QStringList supportedFileExtensions;
    supportedFileExtensions.append("mp3");
    return supportedFileExtensions;
}

} // namespace Mixxx

extern "C" MIXXX_SOUNDSOURCEPLUGINAPI_EXPORT
Mixxx::SoundSourceProviderPointer Mixxx_SoundSourcePluginAPI_getSoundSourceProvider(){
  static Mixxx::SoundSourceProviderMpg123 singleton;
  return Mixxx::SoundSourceProviderPointer (
      &singleton
    , &Mixxx::SoundSourceProviderMpg123::deleter
    );
}
