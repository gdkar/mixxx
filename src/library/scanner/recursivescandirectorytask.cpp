#include <QDirIterator>

#include "library/scanner/recursivescandirectorytask.h"

#include "library/scanner/libraryscanner.h"
#include "library/scanner/importfilestask.h"
#include "util/timer.h"

RecursiveScanDirectoryTask::RecursiveScanDirectoryTask(
        LibraryScanner* pScanner, const ScannerGlobalPointer scannerGlobal,
        const QDir &dir, SecurityTokenPointer pToken, bool scanUnhashed)
        : ScannerTask(pScanner, scannerGlobal),
          m_dir(dir),
          m_pToken(pToken),
          m_scanUnhashed(scanUnhashed)
{
}
void RecursiveScanDirectoryTask::run()
{
    ScopedTimer timer("RecursiveScanDirectoryTask::run");
if (m_scannerGlobal->shouldCancel()) {
        setSuccess(false);
        return;
    }
    // For making the scanner slow
    //qDebug() << "Burn CPU";
    //for (int i = 0;i < 1000000000; i++) asm("nop");

    // Note, we save on filesystem operations (and random work) by initializing
    // a QDirIterator with a QDir instead of a QString -- but it inherits its
    // Filter from the QDir so we have to set it first. If the QDir has not done
    // any FS operations yet then this should be lightweight.
    m_dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    QDirIterator it(m_dir);

    QString currentFile;
    QFileInfo currentFileInfo;
    QLinkedList<QFileInfo> filesToImport;
    QLinkedList<QFileInfo> possibleCovers;
    QStringList newHashStr;

    // TODO(rryan) benchmark QRegExp copy versus QMutex/QRegExp in ScannerGlobal
    // versus slicing the extension off and checking for set/list containment.
    auto supportedExtensionsRegex      = m_scannerGlobal->supportedExtensionsRegex();
    auto supportedCoverExtensionsRegex = m_scannerGlobal->supportedCoverExtensionsRegex();

    while (it.hasNext()) {
        currentFile     = it.next();
        currentFileInfo = it.fileInfo();

        if (currentFileInfo.isFile()) {
            auto  fileName = currentFileInfo.fileName();
            if (supportedExtensionsRegex.indexIn(fileName) != -1) {
                newHashStr.append(currentFile);
                filesToImport.append(currentFileInfo);
            } else if (supportedCoverExtensionsRegex.indexIn(fileName) != -1) {
                possibleCovers.append(currentFileInfo);
            }
        } else {
            // File is a directory
            if (!m_scannerGlobal->directoryBlacklisted(currentFile)) {
                // Skip blacklisted directories like the iTunes Album
                // Art Folder since it is probably a waste of time.
                auto nextDir = QDir(currentFile);
                if (!m_scannerGlobal->testAndMarkDirectoryScanned(nextDir)) {
                    m_pScanner->queueTask(
                            new RecursiveScanDirectoryTask(m_pScanner, m_scannerGlobal,
                                                        nextDir, m_pToken, m_scanUnhashed));
                }
            }
        }
    }
    // Note: A hash of "0" is a real hash if the directory contains no files!
    // Calculate a hash of the directory's file list.
    auto newHash = qHash(newHashStr.join(""));
    auto dirPath = m_dir.path();

    // Try to retrieve a hash from the last time that directory was scanned.
    auto prevHash = m_scannerGlobal->directoryHashInDatabase(dirPath);
    auto prevHashExists = prevHash != -1;

    if (prevHashExists || m_scanUnhashed) {
        // Compare the hashes, and if they don't match, rescan the files in that
        // directory!
        if (prevHash != newHash) {
            // Rescan that mofo! If importing fails then the scan was cancelled so
            // we return immediately.
            if (!filesToImport.isEmpty()) {
                m_pScanner->queueTask(
                        new ImportFilesTask(m_pScanner, m_scannerGlobal, dirPath,
                                            prevHashExists, newHash, filesToImport,
                                            possibleCovers, m_pToken));
            } else {
                emit(directoryHashedAndScanned(dirPath, !prevHashExists, newHash));
            }
        } else {
            emit(directoryUnchanged(dirPath));
        }
    } else {
        m_scannerGlobal->addUnhashedDir(m_dir, m_pToken);
    }
    setSuccess(true);
}
