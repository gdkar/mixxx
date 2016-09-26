#include "controllers/qmlcontrollerenumerator.h"

QmlControllerEnumerator::QmlControllerEnumerator(QObject *p)
: QObject(p)
{
    m_rescanHoldoff.setTimerType (Qt::VeryCoarseTimer);
    m_rescanHoldoff.setInterval  (15 * 1000);
    m_rescanHoldoff.setSingleShot(true);

    connect(&m_directoryWatcher, &QFileSystemWatcher::directoryChanged,this,
        &QmlControllerEnumerator::onDirectoryChanged,
                  static_cast<Qt::ConnectionType>(
            Qt::AutoConnection
           |Qt::UniqueConnection));

    connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged
        ,this,&QmlControllerEnumerator::fileChanged,
            static_cast<Qt::ConnectionType>(Qt::AutoConnection|Qt::UniqueConnection));


}

QmlControllerEnumerator::QmlControllerEnumerator(QStringList searchPaths, QObject *p)
: QObject(p)
, m_searchPaths(searchPaths)
{
    m_directoryWatcher.addPaths(searchPaths);
    refreshFileLists();
}
QmlControllerEnumerator::~QmlControllerEnumerator() = default;
void QmlControllerEnumerator::onDirectoryChanged(QString)
{
    if(m_rescanHoldoff.isActive()) {
        connect(&m_rescanHoldoff,&QTimer::timeout
          , this, &QmlControllerEnumerator::refreshFileLists,
          static_cast<Qt::ConnectionType>(
            Qt::AutoConnection
           |Qt::UniqueConnection));
    }else{
        refreshFileLists();
    }
}

void QmlControllerEnumerator::refreshFileLists()
{
    disconnect(&m_rescanHoldoff,&QTimer::timeout
      , this, &QmlControllerEnumerator::refreshFileLists);
    auto oldScriptNames = m_scriptFileNames;
    auto oldControllerNames = m_controllerFileNames;
    m_scriptFileNames.clear();
    m_controllerFileNames.clear();

    for(auto && dirPath : m_searchPaths) {
        QDirIterator it(dirPath, QDirIterator::Subdirectories);
        while(it.hasNext()) {
            it.next();
            auto info  = it.fileInfo();
            auto path = info.canonicalFilePath();
            if(info.isDir()) {
                qDebug() << "adding recursive directory: " << path;
                m_directoryWatcher.addPath(path);
            }else if (info.suffix().endsWith(".qml",Qt::CaseInsensitive)) {
                qDebug() << "adding controller file : " << path;
                m_controllerFileNames.insert(path);
                m_fileWatcher.addPath(path);
            } else if(info.suffix().endsWith(".js",Qt::CaseInsensitive)) {
                qDebug() << "adding script file : " << path;
                m_scriptFileNames.insert(path);
                m_fileWatcher.addPath(path);
            }else{
                continue;
            }
        }
    }
    m_rescanHoldoff.start();
    if(oldScriptNames != m_scriptFileNames)
        emit scriptFileNamesChanged();
    if(oldControllerNames != m_controllerFileNames)
        emit controllerFileNamesChanged();
}
void QmlControllerEnumerator::setSearchPaths(QStringList s)
{
    m_searchPaths = s;
    auto paths = m_directoryWatcher.directories();
    if(!paths.isEmpty()) {
        m_directoryWatcher.removePaths(paths);
    }
    paths = m_directoryWatcher.files();
    if(!paths.isEmpty()) {
        m_directoryWatcher.removePaths(paths);
    }
    paths = m_fileWatcher.directories();
    if(!paths.isEmpty()) {
        m_fileWatcher.removePaths(paths);
    }
    paths = m_fileWatcher.files();
    if(!paths.isEmpty()) {
        m_fileWatcher.removePaths(paths);
    }
    m_directoryWatcher.addPaths(s);
}
QStringList QmlControllerEnumerator::controllerFileNames() const
{
    return m_controllerFileNames.values();
}
QStringList QmlControllerEnumerator::scriptFileNames() const
{
    return m_scriptFileNames.values();
}
