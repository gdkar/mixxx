
#include <QMetaType>
#include <QMetaObject>
#include <QtQml>
#include <QtQuick>
#include <QtGlobal>

#include "ladspa/port.hpp"
#include "ladspa/descriptor.hpp"
#include "ladspa/effect.hpp"

void LadspaEffect::registerTypes()
{
    qmlRegisterUncreatableType<LadspaEffect>("org.mixxxx.qml.Ladspa", 0,1,"Effect","gotta use the factory. :/");
    qmlRegisterUncreatableType<LadspaDescriptor>("org.mixxxx.qml.Ladspa", 0,1,"Descriptor","just use the lib loader like everyone else");
    qmlRegisterUncreatableType<LadspaPort>("org.mixxxx.qml.Ladspa", 0,1,"Port","can't make those. sorry.");
    qmlRegisterUncreatableType<RangeHint>("org.mixxx.qml.Ladspa",0,1,"RangeHint","not a QObject. sry. T_T");
}

LadspaEffect::~LadspaEffect() { destroy();}
LadspaEffect::LadspaEffect(LadspaDescriptor *_desc, unsigned long sampleRate, QObject *pParent )
: QObject(pParent)
, m_descriptor(_desc)
, m_sample_rate(sampleRate)
{
    if(_desc) {
        create(sampleRate);
    }
}

int LadspaEffect::PortCount(QQmlListProperty<LadspaPort> *item)
{
    if(auto desc = qobject_cast<LadspaEffect*>(item->object)) {
        return int(desc->m_ports.size());
    }
    return -1;
}
QQmlListProperty<LadspaPort> LadspaEffect::ports()
{
    return QQmlListProperty<LadspaPort>(
        this,
        nullptr,
        nullptr,//&LadspaDescriptor::AppendPort,
        &LadspaEffect::PortCount,
        &LadspaEffect::PortAt,
        nullptr//&LadspaDescriptor::ClearPorts,
    );
}
void LadspaEffect::setSampleRate(unsigned long _rate)
{
    if(m_handle && sampleRate() == _rate)
        return;
    if(!m_handle) {
        m_sample_rate = _rate;
        return;
    }
    bool was_active = active();
    deactivate();
    m_sample_rate = _rate;
    if(auto cleanup = desc()->cleanup)
        cleanup(m_handle);
    m_handle = desc()->instantiate(desc(), m_sample_rate);
    if(!m_handle) {
            for(auto &port : m_ports) {
            port->deleteLater();
            port = nullptr;
        }
        m_ports.clear();
        return;
    }else{
        for(auto &port : m_ports) {
            port->setHandle(m_handle);
            if(auto conn = port->connection())
                port->connectPort(conn);
        }
    }
    if(was_active)
        activate();
}
void LadspaEffect::destroy()
{
    deactivate();
    if(m_handle && m_descriptor) {
        for(auto &port : m_ports) {
            port->deleteLater();
            port = nullptr;
        }
        m_ports.clear();
        if(auto cleanup = desc()->cleanup)
            cleanup(m_handle);
        m_handle = nullptr;
    }
    m_adding_gain = 1.f;
    m_sample_rate = 0;
}
bool LadspaEffect::active() const
{
    return m_active;
}
void LadspaEffect::setActive(bool x)
{
    if(x == active())
        return;
    if(x){
        activate();
    }else{
        deactivate();
    }
}
bool LadspaEffect::activate()
{
    if(active())
        return true;
    if(!std::all_of(m_ports.begin(),m_ports.end(),[](auto port){return !!port->connection();}))
        return false;
    if(auto activate_fn = desc()->activate) {
        activate_fn(m_handle);
    }
    return (m_active = true);
}
bool LadspaEffect::deactivate()
{
    if(!active())
        return true;
    if(m_handle && m_descriptor) {
        if(auto deactivate_fn = desc()->deactivate)
            deactivate_fn(m_handle);
    }
    return (m_active = false);
}
float LadspaEffect::addingGain() const { return m_adding_gain;}
void LadspaEffect::setAddingGain(float _gain)
{
    if(m_adding_gain == _gain)
        return;
    if(!m_handle) {
        m_adding_gain = _gain;
        return;
    }
    if(auto set_gain = desc()->set_run_adding_gain) {
        m_adding_gain = _gain;
        set_gain(m_handle, m_adding_gain);
    }
}
void LadspaEffect::run(unsigned long count)
{
    if(m_handle && m_descriptor) {
        if(!active() && !activate())
            return;
        if(auto run_fn = desc()->run) {
            run_fn(m_handle, count);
        }
    }
}
void LadspaEffect::run_adding(unsigned long count)
{
    if(m_handle && m_descriptor) {
        if(!active() && !activate())
            return;
        if(auto run_fn = desc()->run_adding) {
            run_fn(m_handle, count);
        }
    }
}
bool LadspaEffect::create(unsigned long _rate)
{
    if(m_handle && sampleRate() == _rate)
        return true;
    if(!m_descriptor)
        return false;
    if(m_handle)
        destroy();
    m_handle = desc()->instantiate(desc(), _rate);
    m_sample_rate = _rate;
    if(m_handle) {
        for(auto id = 0ul; id < desc()->PortCount; ++id) {
            m_ports.push_back(new LadspaPort(desc(), id, m_handle, this));
        }
        return true;
    }
    return false;
}

unsigned long LadspaEffect::sampleRate() const { return m_sample_rate;}

const LADSPA_Descriptor *LadspaEffect::desc() const { return m_descriptor->raw();}
LadspaDescriptor *LadspaEffect::prototype() const
{
    return m_descriptor;
}
LadspaPort *LadspaEffect::PortAt(QQmlListProperty<LadspaPort> *item, int id)
{
    if(auto desc = qobject_cast<LadspaEffect*>(item->object)) {
        if(id >= 0 && id <= int(desc->m_ports.size()))
            return desc->m_ports.at(id);
    }
    return nullptr;
}

