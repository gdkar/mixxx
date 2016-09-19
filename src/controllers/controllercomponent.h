#ifndef CONTROLLERS_CONTROLLERCOMPONENT_H
#define CONTROLLERS_CONTROLLERCOMPONENT_H
#include <QtGlobal>
#include <QtDebug>
#include <QtQml>
#include <QtQuick>

#include <QDir>
#include <QUrl>
#include <QDirIterator>
#include "preferences/settingsmanager.h"
class ComponentEnumerator : public QObject {
    Q_OBJECT
public:
    ComponentEnumerator(QObject *p = nullptr);
    ComponentEnumerator(QStringList searchPaths, QObject *p = nullptr);
   ~ComponentEnumerator();
    QStringList componentFileNames();
    QStringList scriptFileNames();
protected slots:
    void refreshFileLists();
protected:
    QStringList m_searchPaths;
    QStringList m_componentFileNames;
    QStringList m_scriptFileNames;

};

#endif
