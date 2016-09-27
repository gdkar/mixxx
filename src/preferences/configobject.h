#ifndef PREFERENCES_CONFIGOBJECT_H
#define PREFERENCES_CONFIGOBJECT_H

#include <QString>
#include <QKeySequence>
#include <QStringList>
#include <QDomNode>
#include <QMap>
#include <QHash>
#include <QMetaType>
#include <QReadWriteLock>
#include <initializer_list>
#include <algorithm>
#include <utility>
#include "util/debug.h"

// Class for the key for a specific configuration element. A key consists of a
// group and an item.
class ConfigKey {
    Q_GADGET
    Q_PROPERTY(QString group READ group WRITE setGroup);
    Q_PROPERTY(QString item READ item WRITE setItem);
    Q_PROPERTY(QStringList sections READ sections WRITE setSections);
  public:
    Q_INVOKABLE ConfigKey(); // is required for qMetaTypeConstructHelper()
    Q_INVOKABLE ConfigKey(const ConfigKey& key);
    Q_INVOKABLE ConfigKey(const QString& g, const QString& i);
    Q_INVOKABLE explicit ConfigKey(std::tuple<QString,QString> lst);
    static ConfigKey parseCommaSeparated(const QString& key);
    virtual ~ConfigKey() = default;

    Q_INVOKABLE bool isEmpty() const {
        return std::all_of(m_data.cbegin(),m_data.cend(),[](auto && str){return str.isEmpty();});
    }
    Q_INVOKABLE bool isNull() const {
        return std::all_of(m_data.cbegin(),m_data.cend(),[](auto && str){return str.isNull();});
    }
    Q_INVOKABLE const QString &group() const { return m_data[0]; }
    Q_INVOKABLE QString &group() { return m_data[0]; }
    Q_INVOKABLE void setGroup(const QString &group) { m_data[0] = group; }
    Q_INVOKABLE const QString &item() const { return m_data[1]; }
    Q_INVOKABLE QString &item() { return m_data[1]; }
    Q_INVOKABLE void setItem(const QString &item) { m_data[1] = item; }
    Q_INVOKABLE QStringList &sections() { return m_data;}
    Q_INVOKABLE const QStringList &sections() const { return m_data;}
    Q_INVOKABLE void setSections(const QStringList &lst){
        m_data = lst;
        while(m_data.size() < 2)
            m_data.append(QString{});
    }
    operator std::tuple<QString&,QString&> ()
    {
        return {group(),item()};
    }
    operator std::tuple<QString,QString> () const
    {
        return {group(),item()};
    }
    // comparison function for ConfigKeys. Used by a QHash in ControlObject
    friend bool operator==(const ConfigKey& lhs, const ConfigKey& rhs)
    {
        return static_cast<const std::tuple<QString,QString> >(lhs) ==
            static_cast<const std::tuple<QString,QString> >(rhs);
    }
    friend bool operator!=(const ConfigKey &lhs, const ConfigKey& rhs)
    {
        return !(lhs==rhs);
    }
    // comparison function for ConfigKeys. Used by a QMap in ControlObject
    friend bool operator<(const ConfigKey& lhs, const ConfigKey& rhs)
    {
        return static_cast<const std::tuple<QString,QString> >(lhs) <
            static_cast<const std::tuple<QString,QString> >(rhs);
    }
    // comparison function for ConfigKeys. Used by a QMap in ControlObject
    friend bool operator>(const ConfigKey& lhs, const ConfigKey& rhs)
    {
        return (rhs<lhs);
    }
    // comparison function for ConfigKeys. Used by a QMap in ControlObject
    friend bool operator<=(const ConfigKey& lhs, const ConfigKey& rhs)
    {
        return !(rhs<lhs);
    }
    // comparison function for ConfigKeys. Used by a QMap in ControlObject
    friend bool operator>=(const ConfigKey& lhs, const ConfigKey& rhs)
    {
        return !(lhs<rhs);
    }
    friend QDebug operator<<(QDebug stream, const ConfigKey& c1)
    {
        return stream << "["<< c1.group() << "," << c1.item() <<"]";
    }
    friend uint qHash(const ConfigKey& key)
    {
        return qHash(key.group()) ^ qHash(key.item());
    }
    QStringList m_data{QString{},QString{}};
//    QString m_group, m_item;
};

Q_DECLARE_METATYPE(ConfigKey);

// stream operator function for trivial qDebug()ing of ConfigKeys


// QHash hash function for ConfigKey objects.


inline uint qHash(const QKeySequence& key)
{
    return qHash(key.toString());
}


// The value corresponding to a key. The basic value is a string, but can be
// subclassed to more specific needs.
class ConfigValue {
  public:
    ConfigValue();
    // Only allow non-explicit QString -> ConfigValue conversion for
    // convenience. All other types must be explicit.
    ConfigValue(const QString& value);
    explicit ConfigValue(int value);
    explicit ConfigValue(const QDomNode& /* node */) {
        reportFatalErrorAndQuit("ConfigValue from QDomNode not implemented here");
    }
    void valCopy(const ConfigValue& value);
    bool isNull() const { return value.isNull(); }

    QString value;
    friend bool operator==(const ConfigValue& s1, const ConfigValue& s2);
};

inline uint qHash(const ConfigValue& key) {
    return qHash(key.value.toUpper());
}

class ConfigValueKbd : public ConfigValue {
  public:
    ConfigValueKbd();
    explicit ConfigValueKbd(const QString& _value);
    explicit ConfigValueKbd(const QKeySequence& key);
    explicit ConfigValueKbd(const QDomNode& /* node */) {
        reportFatalErrorAndQuit("ConfigValueKbd from QDomNode not implemented here");
    }
    void valCopy(const ConfigValueKbd& v);
    friend bool operator==(const ConfigValueKbd& s1, const ConfigValueKbd& s2);

    QKeySequence m_qKey;
};

template <class ValueType> class ConfigObject {
  public:
    ConfigObject(const QString& file);
    ConfigObject(const QDomNode& node);
    ~ConfigObject();

    void set(const ConfigKey& k, const ValueType& v);
    ValueType get(const ConfigKey& k) const;
    bool exists(const ConfigKey& key) const;
    QString getValueString(const ConfigKey& k) const;
    QString getValueString(const ConfigKey& k, const QString& default_string) const;

    template <class ResultType>
    void setValue(const ConfigKey& key, const ResultType& value);

    template <class ResultType>
    ResultType getValue(const ConfigKey& key) const {
        return getValue<ResultType>(key, ResultType());
    }

    template <class ResultType>
    ResultType getValue(const ConfigKey& key, const ResultType& default_value) const;

    QMultiHash<ValueType, ConfigKey> transpose() const;

    void reopen(const QString& file);
    void save();

    // Returns the resource path -- the path where controller presets, skins,
    // library schema, keyboard mappings, and more are stored.
    QString getResourcePath() const {
        return m_resourcePath;
    }

    // Returns the settings path -- the path where user data (config file,
    // library SQLite database, etc.) is stored.
    QString getSettingsPath() const {
        return m_settingsPath;
    }

  protected:
    // We use QMap because we want a sorted list in mixxx.cfg
    QMap<ConfigKey, ValueType> m_values;
    mutable QReadWriteLock m_valuesLock;
    QString m_filename;
    const QString m_resourcePath;
    const QString m_settingsPath;

    // Loads and parses the configuration file. Returns false if the file could
    // not be opened; otherwise true.
    bool parse();
};

#endif // PREFERENCES_CONFIGOBJECT_H
