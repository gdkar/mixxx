_Pragma("once")

extern "C" {
#include <ladspa.h>
}
#include <QLibrary>
#include <QObject>
#include <QtGlobal>
#include <QtDebug>
#include <QtQml>
#include <QtQuick>
#include <QMetaEnum>
#include <QMetaType>

class LADSPA : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE LADSPA(QObject *pParent = nullptr);
   ~LADSPA();
    using Data = LADSPA_Data;
    static void registerTypes();
    enum class LadspaProperty : LADSPA_Properties {
        Realtime = LADSPA_PROPERTY_REALTIME,
        InplaceBroken = LADSPA_PROPERTY_INPLACE_BROKEN,
        HardRTCapable = LADSPA_PROPERTY_HARD_RT_CAPABLE,
    };
    Q_DECLARE_FLAGS(Properties, LadspaProperty)
    enum class LadspaPort :  LADSPA_PortDescriptor {
        Input   = LADSPA_PORT_INPUT,
        Output  = LADSPA_PORT_OUTPUT,
        Control = LADSPA_PORT_CONTROL,
        Audio   = LADSPA_PORT_AUDIO,
    };
    Q_DECLARE_FLAGS(PortDescriptor,LadspaPort);
    enum struct Hint : LADSPA_PortRangeHintDescriptor {
        BoundedBelow = LADSPA_HINT_BOUNDED_BELOW,
        BoundedAbove = LADSPA_HINT_BOUNDED_ABOVE,
        Toggled      = LADSPA_HINT_TOGGLED,
        SampleRate   = LADSPA_HINT_SAMPLE_RATE,
        Logarithmic  = LADSPA_HINT_LOGARITHMIC,
        Integer      = LADSPA_HINT_INTEGER,
        DefaultMask  = LADSPA_HINT_DEFAULT_MASK,
/*        DefaultMinimum = LADSPA_HINT_DEFAULT_MINIMUM,
        DefaultLow     = LADSPA_HINT_DEFAULT_LOW,
        DefaultMiddle  = LADSPA_HINT_DEFAULT_MIDDLE,
        DefaultHigh    = LADSPA_HINT_DEFAULT_HIGH,
        DefaultMaximum = LADSPA_HINT_DEFAULT_MAXIMUM,
        Default0       = LADSPA_HINT_DEFAULT_0,
        Default1       = LADSPA_HINT_DEFAULT_1,
        Default100     = LADSPA_HINT_DEFAULT_100,
        Default440     = LADSPA_HINT_DEFAULT_440,*/
    };
    Q_DECLARE_FLAGS(PortRangeHintDescriptor, Hint)
    enum struct HintDefault : LADSPA_PortRangeHintDescriptor {
        None       = LADSPA_HINT_DEFAULT_NONE,
        Mask       = LADSPA_HINT_DEFAULT_MASK,
        Minimum    = LADSPA_HINT_DEFAULT_MINIMUM,
        Low        = LADSPA_HINT_DEFAULT_LOW,
        Middle     = LADSPA_HINT_DEFAULT_MIDDLE,
        High       = LADSPA_HINT_DEFAULT_HIGH,
        Maximum    = LADSPA_HINT_DEFAULT_MAXIMUM,
        Zero       = LADSPA_HINT_DEFAULT_0,
        One        = LADSPA_HINT_DEFAULT_1,
        OneHundred = LADSPA_HINT_DEFAULT_100,
        FourForty = LADSPA_HINT_DEFAULT_440,
    };
    Q_ENUM(HintDefault);
};
QML_DECLARE_TYPE(LADSPA);
Q_DECLARE_METATYPE(LADSPA::Data)

Q_DECLARE_OPERATORS_FOR_FLAGS(LADSPA::Properties)
Q_DECLARE_OPERATORS_FOR_FLAGS(LADSPA::PortDescriptor)
Q_DECLARE_OPERATORS_FOR_FLAGS(LADSPA::PortRangeHintDescriptor)

