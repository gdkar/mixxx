#include "ladspa/qmlladspa.hpp"

LADSPA::LADSPA(QObject *p)
:QObject(p){}
LADSPA::~LADSPA() = default;
void LADSPA::registerTypes()
{
    qRegisterMetaType<LADSPA::Data>("LADSPA_Data");
    qRegisterMetaType<LADSPA::Data*>("LADSPA_Data*");
    qRegisterMetaType<LADSPA::PortDescriptor>("LADSPA_PortDescriptor");
    qRegisterMetaType<LADSPA::LadspaProperty>("LADSPA_Property");
    qRegisterMetaType<LADSPA::Properties>("LADSPA_Properties");
    qRegisterMetaType<LADSPA::LadspaPort>("LADSPA_Port");
    qRegisterMetaType<LADSPA::Hint>("LADSPA_Hint");
    qRegisterMetaType<LADSPA::HintDefault>("LADSPA_HintDefault");
    qRegisterMetaType<LADSPA::PortRangeHintDescriptor>("LADSPA_PortRangeHintDescriptor");

    qmlRegisterType<LADSPA>("org.mixxx.qml", 0, 1, "LADSPA");
    qmlRegisterType<PortRangeHint>("org.mixxx.qml", 0,1,"PortRangeHint");
    qmlRegisterType<Descriptor>("org.mixxx.qml", 0,1,"Descriptor");
    qmlRegisterType<PortInfo>("org.mixxx.qml",  0,1,"PortInfo");
    qmlRegisterType<PortInstance>("org.mixxx.qml", 0,1,"PortInstance");
    qmlRegisterType<LadspaHandle>("org.mixxx.qml", 0,1,"LadspaHandle");
    qmlRegisterType<LadspaLibrary>("org.mixxx.qml", 0,1,"LadspaLibrary");
}
PortRangeHint::PortRangeHint(QObject *pParent):QObject(pParent), m_hintDescriptor{LADSPA::PortRangeHintDescriptor(LADSPA_HINT_DEFAULT_NONE)}  {}
PortRangeHint::PortRangeHint(const LADSPA_PortRangeHint *_d, QObject *pParent)
: QObject(pParent)
, m_hintDescriptor{_d->HintDescriptor}
, m_upperBound{_d->UpperBound}
, m_lowerBound{_d->LowerBound}
{}
PortRangeHint::~PortRangeHint() = default;
LADSPA::PortRangeHintDescriptor PortRangeHint::hintDescriptor() const { return m_hintDescriptor;}
void PortRangeHint::setHintDescriptor(LADSPA::PortRangeHintDescriptor hint)
{
    if(hintDescriptor() != hint) {
        auto prevDefault = defaultValue();
        emit hintDescriptorChanged(m_hintDescriptor = hint);
        if(prevDefault != defaultValue())
            emit defaultValueChanged(defaultValue());
    }
}
LADSPA::Data PortRangeHint::lowerBound() const
{
    if(hintDescriptor() & LADSPA::Hint::BoundedBelow) {
        if(hintDescriptor() & LADSPA::Hint::SampleRate)
            return m_lowerBound * m_sampleRate;
        else
            return m_lowerBound;
    } else {
        return -std::numeric_limits<LADSPA::Data>::infinity();
    }
}
void PortRangeHint::setSampleRate(quint64 rate)
{
    if(sampleRate() != rate) {
        auto prevDefault = defaultValue();
        emit sampleRateChanged(m_sampleRate = rate);
        if(prevDefault != defaultValue())
            emit defaultValueChanged(defaultValue());
    }
}
quint64 PortRangeHint::sampleRate() const
{
    return m_sampleRate;
}
void PortRangeHint::setLowerBound(LADSPA::Data bound)
{
    if(lowerBound() != bound) {
        auto prevDefault = defaultValue();
        emit lowerBoundChanged(m_lowerBound = bound);
        if(prevDefault != defaultValue())
            emit defaultValueChanged(defaultValue());
    }
}
LADSPA::Data PortRangeHint::upperBound() const
{
    if(hintDescriptor() & LADSPA::Hint::BoundedAbove) {
        if(hintDescriptor() & LADSPA::Hint::SampleRate)
            return m_upperBound * m_sampleRate;
        else
            return m_upperBound;
    }else {
        return std::numeric_limits<LADSPA::Data>::infinity();
    }
}
void PortRangeHint::setUpperBound(LADSPA::Data bound)
{
    if(upperBound() != bound) {
        auto prevDefault = defaultValue();
        emit upperBoundChanged(m_upperBound = bound);
        if(prevDefault != defaultValue())
            emit defaultValueChanged(defaultValue());
    }
}
LADSPA::HintDefault PortRangeHint::hintDefault() const
{
    return LADSPA::HintDefault(LADSPA_PortRangeHintDescriptor(hintDescriptor()) & LADSPA_HINT_DEFAULT_MASK);
}
void PortRangeHint::setHintDefault(LADSPA::HintDefault hint)
{
    auto prev = LADSPA_PortRangeHintDescriptor(hintDescriptor()) & ~LADSPA_HINT_DEFAULT_MASK;
    auto replacement = prev | (LADSPA_PortRangeHintDescriptor(hint)&LADSPA_HINT_DEFAULT_MASK);
    if(LADSPA::PortRangeHintDescriptor(replacement) != hintDescriptor()) {
        m_hintDescriptor =  LADSPA::PortRangeHintDescriptor(replacement);
        emit hintDescriptorChanged(hintDescriptor());
        emit hintDefaultChanged(hintDefault());
        emit defaultValueChanged(defaultValue());
    }
}
LADSPA::Data PortRangeHint::defaultValue() const
{
    auto && lerp = [](float x, float y, float m) { return ( y * m) + x * (1.f - m);};
    auto && mixture = [&](float prop)
    {
        return hintDescriptor() & LADSPA::Hint::Logarithmic
          ? std::exp(lerp(std::log(lowerBound()),std::log(upperBound()),prop))
          : lerp(lowerBound(),upperBound(),prop);
    };
    auto baseValue = ([&](){
        switch (hintDefault()) {
            case LADSPA::HintDefault::None: return 0.f;
            case LADSPA::HintDefault::Minimum: return lowerBound();
            case LADSPA::HintDefault::Low: return mixture(0.25f);
            case LADSPA::HintDefault::Middle: return mixture(0.5f);
            case LADSPA::HintDefault::High: return mixture(0.75f);
            case LADSPA::HintDefault::Maximum: return upperBound();
            case LADSPA::HintDefault::Zero: return 0.f;
            case LADSPA::HintDefault::One: return 1.f;
            case LADSPA::HintDefault::OneHundred: return 100.f;
            case LADSPA::HintDefault::FourForty: return 100.f;
            default: return 0.f;
        }
    })();
    if(hintDescriptor() & LADSPA::Hint::SampleRate) {
        baseValue *= m_sampleRate;
    }
    return baseValue;
}
Descriptor *PortInfo::descriptor() const { return qobject_cast<Descriptor*>(parent());}
PortInfo::PortInfo(QObject *pParent)
: QObject(pParent){}
PortInfo::PortInfo(const LADSPA_Descriptor *_d, quint64 _index, QObject *pParent)
: QObject(pParent)
, m_index(_index)
, m_desc(_d->PortDescriptors[_index])
, m_name(_d->PortNames[_index])
, m_rangeHint(new PortRangeHint(&_d->PortRangeHints[_index]))
{}

