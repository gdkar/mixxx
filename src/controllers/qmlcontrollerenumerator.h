#ifndef CONTROLLERS_QMLCONTROLLERENUMERATOR_H
#define CONTROLLERS_QMLCONTROLLERENUMERATOR_H
#include <QtGlobal>
#include <QtDebug>
#include <QtQml>
#include <QtQuick>

#include <QDir>
#include <QUrl>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QTimer>
#include "preferences/settingsmanager.h"
class QmlControllerEnumerator : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList controllerFileNames READ controllerFileNames NOTIFY controllerFileNamesChanged)
    Q_PROPERTY(QStringList scriptFileNames READ scriptFileNames NOTIFY scriptFileNamesChanged)
public:
    QmlControllerEnumerator(QObject *p = nullptr);
    QmlControllerEnumerator(QStringList searchPaths, QObject *p = nullptr);
   ~QmlControllerEnumerator();
    QStringList controllerFileNames() const;
    QStringList scriptFileNames() const;
    void setSearchPaths(QStringList);
signals:
    void controllerFileNamesChanged();
    void scriptFileNamesChanged();
protected slots:
    void refreshFileLists();
    void onDirectoryChanged(QString);
protected:
    QFileSystemWatcher m_directoryWatcher{this};
    QTimer             m_rescanHoldoff{this};
    QStringList        m_searchPaths;
    QSet<QString>      m_controllerFileNames;
    QSet<QString>      m_scriptFileNames;

};
#endif
