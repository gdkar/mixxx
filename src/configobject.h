/***************************************************************************
                          configobject.h  -  description
                             -------------------
    begin                : Thu Jun 6 2002
    copyright            : (C) 2002 by Tue & Ken Haste Andersen
    email                : haste@diku.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CONFIGOBJECT_H
#define CONFIGOBJECT_H

#include <QString>
#include <QFile>
#include <QKeySequence>
#include <QDomNode>
#include <QMap>
#include <QHash>
#include <QMetaType>

#include "util/debug.h"


// Class for the key for a specific configuration element. A key consists of a
// group and an item.

class ConfigKey:public QObject {
  Q_OBJECT
  Q_PROPERTY(QString group READ getGroup)
  Q_PROPERTY(QString item  READ getItem)
  public:
    ConfigKey();
    ConfigKey(const ConfigKey& other):QObject(),group(other.group),item(other.item){
    }
    ConfigKey(const QString& g, const QString& i);
    ConfigKey(const char* g, const char* i);
    ConfigKey &operator = (ConfigKey other){
      group=other.group;
      item =other.item;
      return *this;
    }
    QString getGroup()const {return group;}
    QString getItem()const {return item;}
    static ConfigKey parseCommaSeparated(QString key);

    inline bool isNull() const {
        return group.isNull() && item.isNull();
    }

    QString group, item;
};
Q_DECLARE_METATYPE(ConfigKey);

// comparison function for ConfigKeys. Used by a QHash in ControlObject
inline bool operator==(const ConfigKey& c1, const ConfigKey& c2) {
    return c1.group == c2.group && c1.item == c2.item;
}

// stream operator function for trivial qDebug()ing of ConfigKeys
inline QDebug operator<<(QDebug stream, const ConfigKey& c1) {
    stream << c1.group << "," << c1.item;
    return stream;
}

// QHash hash function for ConfigKey objects.
inline uint qHash(const ConfigKey& key) {
    return qHash(key.group) ^ qHash(key.item);
}

inline uint qHash(const QKeySequence& key) {
    return qHash(key.toString());
}


// The value corresponding to a key. The basic value is a string, but can be
// subclassed to more specific needs.
class ConfigValue:public QObject {
  Q_OBJECT
  Q_PROPERTY(QString value READ getValue WRITE setValue)
  public:
    ConfigValue();
    ConfigValue(const ConfigValue &other):QObject(),value(other.value){}
    ConfigValue(QString _value);
    ConfigValue(int _value);
    ConfigValue & operator = (ConfigValue other){
      valCopy(other);
      return *this;
    }
    virtual QString getValue()const{return value;}
    virtual void setValue(const QString &val){value =val;}
    inline ConfigValue(QDomNode /* node */) {
        reportFatalErrorAndQuit("ConfigValue from QDomNode not implemented here");
    }
    void valCopy(const ConfigValue& _value);

    QString value;
    friend bool operator==(const ConfigValue& s1, const ConfigValue& s2);
};
Q_DECLARE_METATYPE(ConfigValue)

class ConfigValueKbd : public ConfigValue {
  public:
    ConfigValueKbd();
    ConfigValueKbd(QString _value);
    ConfigValueKbd(QKeySequence key);
    inline ConfigValueKbd(QDomNode /* node */) {
        reportFatalErrorAndQuit("ConfigValueKbd from QDomNode not implemented here");
    }
    void valCopy(const ConfigValueKbd& v);
    friend bool operator==(const ConfigValueKbd& s1, const ConfigValueKbd& s2);

    QKeySequence m_qKey;
};

template <class ValueType> class ConfigOption :public QObject{
  Q_OBJECT
  Q_PROPERTY(ConfigKey key READ getKey CONSTANT )
  Q_PROPERTY(ValueType value READ getValue CONSTANT )
  public:
    ConfigOption() { val = NULL; key = NULL;};
    ConfigOption(ConfigKey* _key, ValueType* _val) { key = _key ; val = _val; };
    virtual ConfigKey getKey()const{return key? *key:ConfigKey();}
    virtual ValueType getValue()const{return val?*val:ValueType();}
    virtual ~ConfigOption() {
        delete key;
        delete val;
    }
    ValueType* val;
    ConfigKey* key;
};

template <class KeyType, class ValueType> class ConfigObjectBase : public QObject {
  Q_OBJECT
  Q_PROPERTY(ValueType valueString READ getValueString)
  Q_PROPERTY(QString resourcePath READ getResourcePath CONSTANT)
  Q_PROPERTY(QString settingsPath READ getSettingsPath CONSTANT)
  public:
    KeyType key;
    ValueType value;
    ConfigOption<ValueType> option;

    ConfigObjectBase(){}
    virtual ~ConfigObjectBase(){}
    virtual ConfigOption<ValueType> *set(KeyType, ValueType)=0;
    virtual ConfigOption<ValueType> *get(KeyType key)= 0;
    Q_INVOKABLE virtual bool exists(KeyType key) =0;
    Q_INVOKABLE virtual KeyType *get(ValueType v)=0;
    Q_INVOKABLE virtual QString getValueString(KeyType k )=0;
    Q_INVOKABLE virtual QString getValueString(KeyType k, const QString& default_string)=0;
    virtual QHash<KeyType, ValueType> toHash() const =0;

    virtual void clear()= 0;
    virtual void reopen(QString file)=0;
    virtual void Save() = 0;

    // Returns the resource path -- the path where controller presets, skins,
    // library schema, keyboard mappings, and more are stored.
    virtual QString getResourcePath() const = 0;

    // Returns the settings path -- the path where user data (config file,
    // library SQLite database, etc.) is stored.
    virtual QString getSettingsPath() const = 0;

};
template<class ValueType>
class ConfigObject : public ConfigObjectBase<ConfigKey,ValueType>{
  Q_OBJECT
  Q_PROPERTY(ValueType valueString READ getValueString)
  Q_PROPERTY(QString resourcePath READ getResourcePath CONSTANT)
  Q_PROPERTY(QString settingsPath READ getSettingsPath CONSTANT)
  public:
    ConfigKey key;
    ValueType value;
    ConfigOption<ValueType> option;

    ConfigObject(QString file);
    ConfigObject(QDomNode node);
    virtual ~ConfigObject();
    virtual ConfigOption<ValueType> *set(ConfigKey, ValueType);
    virtual ConfigOption<ValueType> *get(ConfigKey key);
    Q_INVOKABLE virtual bool exists(ConfigKey key);
    Q_INVOKABLE virtual ConfigKey *get(ValueType v);
    Q_INVOKABLE virtual QString getValueString(ConfigKey k );
    Q_INVOKABLE virtual QString getValueString(ConfigKey k, const QString& default_string);
    virtual QHash<ConfigKey, ValueType> toHash() const;

    virtual void clear();
    virtual void reopen(QString file);
    virtual void Save();

    // Returns the resource path -- the path where controller presets, skins,
    // library schema, keyboard mappings, and more are stored.
    virtual QString getResourcePath() const;

    // Returns the settings path -- the path where user data (config file,
    // library SQLite database, etc.) is stored.
    virtual QString getSettingsPath() const;
  protected:
    QList<ConfigOption<ValueType>*> m_list;
    QString m_filename;

    // Loads and parses the configuration file. Returns false if the file could
    // not be opened; otherwise true.
    bool Parse();

};
Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE(ConfigObjectBase)

#endif