int Descriptor::PortCount(QQmlListProperty<PortInfo> *list)
{
    if(auto obj = qobject_cast<Descriptor*>(list->object))
        return obj->m_ports.size();
    else
        return 0;
}
PortInfo *Descriptor::portAt(quint64 id) const
{
    if(id < m_ports.size())
        return m_ports.at(id);
    else
        return nullptr;
}
quint64 Descriptor::portCount() const
{
    return m_ports.size();
}
PortInfo *Descriptor::PortAt(QQmlListProperty<PortInfo> *list, int idx)
{
    if(auto obj = qobject_cast<Descriptor*>(list->object)) {
        if(idx >= 0 && idx < obj->m_ports.size())
            return obj->m_ports.at(idx);
    }
    return nullptr;
}
void Descriptor::ClearPorts(QQmlListProperty<PortInfo> *list)
{
    if(auto obj = qobject_cast<Descriptor*>(list->object)){
        for(auto port : obj->m_ports)
            port->deleteLater();
        obj->m_ports.clear();
    }
}
void Descriptor::AppendPort(QQmlListProperty<PortInfo> *list, PortInfo *port)
{
    if(auto obj = qobject_cast<Descriptor*>(list->object)){
        port->setParent(obj);
        obj->m_ports.push_back(port);
    }
}
Descriptor::Descriptor(QObject *pParent)
: QObject(pParent){}
Descriptor::Descriptor(const LADSPA_Descriptor*_d, QObject *pParent)
: QObject(pParent)
, m_d(_d)
{
    for(auto i = 0ul; i < _d->PortCount; ++i) {
        m_ports.push_back(new PortInfo(_d, i, this));
    }
}
quint64 Descriptor::uniqueID() const
{
    return m_d->UniqueID;
}
QString Descriptor::label() const
{
    return {m_d->Label};
}
QString Descriptor::name() const
{
    return {m_d->Name};
}
QString Descriptor::maker() const
{
    return {m_d->Maker};
}
QString Descriptor::copyright() const
{
    return {m_d->Copyright};
}
QQmlListProperty<PortInfo> Descriptor::ports()
{
    return QQmlListProperty<PortInfo>(
        this
      , nullptr
      ,&Descriptor::AppendPort
      ,&Descriptor::PortCount
      ,&Descriptor::PortAt
      ,&Descriptor::ClearPorts);
}

