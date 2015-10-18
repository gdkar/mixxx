#include "library/scanner/scannerglobal.h"

ScannerGlobal::ScannerGlobal(QSet<QString> trackLocations,
                  QHash<QString, int> directoryHashes,
                  QRegExp supportedExtensionsMatcher,
                  QRegExp supportedCoverExtensionsMatcher,
                  QStringList directoriesBlacklist)
        : m_trackLocations(trackLocations),
          m_directoryHashes(directoryHashes),
          m_supportedExtensionsMatcher(supportedExtensionsMatcher),
          m_supportedCoverExtensionsMatcher(supportedCoverExtensionsMatcher),
          m_directoriesBlacklist(directoriesBlacklist),
          // Unless marked un-clean, we assume it will finish cleanly.
          m_scanFinishedCleanly(true),
          m_shouldCancel(false),
          m_numAddedTracks(0),
          m_numScannedDirectories(0)
{
}
TaskWatcher& ScannerGlobal::getTaskWatcher()
{ 
  return m_watcher;
}
bool ScannerGlobal::trackExistsInDatabase(QString trackLocation) const
{
    return m_trackLocations.contains(trackLocation);
}
int ScannerGlobal::directoryHashInDatabase(QString directoryPath) const
{
    return m_directoryHashes.value(directoryPath, -1);
}
bool ScannerGlobal::directoryBlacklisted(QString directoryPath) const
{
    return m_directoriesBlacklist.contains(directoryPath);
}
QRegExp ScannerGlobal::supportedExtensionsRegex() const
{
    return m_supportedExtensionsMatcher;
}
// TODO(rryan) test whether tasks should create their own QRegExp.
bool ScannerGlobal::isAudioFileSupported(QString fileName) const
{
    QMutexLocker locker(&m_supportedExtensionsMatcherMutex);
    return m_supportedExtensionsMatcher.indexIn(fileName) != -1;
}
QRegExp ScannerGlobal::supportedCoverExtensionsRegex() const
{
    return m_supportedCoverExtensionsMatcher;
}
// TODO(rryan) test whether tasks should create their own QRegExp.
bool ScannerGlobal::isCoverFileSupported(QString fileName) const
{
    QMutexLocker locker(&m_supportedCoverExtensionsMatcherMutex);
    return m_supportedCoverExtensionsMatcher.indexIn(fileName) != -1;
}
bool ScannerGlobal::shouldCancel() const
{ 
    return m_shouldCancel;
}
std::atomic<bool>* ScannerGlobal::shouldCancelPointer()
{ 
    return &m_shouldCancel;
}
void ScannerGlobal::cancel()
{ 
  m_shouldCancel.store(true);
}
bool ScannerGlobal::scanFinishedCleanly() const
{ 
  return m_scanFinishedCleanly;
}
void ScannerGlobal::clearScanFinishedCleanly()
{ 
  m_scanFinishedCleanly = false;
}
void ScannerGlobal::addVerifiedDirectory(QString directory)
{ 
  m_verifiedDirectories << directory;
}
QStringList ScannerGlobal::verifiedDirectories() const
{ 
  return m_verifiedDirectories;
}
void ScannerGlobal::addVerifiedTrack(QString trackLocation)
{ 
  m_verifiedTracks << trackLocation;
}
QStringList ScannerGlobal::verifiedTracks() const
{ 
  return m_verifiedTracks;
}
void ScannerGlobal::startTimer()
{ 
  m_timer.start();
}
// Elapsed time in nanoseconds since startTimer was called.
qint64 ScannerGlobal::timerElapsed()
{ 
  return m_timer.elapsed();
}
int ScannerGlobal::numAddedTracks() const
{ 
  return m_numAddedTracks;
}
void ScannerGlobal::trackAdded()
{
  m_numAddedTracks++;
}
int ScannerGlobal::numScannedDirectories() const
{
  return m_numScannedDirectories;
}
void ScannerGlobal::directoryScanned()
{ 
  m_numScannedDirectories++;
}

