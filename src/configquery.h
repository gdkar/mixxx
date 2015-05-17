#ifndef CONFIGQUERY_H
#define CONFIGQUERY_H

#include <qglobal.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qatomic.h>
#include <qobject.h>
#include <qmetatype.h>


class ConfigQuery{
  QStringList     m_keys;
public:
  ConfigQuery(QStringList &args)
    : m_keys(args){}
  ConfigQuery(QString &alltogether)
    :m_keys(alltogether.split(".")){}
  QVariant &query(QObject *root){
    for(int i = 0; i < m_keys.size()-1;i++){
      QVariant next = root->property(m_keys[i]);
      if(!next.isValid() || !(root = next.value<QObject*>()))
        return QVariant();
    }
    return root->property(m_keys[i]);

  }
};
#endif