LadspaHandle::LadspaHandle(QObject *p)
: QObject(p) {}

LadspaHandle::LadspaHandle(Descriptor *_descriptor, LADSPA_Handle _handle, QObject *p)
: QObject(p)
, m_descriptor(_descriptor)
, m_handle(_handle)
{
    for(auto i = 0ul; i < m_descriptor->portCount(); ++i) {
        m_ports.push_back(new PortInstance(m_descriptor->portAt(i), this));
    }
}
LadspaHandle *PortInstance::handle() const
{
    if(!m_handle) {
        return qobject_cast<LadspaHandle*>(parent());
    }
    return m_handle;
}
Descriptor *PortInstance::descriptor() const
{
    if(!m_descriptor){
        if(auto h = handle()) {
            return h->descriptor();
        }
    }
    return m_descriptor;
}
const LADSPA_Descriptor *Descriptor::raw() const
{
    return m_d;
}
LADSPA_Handle Descriptor::instantiate_fn(unsigned long sampleRate) const
{
    return m_d->instantiate(m_d, sampleRate);
}
void Descriptor::connect_port_fn(LADSPA_Handle instance, unsigned long port, LADSPA::Data *data) const
{
    m_d->connect_port(instance,port,data);
}
void Descriptor::activate_fn(LADSPA_Handle instance) const
{
    if(m_d->activate)
        m_d->activate(instance);
}
void Descriptor::deactivate_fn(LADSPA_Handle instance) const
{
    if(m_d->deactivate)
        m_d->deactivate(instance);
}
void Descriptor::cleanup_fn(LADSPA_Handle instance) const
{
    if(m_d->cleanup)
        m_d->cleanup(instance);
}
void Descriptor::run_fn(LADSPA_Handle instance,unsigned long count) const
{
    if(m_d->run)
        m_d->run(instance, count);
}
void Descriptor::run_adding_fn(LADSPA_Handle instance,unsigned long count) const
{
    if(m_d->run_adding)
        m_d->run_adding(instance, count);
}
void Descriptor::set_run_adding_gain_fn(LADSPA_Handle instance,LADSPA::Data gain) const
{
    if(m_d->set_run_adding_gain)
        m_d->set_run_adding_gain(instance, gain);
}
LadspaHandle *Descriptor::create(quint64 sampleRate, QObject *pParent)
{
    auto inst = instantiate_fn(sampleRate);
    if(inst) {
        return new LadspaHandle(this, inst, pParent);
    }
    return nullptr;
}
PortInstance::PortInstance(QObject *pParent)
: PortInfo(pParent){}
PortInstance::PortInstance(PortInfo *_info, QObject *p)
: PortInfo(p)
, m_handle(qobject_cast<LadspaHandle*>(p))
, m_descriptor(m_handle ? m_handle->descriptor() : nullptr)
{
    m_index = _info->index();
    m_rangeHint = new PortRangeHint(this);
    m_rangeHint->setHintDescriptor(_info->rangeHint()->hintDescriptor());
    m_rangeHint->setLowerBound(_info->rangeHint()->lowerBound());
    m_rangeHint->setUpperBound(_info->rangeHint()->upperBound());
    m_name = _info->name();
    m_desc = _info->portDescriptor();
}
LADSPA_Handle  LadspaHandle::raw_handle() const
{
    return m_handle;
}
void LadspaHandle::connect_port(quint64 _index, LADSPA::Data *to)
{
    if(_index < m_ports.size()) {
        m_ports.at(_index)->connect_port(to);
    }
}
LADSPA::Data *PortInstance::connection() const
{
    return m_connection;
}
void PortInstance::connect_port(LADSPA::Data *to)
{
    if(m_handle && m_descriptor) {
        m_descriptor->connect_port_fn(m_handle->raw_handle(),index(),to);
        if(m_connection != to)
            emit connectionChanged(m_connection = to);
    }
}
bool LadspaHandle::active() const
{
    return m_active;
}
void LadspaHandle::setActive(bool _active)
{
    if(m_active && !_active)
        deactivate();
    else if(!m_active && _active)
        activate();
}
void LadspaHandle::runAdding(size_t count)
{
    if(m_active){
        m_descriptor->run_adding_fn(raw_handle(),count);
    }
}
void LadspaHandle::run(size_t count)
{
    if(m_active){
        m_descriptor->run_fn(raw_handle(),count);
    }
}
LADSPA::Data LadspaHandle::runAddingGain() const
{
    return m_runAddingGain;
}
void LadspaHandle::setRunAddingGain(LADSPA::Data gain)
{
    if(gain != m_runAddingGain){
        m_descriptor->set_run_adding_gain_fn(raw_handle(),gain);
        emit runAddingGainChanged(m_runAddingGain = gain);
    }
}
void LadspaHandle::activate()
{
    if(!m_active){
        m_descriptor->activate_fn(raw_handle());
        emit activeChanged(m_active = true);
    }
}
Descriptor *LadspaHandle::descriptor() const
{
    return m_descriptor;
}
void LadspaHandle::deactivate()
{
    if(m_active){
        m_descriptor->deactivate_fn(raw_handle());
        emit activeChanged(m_active = false);
    }
}
LadspaHandle::~LadspaHandle()
{
    deactivate();
    m_descriptor->cleanup_fn(m_handle);
    m_handle = nullptr;
}

