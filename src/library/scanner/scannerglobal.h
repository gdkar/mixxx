_Pragma("once")
#include <QSet>
#include <QHash>
#include <QRegExp>
#include <QStringList>
#include <QMutex>
#include <QMutexLocker>
#include <QSharedPointer>
#include <atomic>

#include "util/task.h"
#include "util/performancetimer.h"

class ScannerGlobal {
  public:
    ScannerGlobal(QSet<QString> trackLocations,
                  QHash<QString, int> directoryHashes,
                  QRegExp supportedExtensionsMatcher,
                  QRegExp supportedCoverExtensionsMatcher,
                  QStringList directoriesBlacklist);
    TaskWatcher& getTaskWatcher();
    // Returns whether the track already exists in the database.
    bool trackExistsInDatabase(QString trackLocation) const;
    // Returns the directory hash if it exists or -1 if it doesn't.
    int directoryHashInDatabase(QString directoryPath) const;
    bool directoryBlacklisted(QString directoryPath) const;
    QRegExp supportedExtensionsRegex() const;
    // TODO(rryan) test whether tasks should create their own QRegExp.
    bool isAudioFileSupported(QString fileName) const;
    QRegExp supportedCoverExtensionsRegex() const;
    // TODO(rryan) test whether tasks should create their own QRegExp.
    bool isCoverFileSupported(QString fileName) const;
    bool shouldCancel() const;
    std::atomic<bool>* shouldCancelPointer();
    void cancel();
    bool scanFinishedCleanly() const;
    void clearScanFinishedCleanly();
    void addVerifiedDirectory(QString directory);
    QStringList verifiedDirectories() const;
    void addVerifiedTrack(QString trackLocation);
    QStringList verifiedTracks() const;
    void startTimer();
    // Elapsed time in nanoseconds since startTimer was called.
    qint64 timerElapsed();
    int numAddedTracks() const;
    void trackAdded();
    int numScannedDirectories() const;
    void directoryScanned();
  private:
    TaskWatcher m_watcher;
    QSet<QString> m_trackLocations;
    QHash<QString, int> m_directoryHashes;
    mutable QMutex m_supportedExtensionsMatcherMutex;
    QRegExp m_supportedExtensionsMatcher;
    mutable QMutex m_supportedCoverExtensionsMatcherMutex;
    QRegExp m_supportedCoverExtensionsMatcher;
    // Typically there are 1 to 2 entries in the blacklist so a O(n) search in a
    // QList may have better constant factors than a O(1) QSet check. However,
    // this has never been investigated.
    QStringList m_directoriesBlacklist;
    // The list of directories verified by the scan.
    QStringList m_verifiedDirectories;
    // The list of tracks verified by the scan.
    QStringList m_verifiedTracks;
    std::atomic<bool> m_scanFinishedCleanly{true};
    std::atomic< bool> m_shouldCancel{false};
    // Stats tracking.
    PerformanceTimer m_timer;
    int m_numAddedTracks;
    int m_numScannedDirectories;
};
typedef QSharedPointer<ScannerGlobal> ScannerGlobalPointer;