class PortRangeHint : public QObject {
    Q_OBJECT
    Q_PROPERTY(LADSPA::PortRangeHintDescriptor hintDescriptor READ hintDescriptor WRITE setHintDescriptor NOTIFY hintDescriptorChanged)
    Q_PROPERTY(LADSPA::HintDefault hintDefault READ hintDefault WRITE setHintDefault NOTIFY hintDefaultChanged)
    Q_PROPERTY(LADSPA::Data lowerBound READ lowerBound WRITE setLowerBound NOTIFY lowerBoundChanged)
    Q_PROPERTY(LADSPA::Data upperBound READ upperBound WRITE setUpperBound NOTIFY upperBoundChanged)
    Q_PROPERTY(LADSPA::Data defaultValue READ defaultValue NOTIFY defaultValueChanged)
    Q_PROPERTY(quint64 sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged);
public:
    Q_INVOKABLE PortRangeHint(QObject *pParent= nullptr);
    PortRangeHint(const LADSPA_PortRangeHint*_d, QObject *pParent=nullptr);
   ~PortRangeHint();
    LADSPA::PortRangeHintDescriptor hintDescriptor() const;
    void setHintDescriptor(LADSPA::PortRangeHintDescriptor hint);
    LADSPA::Data lowerBound() const;
    void setLowerBound(LADSPA::Data bound);
    LADSPA::Data upperBound() const;
    void setUpperBound(LADSPA::Data bound);
    LADSPA::HintDefault hintDefault() const;
    void setHintDefault(LADSPA::HintDefault def);
    quint64 sampleRate() const;
    void setSampleRate(quint64);
    LADSPA::Data defaultValue() const;
signals:
    void hintDefaultChanged(LADSPA::HintDefault def);
    void hintDescriptorChanged(LADSPA::PortRangeHintDescriptor);
    void lowerBoundChanged(LADSPA::Data);
    void upperBoundChanged(LADSPA::Data);
    void defaultValueChanged(LADSPA::Data);
    void sampleRateChanged(quint64);
protected:
    quint64                         m_sampleRate{1};
    LADSPA::PortRangeHintDescriptor m_hintDescriptor{};
    LADSPA::Data                    m_upperBound{};
    LADSPA::Data                    m_lowerBound{};
};

QML_DECLARE_TYPE(PortRangeHint)
class Descriptor;
class PortInfo : public QObject {
    Q_OBJECT
    Q_PROPERTY(Descriptor* descriptor READ descriptor CONSTANT)
    Q_PROPERTY(quint64 index READ index CONSTANT)
    Q_PROPERTY(LADSPA::PortDescriptor portDescriptor READ portDescriptor CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(PortRangeHint * rangeHint READ rangeHint CONSTANT)
public:
    PortInfo(QObject *pParent= nullptr);
    PortInfo(const LADSPA_Descriptor *desc, quint64 index, QObject *pParent = nullptr);
    virtual LADSPA::PortDescriptor portDescriptor() const { return m_desc;}
    virtual Descriptor *descriptor() const;
    virtual QString name() const { return {m_name};}
    virtual PortRangeHint* rangeHint() const { return m_rangeHint;}
    virtual quint64 index() const { return m_index;}
protected:
    quint64                 m_index{};
    LADSPA::PortDescriptor  m_desc{};
    QString                 m_name{};
    PortRangeHint          *m_rangeHint{};
};
QML_DECLARE_TYPE(PortInfo)
class LadspaHandle;
class Descriptor : public QObject {
    Q_OBJECT
    Q_PROPERTY(quint64 uniqueID READ uniqueID CONSTANT)
    Q_PROPERTY(QString label READ label CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString maker READ maker CONSTANT)
    Q_PROPERTY(QString copyright READ copyright CONSTANT)
    Q_PROPERTY(QQmlListProperty<PortInfo> ports READ ports);
    static void AppendPort(QQmlListProperty<PortInfo> *, PortInfo*);
    static void ClearPorts(QQmlListProperty<PortInfo> * );
    static PortInfo *PortAt(QQmlListProperty<PortInfo> *, int );
    static int PortCount(QQmlListProperty<PortInfo> *);

