#include "soundsourcemp3.h"
#include "util/math.h"
#include <id3tag.h>

namespace Mixxx {


/* static */ ssize_t SoundSourceMp3::io_read ( void * vptr, void *buf, size_t size){
  SoundSourceMp3 *self = reinterpret_cast<SoundSourceMp3*>(vptr);
  size = math_min<ssize_t>(size,self->m_fileData.size()-self->m_fileTell);
  memmove(buf, self->m_fileData.constData() + self->m_fileTell,size);
  self->m_fileTell+= size;
  return size;
}
/* static */ off_t   SoundSourceMp3::io_lseek ( void * vptr, off_t offset, int whence ){
  SoundSourceMp3 *self = reinterpret_cast<SoundSourceMp3*>(vptr);
  if ( whence == SEEK_CUR ) offset += self->m_fileTell;
  else if ( whence == SEEK_END ) offset += self->m_fileData.size();
  else if ( whence != SEEK_SET ) return self->m_fileTell;
  offset = math_clamp<ssize_t>(offset,0l,(ssize_t)self->m_fileData.size());
  self->m_fileTell = offset;
  return offset;
}
/* static */ void    SoundSourceMp3::io_cleanup ( void * vptr ){
  SoundSourceMp3 *self = reinterpret_cast<SoundSourceMp3*>(vptr);
  self->m_fileData.clear();
}

SoundSourceMp3::SoundSourceMp3(const QUrl &url)
        : SoundSource(url, "mp3"),
          m_h(nullptr){
  mpg123_init ();
  m_h = mpg123_new ( NULL, NULL );
  mpg123_param ( m_h, MPG123_ADD_FLAGS, MPG123_FORCE_STEREO | MPG123_FORCE_FLOAT , 0.0f );
  mpg123_param ( m_h, MPG123_INDEX_SIZE, -1, 0.0f );
  mpg123_replace_reader_handle ( m_h, &SoundSourceMp3::io_read, &SoundSourceMp3::io_lseek, &SoundSourceMp3::io_cleanup );
}

SoundSourceMp3::~SoundSourceMp3() {
    close();
    mpg123_delete ( m_h );
    mpg123_exit ();
}

Result SoundSourceMp3::tryOpen(const AudioSourceConfig& /*audioSrcCfg*/) {
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

void SoundSourceMp3::close() {
    mpg123_close ( m_h );
    m_fileData.clear();
}

SINT SoundSourceMp3::seekSampleFrame(SINT frameIndex) {
    return mpg123_seek ( m_h, frameIndex, SEEK_SET );
}

SINT SoundSourceMp3::readSampleFrames(
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
SINT SoundSourceMp3::readSampleFrames(
        SINT numberOfFrames, CSAMPLE* sampleBuffer,SINT bufferSize, bool stereo){
    return readSampleFrames(numberOfFrames,sampleBuffer);    
}
SINT SoundSourceMp3::readSampleFramesStereo(SINT numberOfFrames,CSAMPLE*sampleBuffer,SINT sampleBufferSize){
  return readSampleFrames(numberOfFrames,sampleBuffer,sampleBufferSize,true);
  
}
QString SoundSourceProviderMp3::getName() const {
    return "mpg123: MPEG Audio Layer 1/2/3 Decoder";
}

QStringList SoundSourceProviderMp3::getSupportedFileExtensions() const {
    QStringList supportedFileExtensions;
    supportedFileExtensions.append("mp3");
    return supportedFileExtensions;
}
} // namespace Mixxx
