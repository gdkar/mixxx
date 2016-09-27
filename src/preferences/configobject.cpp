#include "preferences/configobject.h"

#include <QIODevice>
#include <QTextStream>
#include <QApplication>
#include <QDir>
#include <QtDebug>

#include "widget/wwidget.h"
#include "util/cmdlineargs.h"
#include "util/xml.h"

// TODO(rryan): Move to a utility file.
namespace {

QString computeResourcePath()
{
    // Try to read in the resource directory from the command line
    auto qResourcePath = CmdlineArgs::Instance().getResourcePath();

    if (qResourcePath.isEmpty()) {
        QDir mixxxDir(QCoreApplication::applicationDirPath());
        // We used to support using the mixxx.cfg's [Config],Path setting but
        // this causes issues if you try and use two different versions of Mixxx
        // on the same computer. See Bug #1392854. We start by checking if we're
        // running out of a build root ('res' dir exists or our path ends with
        // '_build') and if not then we fall back on a platform-specific method
        // of determining the resource path (see comments below).
        if (mixxxDir.cd("res")) {
            // We are running out of the repository root.
            qResourcePath = mixxxDir.absolutePath();
        } else if (mixxxDir.absolutePath().endsWith("_build") &&
                   mixxxDir.cdUp() && mixxxDir.cd("res")) {
            // We are running out of the (lin|win|osx)XX_build folder.
            qResourcePath = mixxxDir.absolutePath();
        }
#ifdef __UNIX__
        // On Linux if all of the above fail the /usr/share path is the logical
        // place to look.
        else {
            qResourcePath = UNIX_SHARE_PATH;
        }
#endif
#ifdef __WINDOWS__
        // On Windows, set the config dir relative to the application dir if all
        // of the above fail.
        else {
            qResourcePath = QCoreApplication::applicationDirPath();
        }
#endif
#ifdef __APPLE__
        else if (mixxxDir.cdUp() && mixxxDir.cd("Resources")) {
            // Release configuraton
            qResourcePath = mixxxDir.absolutePath();
        } else {
            // TODO(rryan): What should we do here?
        }
#endif
    } else {
        //qDebug() << "Setting qResourcePath from location in resourcePath commandline arg:" << qResourcePath;
    }
    if (qResourcePath.isEmpty()) {
        reportCriticalErrorAndQuit("qConfigPath is empty, this can not be so -- did our developer forget to define one of __UNIX__, __WINDOWS__, __APPLE__??");
    }
    // If the directory does not end with a "/", add one
    if (!qResourcePath.endsWith("/")) {
        qResourcePath.append("/");
    }
    qDebug() << "Loading resources from " << qResourcePath;
    return qResourcePath;
}
QString computeSettingsPath(const QString& configFilename)
{
    QFileInfo configFileInfo(configFilename);
    return configFileInfo.absoluteDir().absolutePath();
}

}  // namespace

ConfigKey::ConfigKey() = default;
ConfigKey::ConfigKey(const ConfigKey& key) = default;
ConfigKey::ConfigKey(std::tuple<QString,QString> lst)
    : m_data{std::get<0>(lst),std::get<1>(lst)}
{}
ConfigKey::ConfigKey(const QString& g, const QString& i)
    : m_data{g.trimmed(),i.trimmed()}
{ }

// static
ConfigKey ConfigKey::parseCommaSeparated(const QString& key)
{
    auto comma = key.indexOf(",");
    return ConfigKey (key.left(comma).trimmed(), key.mid(comma + 1).trimmed());
}

ConfigValue::ConfigValue() { }

ConfigValue::ConfigValue(const QString& stValue)
    : value(stValue)
{ }

ConfigValue::ConfigValue(int iValue)
    : value(QString::number(iValue))
{ }

void ConfigValue::valCopy(const ConfigValue& configValue)
{
    value = configValue.value;
}

ConfigValueKbd::ConfigValueKbd()
{}

ConfigValueKbd::ConfigValueKbd(const QString& value)
        : ConfigValue(value)
{
    m_qKey = QKeySequence(value);
}
ConfigValueKbd::ConfigValueKbd(const QKeySequence& key)
{
    m_qKey = key;
    QTextStream(&value) << m_qKey.toString();
    // qDebug() << "value" << value;
}
void ConfigValueKbd::valCopy(const ConfigValueKbd& v)
{
    m_qKey = v.m_qKey;
    QTextStream(&value) << m_qKey.toString();
}
bool operator==(const ConfigValue& s1, const ConfigValue& s2)
{
    return (s1.value.toUpper() == s2.value.toUpper());
}
bool operator==(const ConfigValueKbd& s1, const ConfigValueKbd& s2)
{
    //qDebug() << s1.m_qKey << "==" << s2.m_qKey;
    return (s1.m_qKey == s2.m_qKey);
}
template <class ValueType> ConfigObject<ValueType>::ConfigObject(const QString& file)
        : m_resourcePath(computeResourcePath()),
          m_settingsPath(computeSettingsPath(file))
{
    reopen(file);
}
template <class ValueType> ConfigObject<ValueType>::~ConfigObject() {
}
template <class ValueType>
void ConfigObject<ValueType>::set(const ConfigKey& k, const ValueType& v) {
    QWriteLocker lock(&m_valuesLock);
    m_values.insert(k, v);
}
template <class ValueType>
ValueType ConfigObject<ValueType>::get(const ConfigKey& k) const {
    QReadLocker lock(&m_valuesLock);
    return m_values.value(k);
}
template <class ValueType>
bool ConfigObject<ValueType>::exists(const ConfigKey& k) const {
    QReadLocker lock(&m_valuesLock);
    return m_values.contains(k);
}
template <class ValueType>
QString ConfigObject<ValueType>::getValueString(const ConfigKey& k) const {
    return get(k).value;
}