LadspaLibrary::LadspaLibrary(QObject *p)
: QObject(p) {}

LadspaLibrary::LadspaLibrary(QString name, QObject *p)
: QObject( p)
{
    open(name);
}
void LadspaLibrary::setFileName(QString name)
{
    open(name);
}
int LadspaLibrary::open(QString name)
{
    if(name == m_fileName)
        return 0;
    QLibrary lib(name,this);
    if(auto descriptor_fn = (LADSPA_Descriptor_Function)lib.resolve(name, "ladspa_descriptor")) {
        emit(errorStringChanged(m_errorString = QString{}));
        m_fileName = lib.fileName();
        emit(fileNameChanged(m_fileName));
        auto id = quint64{0};
        auto originalCount = m_plugins.size();
        for(;;++id){
            auto desc = descriptor_fn(id);
            if(desc) {
                auto Desc = new Descriptor(desc, this);
                m_plugins.push_back(Desc);
            }else{
                break;
            }
        }
        if(m_plugins.size() != originalCount){
            emit pluginCountChanged(m_plugins.size());
            emit pluginsChanged();
        }
        return id;
    }else{
        m_errorString = lib.errorString();
        qDebug() << m_errorString;
        emit(errorStringChanged(m_errorString));
    }
    return -1;
}
int LadspaLibrary::PluginCount(QQmlListProperty<Descriptor> *obj)
{
    if(auto lib = qobject_cast<LadspaLibrary *>(obj->object))
        return lib->m_plugins.size();
    else
        return 0;
}
void LadspaLibrary::ClearPlugins(QQmlListProperty<Descriptor> *obj)
{
    if(auto lib = qobject_cast<LadspaLibrary *>(obj->object))
        lib->m_plugins.clear();
}
void LadspaLibrary::AppendPlugin(QQmlListProperty<Descriptor> *obj, Descriptor* item)
{
    if(auto lib = qobject_cast<LadspaLibrary *>(obj->object))
        lib->m_plugins.push_back(item);
}
Descriptor *LadspaLibrary::PluginAt(QQmlListProperty<Descriptor> *obj, int idx)
{
    if(auto lib = qobject_cast<LadspaLibrary *>(obj->object)) {
        if(idx >= 0 && size_t(idx) < lib->m_plugins.size())
            return lib->m_plugins.at(idx);
    }
    return nullptr;
}
QQmlListProperty<Descriptor> LadspaLibrary::plugins()
{
    return QQmlListProperty<Descriptor>(
        this
      , nullptr
      ,&LadspaLibrary::AppendPlugin
      ,&LadspaLibrary::PluginCount
      ,&LadspaLibrary::PluginAt
      ,&LadspaLibrary::ClearPlugins);
}
quint64 LadspaLibrary::pluginCount() const
{
    return m_plugins.size();
}
QString LadspaLibrary::fileName() const
{
    return m_fileName;
}
QString LadspaLibrary::errorString() const
{
    return m_errorString;
}

