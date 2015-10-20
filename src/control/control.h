_Pragma("once")
#include <QHash>
#include <QMutex>
#include <QString>
#include <QObject>
#include <QSharedPointer>
#include <memory>
#include <atomic>
#include "control/controlbehavior.h"
#include "configobject.h"

class ControlObject;
class ControlDoublePrivate : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged);
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged);
    Q_PROPERTY(QString group READ group CONSTANT);
    Q_PROPERTY(QString item READ item CONSTANT);
    Q_PROPERTY(double value READ get WRITE set RESET reset NOTIFY valueChanged);
    Q_PROPERTY(double parameter READ getParameter WRITE setParameter RESET reset NOTIFY parameterChanged);
    Q_PROPERTY(double defaultValue READ defaultValue WRITE setDefaultValue NOTIFY defaultValueChanged);
  public:
    virtual ~ControlDoublePrivate();

    // Used to implement control persistence. All controls that are marked
    // "persist in user config" get and set their value on creation/deletion
    // using this ConfigObject.
    static void setUserConfig(ConfigObject<ConfigValue>* pConfig);
    // Adds a ConfigKey for 'alias' to the control for 'key'. Can be used for
    // supporting a legacy / deprecated control. The 'key' control must exist
    // for this to work.
    static void insertAlias(ConfigKey alias, ConfigKey key);
    // Gets the ControlDoublePrivate matching the given ConfigKey. If pCreatorCO
    // is non-NULL, allocates a new ControlDoublePrivate for the ConfigKey if
    // one does not exist.
    static QSharedPointer<ControlDoublePrivate> getControl(
              ConfigKey key
            , bool warn = true
            , ControlObject* pCreatorCO = nullptr
            , bool bTrack = false
            , bool bPersist = false
            );
    // Adds all ControlDoublePrivate that currently exist to pControlList
    static QList<QSharedPointer<ControlDoublePrivate> > getControls();
    static void clearControls();
    static QHash<ConfigKey, ConfigKey> getControlAliases();
    QString name() const;
    void setName(QString name);
    QString description() const; 
    void setDescription(QString );
    // Sets the control value.
    void set(double value);
    // directly sets the control value. Must be used from and only from the
    // ValueChangeRequest slot.
    void setAndConfirm(double value, QObject* pSender);
    // Gets the control value.
    double get() const;
    // Resets the control value to its default.
    void reset();
    // Set the behavior to be used when setting values and translating between
    // parameter and value space. Returns the previously set behavior (if any).
    // The caller must not delete the behavior at any time. The memory is managed
    // by this function.
    void setBehavior(ControlBehavior* pBehavior);
    void setParameter(double dParam);
    double getParameter() const;
    double getParameterForValue(double value) const;
    void setDefaultValue(double dValue);
    double defaultValue() const;
    ControlObject* getCreatorCO() const;
    void removeCreatorCO(ControlObject *);
    ConfigKey getKey() const;
    QString group() const;
    QString item()  const;
    // Connects a slot to the ValueChange request for CO validation. All change
    // requests issued by set are routed though the connected slot. This can
    // decide with its own thread safe solution if the requested value can be
    // confirmed by setAndConfirm() or not. Note: Once connected, the CO value
    // itself is ONLY set by setAndConfirm() typically called in the connected
    // slot.
    bool connectValueChangeRequest(const QObject* receiver, const char* method, Qt::ConnectionType type);
  signals:
    // Emitted when the ControlDoublePrivate value changes. pSender is a
    // pointer to the setter of the value (potentially NULL).
    void valueChanged(double value);
    void valueChangeRequest(double);
    void defaultValueChanged(double);
    void nameChanged(QString);
    void descriptionChanged(QString);
    void parameterChanged();
  private:
    ControlDoublePrivate(ConfigKey key, ControlObject* pCreatorCO,bool bTrack, bool bPersist);
    void initialize();
    void setInner(double value);
    ConfigKey m_key;
    // Whether the control should persist in the Mixxx user configuration. The
    // value is loaded from configuration when the control is created and
    // written to the configuration when the control is deleted.
    bool m_bPersistInConfiguration;
    // User-visible, i18n name for what the control is.
    QString m_name;
    // User-visible, i18n descripton for what the control does.
    QString m_description;
    // Whether to ignore sets which would have no effect.
    // Whether to track value changes with the stats framework.
    bool m_bTrack;
    QString m_trackKey;
    int m_trackType;
    int m_trackFlags;
    bool m_confirmRequired;
    // The control value.
    std::atomic<double> m_value;
    // The default control value.
    std::atomic<double> m_defaultValue;
    QSharedPointer<ControlBehavior> m_pBehavior;
    ControlObject* m_pCreatorCO;
    // Hack to implement persistent controls. This is a pointer to the current
    // user configuration object (if one exists). In general, we do not want the
    // user configuration to be a singleton -- objects that need access to it
    // should be passed it explicitly. However, the Control system is so
    // pervasive that updating every control creation to include the
    // configuration object would be arduous.
    static ConfigObject<ConfigValue>* s_pUserConfig;
    // Hash of ControlDoublePrivate instantiations.
    static QHash<ConfigKey, QSharedPointer<ControlDoublePrivate> > s_qCOHash;
    // Hash of aliases between ConfigKeys. Solely used for looking up the first
    // alias associated with a key.
    static QHash<ConfigKey, ConfigKey> s_qCOAliasHash;
    // Mutex guarding access to s_qCOHash and s_qCOAliasHash.
    static QMutex s_qCOHashMutex;
};
