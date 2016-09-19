#include "controllers/controllercomponent.h"

ComponentEnumerator::ComponentEnumerator(QObject *p)
: QObject(p)
{}

ComponentEnumerator::ComponentEnumerator(QStringList searchPaths, QObject *p)
: QObject(p)
, m_searchPaths(searchPaths)
{}
ComponentEnumerator::~ComponentEnumerator() = default;

void ComponentEnumerator::refreshFileLists()
{
    m_scriptFileNames.clear();
    m_componentFileNames.clear();
    for(auto && dirPath : m_searchPaths) {
        QDirIterator it(dirPath);
        while(it.hasNext()) {
            it.next();
            auto path = it.filePath();
            if (path.endsWith(".qml",Qt::CaseInsensitive))
                m_componentFileNames.append(path);
            else if(path.endsWith(".js",Qt::CaseInsensitive))
                m_scriptFileNames.append(path);
        }
    }
}

QStringList ComponentEnumerator::componentFileNames()
{
    refreshFileLists();
    return m_componentFileNames;
}
QStringList ComponentEnumerator::scriptFileNames()
{
    refreshFileLists();
    return m_scriptFileNames;
}