PortInstance*LadspaHandle::PortAt(QQmlListProperty<PortInstance> *list, int idx)
{
    if(auto obj = qobject_cast<LadspaHandle*>(list->object)) {
        if(idx >= 0 && idx < obj->m_ports.size())
            return obj->m_ports.at(idx);
    }
    return nullptr;
}
void LadspaHandle::ClearPorts(QQmlListProperty<PortInstance> *list)
{
    if(auto obj = qobject_cast<LadspaHandle*>(list->object)){
        for(auto port : obj->m_ports)
            port->deleteLater();
        obj->m_ports.clear();
    }
}
void LadspaHandle::AppendPort(QQmlListProperty<PortInstance > *list, PortInstance *port)
{
    if(auto obj = qobject_cast<LadspaHandle*>(list->object)){
        port->setParent(obj);
        obj->m_ports.push_back(port);
    }
}
int LadspaHandle::PortCount(QQmlListProperty<PortInstance> *list)
{
    if(auto obj = qobject_cast<LadspaHandle*>(list->object))
        return obj->m_ports.size();
    else
        return 0;
}
QQmlListProperty<PortInstance> LadspaHandle::ports()
{
    return QQmlListProperty<PortInstance>(
        this
      , nullptr
      ,&LadspaHandle::AppendPort
      ,&LadspaHandle::PortCount
      ,&LadspaHandle::PortAt
      ,&LadspaHandle::ClearPorts);
}