template <class ValueType>
QString ConfigObject<ValueType>::getValueString(const ConfigKey& k,
                                                const QString& default_string) const {
    auto ret = getValueString(k);
    if (ret.isEmpty()) {
        return default_string;
    }
    return ret;
}

template <class ValueType> bool ConfigObject<ValueType>::parse() {
    // Open file for reading
    QFile configfile(m_filename);
    if (m_filename.length() < 1 || !configfile.open(QIODevice::ReadOnly)) {
        qDebug() << "ConfigObject: Could not read" << m_filename;
        return false;
    } else {
        //qDebug() << "ConfigObject: Parse" << m_filename;
        // Parse the file
        int group = 0;
        QString groupStr, line;
        QTextStream text(&configfile);
        text.setCodec("UTF-8");

        while (!text.atEnd()) {
            line = text.readLine().trimmed();
            if (line.length() != 0) {
                if (line.startsWith("[") && line.endsWith("]")) {
                    group++;
                    groupStr = line.trimmed();
                    //qDebug() << "Group :" << groupStr;
                } else if (group > 0) {
                    QString key;
                    QTextStream(&line) >> key;
                    key = key.trimmed();
                    auto val = line.right(line.length() - key.length()).trimmed(); // finds the value string
                    //qDebug() << "control:" << key << "value:" << val;
                    ConfigKey k(groupStr, key);
                    ValueType m(val);
                    set(k, m);
                }
            }
        }
        configfile.close();
    }
    return true;
}

template <class ValueType> void ConfigObject<ValueType>::reopen(const QString& file) {
    m_filename = file;
    if (!m_filename.isEmpty()) {
        parse();
    }
}

template <class ValueType> void ConfigObject<ValueType>::save() {
    QReadLocker lock(&m_valuesLock); // we only read the m_values here.
    QFile file(m_filename);
    if (!QDir(QFileInfo(file).absolutePath()).exists()) {
        QDir().mkpath(QFileInfo(file).absolutePath());
    }
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Could not write file" << m_filename << ", don't worry.";
        return;
    } else {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        auto grp = QString{};

        for (auto i = m_values.constBegin(); i != m_values.constEnd(); ++i) {
            if (i.key().group().trimmed() != grp.trimmed()) {
                grp = i.key().group().trimmed();
                stream << "\n" << grp << "\n";
            }
            stream << i.key().item().trimmed() << " " << i.value().value.trimmed() << "\n";
        }
        file.close();
        if (file.error()!=QFile::NoError) { //could be better... should actually say what the error was..
            qDebug() << "Error while writing configuration file:" << file.errorString();
        }
    }
}

template <class ValueType> ConfigObject<ValueType>::ConfigObject(const QDomNode& node)
{
    if (!node.isNull() && node.isElement()) {
        auto ctrl = node.firstChild();

        while (!ctrl.isNull()) {
            if(ctrl.nodeName() == "control") {
                auto group = XmlParse::selectNodeQString(ctrl, "group");
                auto key = XmlParse::selectNodeQString(ctrl, "key");
                ConfigKey k(group, key);
                ValueType m(ctrl);
                set(k, m);
            }
            ctrl = ctrl.nextSibling();
        }
    }
}

template <class ValueType>
QMultiHash<ValueType, ConfigKey> ConfigObject<ValueType>::transpose() const {
    QReadLocker lock(&m_valuesLock);

    QMultiHash<ValueType, ConfigKey> transposedHash;
    for (auto it = m_values.cbegin(); it != m_values.cend(); ++it) {
        transposedHash.insert(it.value(), it.key());
    }
    return transposedHash;
}

template class ConfigObject<ConfigValue>;
template class ConfigObject<ConfigValueKbd>;

template <> template <>
void ConfigObject<ConfigValue>::setValue(const ConfigKey& key, const QString& value)
{
    set(key, ConfigValue(value));
}

template <> template <>
void ConfigObject<ConfigValue>::setValue(const ConfigKey& key, const bool& value)
{
    set(key, value ? ConfigValue("1") : ConfigValue("0"));
}

template <> template <>
void ConfigObject<ConfigValue>::setValue(const ConfigKey& key, const int& value)
{
    set(key, ConfigValue(QString::number(value)));
}

template <> template <>
bool ConfigObject<ConfigValue>::getValue(const ConfigKey& key,
                                         const bool& default_value) const {
    auto value = get(key);
    if (value.isNull()) {
        return default_value;
    }
    return static_cast<bool>(value.value.toInt());
}

template <> template <>
int ConfigObject<ConfigValue>::getValue(const ConfigKey& key,
                                        const int& default_value) const {
    auto value = get(key);
    if (value.isNull()) {
        return default_value;
    }
    return value.value.toInt();
}

template <> template <>
QString ConfigObject<ConfigValue>::getValue(const ConfigKey& key,
                                            const QString& default_value) const {
    auto value = get(key);
    if (value.isNull()) {
        return default_value;
    }
    return value.value;
}
