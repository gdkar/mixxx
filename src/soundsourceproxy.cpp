#include "soundsourceproxy.h"
#include "sources/soundsourceffmpeg.h"
#include "util/cmdlineargs.h"
#include "util/sandbox.h"
#include "util/regex.h"
#include <QApplication>
#include <QDesktopServices>
#include <QStringList>
namespace /*anonymous*/
{
  SecurityTokenPointer openSecurityToken(QString filename, SecurityTokenPointer pToken)
  {
    if ( pToken.isNull()) 
    {
      auto info = QFileInfo(filename);
      return Sandbox::openSecurityToken(info,true);
    }
    else return pToken;
  }
}
//Static memory allocation
Mixxx::SoundSourceProviderPointer SoundSourceProxy::s_soundSourceProvider = Mixxx::SoundSourceProviderPointer(new Mixxx::SoundSourceProviderFFmpeg{});
//Constructor
SoundSourceProxy::SoundSourceProxy(QString qFilename,
        SecurityTokenPointer pToken)
        : m_pSecurityToken(openSecurityToken(qFilename, pToken))
        , m_pSoundSource(initialize(qFilename))
{
}
//Other constructor
SoundSourceProxy::SoundSourceProxy(TrackPointer pTrack)
        : m_pTrack(pTrack)
        , m_pSecurityToken(
            openSecurityToken(pTrack->getLocation(),
            pTrack->getSecurityToken()))
        , m_pSoundSource(initialize(pTrack->getLocation()))
{
                          
}
Mixxx::SoundSourcePointer SoundSourceProxy::openSoundSource(Mixxx::SoundSourceConfig audioSrcCfg)
{
    if (!m_pSoundSource)
    {
        qDebug() << "No SoundSource available";
        return m_pSoundSource;
    }
    if (!m_pSoundSource->open(audioSrcCfg)) {
        qWarning() << "Failed to open SoundSource";
        return m_pSoundSource;
    }
    if (!m_pSoundSource->isValid())
    {
        qWarning() << "Invalid file:" << m_pSoundSource->getUrlString()
                << "channels" << m_pSoundSource->getChannelCount()
                << "frame rate" << m_pSoundSource->getChannelCount();
        return m_pSoundSource;
    }
    if (m_pSoundSource->isEmpty())
    {
        qWarning() << "Empty file:" << m_pSoundSource->getUrlString();
        return m_pSoundSource;
    }
    // Overwrite metadata with actual audio properties
    if (m_pTrack)
    {
        m_pTrack->setChannels(m_pSoundSource->getChannelCount());
        m_pTrack->setSampleRate(m_pSoundSource->getFrameRate());
        if (m_pSoundSource->hasDuration())  m_pTrack->setDuration(m_pSoundSource->getDuration());
        if (m_pSoundSource->hasBitrate())   m_pTrack->setBitrate(m_pSoundSource->getBitrate());
    }
    m_pSoundSource = m_pSoundSource;
    return m_pSoundSource;
}
void SoundSourceProxy::closeSoundSource()
{
    if (m_pSoundSource)
    {
        DEBUG_ASSERT(m_pSoundSource);
        m_pSoundSource->close();
        m_pSoundSource.clear();
    }
}
// static
QStringList SoundSourceProxy::getSupportedFileExtensions()
{ 
  return s_soundSourceProvider->getSupportedFileExtensions();
}
// static
QStringList SoundSourceProxy::getSupportedFileNamePatterns()
{
  auto exts = getSupportedFileExtensions();
  auto pats = QStringList{};
  for ( auto &ext : exts ) pats += QString("*.%1").arg(ext);
  return pats;
}
//static
QRegExp SoundSourceProxy::getSupportedFileNameRegex()
{
  return QRegExp(RegexUtils::fileExtensionsRegex(getSupportedFileNamePatterns()),Qt::CaseInsensitive);
}
// static
bool SoundSourceProxy::isUrlSupported(QUrl url)
{
    auto fileInfo = QFileInfo(url.toLocalFile());
    return isFileSupported(fileInfo);
}
// static
bool SoundSourceProxy::isFileSupported(QFileInfo fileInfo)
{ 
  return isFileNameSupported(fileInfo.fileName()); 
}
// static
bool SoundSourceProxy::isFileNameSupported(QString fileName)
{ 
  return fileName.contains(getSupportedFileNameRegex());
}
// static
bool SoundSourceProxy::isFileExtensionSupported(QString fileExtension) 
{ 
  return getSupportedFileExtensions().contains(fileExtension);
}
// static
Mixxx::SoundSourcePointer SoundSourceProxy::initialize( QString qFilename )
{
    auto url = QUrl::fromLocalFile(qFilename);
    if(auto ptr = s_soundSourceProvider->newSoundSource(url)) return ptr;
    qWarning() << "Unsupported file type" << qFilename;
    return Mixxx::SoundSourcePointer();
}
QImage SoundSourceProxy::parseCoverArt() const
{
    QImage coverArt;
    auto result = parseTrackMetadataAndCoverArt(NULL, &coverArt);
    return (result ) ? coverArt : QImage();
}
bool SoundSourceProxy::parseTrackMetadata(Mixxx::TrackMetadata* pTrackMetadata)
{ 
  return parseTrackMetadataAndCoverArt(pTrackMetadata, NULL) ;
}
QString SoundSourceProxy::getType() const
{
    if (m_pSoundSource) return m_pSoundSource->getType();
    else return QString();
}
bool SoundSourceProxy::parseTrackMetadataAndCoverArt( Mixxx::TrackMetadata* pTrackMetadata, QImage* pCoverArt) const
{
    if (m_pSoundSource) return m_pSoundSource->parseTrackMetadataAndCoverArt( pTrackMetadata, pCoverArt);
    else                return false;
}


