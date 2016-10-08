#ifndef CONTROL_H
#define CONTROL_H

#include <QHash>
#include <QString>
#include <QObject>
#include <QAtomicPointer>

#include "control/controlhint.h"
#include "control/controlbehavior.h"
#include "control/controlvalue.h"
#include "preferences/usersettings.h"
#include "util/mutex.h"

class ControlObject;

class ControlDoublePrivate : public QObject, public QEnableSharedFromThis<ControlDoublePrivate> {
    Q_OBJECT
    Q_PROPERTY(ConfigKey key READ getKey CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(double value READ get WRITE set RESET reset NOTIFY valueChanged)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)
    Q_PROPERTY(double default READ defaultValue WRITE setDefaultValue NOTIFY defaultValueChanged)
    Q_PROPERTY(ControlHint range READ range WRITE setRange NOTIFY rangeChanged);
  public:
    virtual ~ControlDoublePrivate();

    // Used to implement control persistence. All controls
    // "persist in user config" get and set their value on creation/deletion
    // using this UserSettings.
    static void setUserConfig(UserSettingsPointer pConfig);

    // Adds a ConfigKey for 'alias' to the control for 'key'. Can be used for
    // supporting a legacy / deprecated control. The 'key' control must exist
    // for this to work.
    static void insertAlias(ConfigKey alias, ConfigKey key);

    // Gets the ControlDoublePrivate matching the given ConfigKey. If pCreatorCO
    // is non-NULL, allocates a new ControlDoublePrivate for the ConfigKey if
    // one does not exist.
    static QSharedPointer<ControlDoublePrivate> getControl(
            ConfigKey key, bool warn = false, bool bIgnoreNops = true, bool bTrack = false,
            bool bPersist = false);
    static QSharedPointer<ControlDoublePrivate> getIfExists(ConfigKey key);

    // Adds all ControlDoublePrivate that currently exist to pControlList
    static void getControls(QList<QSharedPointer<ControlDoublePrivate> >* pControlsList);

    static QHash<ConfigKey, ConfigKey> getControlAliases();
    double exchange(double with);
    const ControlHint  &range() const;
    ControlHint &range();
    void setRange(const ControlHint &hint);
    template<class Func>
    double updateAtomically(Func &&func)
    {
        auto expected = m_value.load(std::memory_order_acquire);
        auto desired  = expected;
        do {
            desired = func(expected);
            if(desired == expected)
                return desired;
        }while(!m_value.compare_exchange_strong(
            expected,
            desired,
            std::memory_order_acq_rel,
            std::memory_order_acquire));
        emit valueChanged(desired,nullptr);
        return desired;
    }
    bool compare_exchange_strong(double &expected, double desired);
  public slots:
    QString name() const;
    void setName(QString name);
    QString description() const;
    void setDescription(QString description);
    double minimum() const;
    void setMinimum(double value);
    double maximum() const;
    void setMaximum(double value);

    // Sets the control value.
    void set(double value, QObject* pSender = nullptr);
    // directly sets the control value. Must be used from and only from the
    // ValueChangeRequest slot.
    void setAndConfirm(double value, QObject* pSender = nullptr);
    // Gets the control value.
    double get() const;
    // Resets the control value to its default.
    void reset();
    // Set the behavior to be used when setting values and translating between
    // parameter and value space. Returns the previously set behavior (if any).
    // The caller must not delete the behavior at any time. The memory is managed
    // by this function.
    void setBehavior(ControlNumericBehavior* pBehavior);

    void setParameter(double dParam, QObject* pSender);
    double getParameter() const;
    double getParameterForValue(double value) const;


    bool ignoreNops() const;
    void setDefaultValue(double dValue);
    double defaultValue() const;
    ControlObject* getCreatorCO() const;
    ConfigKey getKey();

    // Connects a slot to the ValueChange request for CO validation. All change
    // requests issued by set are routed though the connected slot. This can
    // decide with its own thread safe solution if the requested value can be
    // confirmed by setAndConfirm() or not. Note: Once connected, the CO value
    // itself is ONLY set by setAndConfirm() typically called in the connected
    // slot.
    bool connectValueChangeRequest(const QObject* receiver,
                                   const char* method, Qt::ConnectionType type);

  signals:
    // Emitted when the ControlDoublePrivate value changes. pSender is a
    // pointer to the setter of the value (potentially NULL).
    void minimumChanged(double value);
    void maximumChanged(double value);
    void valueChanged(double value, QObject* pSender);
    void defaultValueChanged(double);
    void valueChangeRequest(double value);
    void trigger();
    void nameChanged();
    void descriptionChanged();
    void rangeChanged(const ControlHint &hint);
  private:
    ControlDoublePrivate(ConfigKey key,
                         bool bIgnoreNops, bool bTrack, bool bPersist);
    void initialize();
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
    bool m_bIgnoreNops;
    // Whether to track value changes with the stats framework.
    bool m_bTrack;
    QString m_trackKey;
    int m_trackType;
    int m_trackFlags;
    bool m_confirmRequired;
    // The control value.
    std::atomic<double> m_value;
    // The default control value.
    std::atomic<double> m_minimum{-std::numeric_limits<double>::infinity()};
    std::atomic<double> m_maximum{std::numeric_limits<double>::infinity()};
    std::atomic<double> m_defaultValue;
    ControlHint m_range{};
    QSharedPointer<ControlNumericBehavior> m_pBehavior;
    ControlObject* m_pCreatorCO{};
    // Hack to implement persistent controls. This is a pointer to the current
    // user configuration object (if one exists). In general, we do not want the
    // user configuration to be a singleton -- objects that need access to it
    // should be passed it explicitly. However, the Control system is so
    // pervasive that updating every control creation to include the
    // configuration object would be arduous.
    static UserSettingsPointer s_pUserConfig;
    // Hash of ControlDoublePrivate instantiations.
    static QHash<ConfigKey, QWeakPointer<ControlDoublePrivate> > s_qCOHash;
    // Hash of aliases between ConfigKeys. Solely used for looking up the first
    // alias associated with a key.
    static QHash<ConfigKey, ConfigKey> s_qCOAliasHash;
    // Mutex guarding access to s_qCOHash and s_qCOAliasHash.
    static MMutex s_qCOHashMutex;
    friend class QSharedPointer<ControlDoublePrivate>;
};


#endif /* CONTROL_H */