    std::vector<PortInfo*> m_ports;
    const LADSPA_Descriptor *m_d{};
public:
    Descriptor(QObject *pParent = nullptr);
    Descriptor(const LADSPA_Descriptor *, QObject *pParent = nullptr);
    quint64 uniqueID() const;
    QString label() const;
    QString name() const;
    QString maker() const;
    QString copyright() const;
    QQmlListProperty<PortInfo> ports();
    Q_INVOKABLE PortInfo *portAt(quint64 id) const;
    Q_INVOKABLE quint64  portCount() const;
    const LADSPA_Descriptor *raw() const;
    Q_INVOKABLE LadspaHandle *create(quint64 sampleRate, QObject *pParent = nullptr);
    LADSPA_Handle instantiate_fn(unsigned long sampleRate) const;
    void          connect_port_fn(LADSPA_Handle instance, unsigned long port, LADSPA::Data* data) const;
    void          activate_fn(LADSPA_Handle instance) const;
    void          deactivate_fn(LADSPA_Handle instance) const;
    void          cleanup_fn(LADSPA_Handle instance) const;
    void          run_fn(LADSPA_Handle instance, unsigned long count) const;
    void          run_adding_fn(LADSPA_Handle instance, unsigned long count) const;
    void          set_run_adding_gain_fn(LADSPA_Handle instance, LADSPA::Data gain) const;
};

QML_DECLARE_TYPE(Descriptor)

class PortInstance : public PortInfo {
    Q_OBJECT
    Q_PROPERTY(LadspaHandle *handle READ handle CONSTANT)
    Q_PROPERTY(LADSPA::Data* connection READ connection WRITE connect_port NOTIFY connectionChanged);
protected:
    LADSPA::Data    *m_connection{};
    LadspaHandle    *m_handle{};
    Descriptor      *m_descriptor{};
public:
    Q_INVOKABLE PortInstance(QObject *pParent = nullptr);
    PortInstance(PortInfo *_info, QObject *pParent = nullptr);
    virtual LadspaHandle* handle() const;
    Descriptor *descriptor() const override;
    LADSPA::Data *connection() const;
    Q_INVOKABLE void connect_port(LADSPA::Data *to);
signals:
    void connectionChanged(LADSPA::Data*);
};
QML_DECLARE_TYPE(PortInstance)
class LadspaHandle : public QObject {
    Q_OBJECT
    Q_PROPERTY(Descriptor* descriptor READ descriptor CONSTANT);
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(LADSPA::Data runAddingGain READ runAddingGain WRITE setRunAddingGain NOTIFY runAddingGainChanged);
    Q_PROPERTY(QQmlListProperty<PortInstance> ports READ ports)
    static void AppendPort(QQmlListProperty<PortInstance> *, PortInstance*);
    static void ClearPorts(QQmlListProperty<PortInstance> * );
    static PortInstance *PortAt(QQmlListProperty<PortInstance> *, int );
    static int PortCount(QQmlListProperty<PortInstance> *);

public:
    Q_INVOKABLE LadspaHandle(QObject *pParent=nullptr);
    Q_INVOKABLE LadspaHandle(Descriptor *_descriptor, LADSPA_Handle _handle, QObject *pParent = nullptr);
   ~LadspaHandle();
    LADSPA_Handle raw_handle() const;
    QQmlListProperty<PortInstance> ports();
    Q_INVOKABLE void connect_port(quint64 _index, LADSPA::Data*to);
    Q_INVOKABLE void activate();
    Q_INVOKABLE void deactivate();
    Q_INVOKABLE void run(size_t count);
    Q_INVOKABLE void runAdding(size_t count);
    Q_INVOKABLE void setRunAddingGain(LADSPA::Data _gain);
    Q_INVOKABLE LADSPA::Data runAddingGain() const;
    bool active() const;
    void setActive(bool);
    Descriptor *descriptor() const;
signals:
    void activeChanged(bool);
    void runAddingGainChanged(LADSPA::Data);
protected:
    Descriptor                *m_descriptor{};
    const LADSPA_Descriptor   *m_raw_desc{};
    LADSPA_Handle              m_handle{};
    std::vector<PortInstance*> m_ports{};
    LADSPA::Data               m_runAddingGain{1.f};
    bool                       m_active{};
};
QML_DECLARE_TYPE(LadspaHandle)
class LadspaLibrary : public QObject{
    Q_OBJECT;
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)
    Q_PROPERTY(quint64 pluginCount READ pluginCount NOTIFY pluginCountChanged)
    Q_PROPERTY(QQmlListProperty<Descriptor> plugins READ plugins NOTIFY pluginsChanged)
protected:
    std::vector<Descriptor*> m_plugins;
    QString m_errorString{};
    QString m_fileName{};
public:
    Q_INVOKABLE LadspaLibrary(QObject *pParent = nullptr);
    LadspaLibrary(QString fileName, QObject *pParent = nullptr);
    Q_INVOKABLE int open(QString filename);
    QQmlListProperty<Descriptor> plugins();
    QString errorString() const;
    QString fileName() const;
    void setFileName(QString );
    quint64 pluginCount() const;
    static int PluginCount(QQmlListProperty<Descriptor>* obj);
    static Descriptor *PluginAt(QQmlListProperty<Descriptor>* obj, int);
    static void AppendPlugin(QQmlListProperty<Descriptor>* obj, Descriptor*item);
    static void ClearPlugins(QQmlListProperty<Descriptor>*obj);
signals:
    void fileNameChanged(QString);
    void errorStringChanged(QString);
    void pluginsChanged();
    void pluginCountChanged(quint64);
};
QML_DECLARE_TYPE(LadspaLibrary)
