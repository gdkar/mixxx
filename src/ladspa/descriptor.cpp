
#include <QMetaType>
#include <QMetaObject>
#include <QtQml>
#include <QtQuick>
#include <QtGlobal>

#include "ladspa/port.hpp"
#include "ladspa/descriptor.hpp"
#include "ladspa/effect.hpp"


LadspaDescriptor::~LadspaDescriptor() = default;

LadspaDescriptor::LadspaDescriptor(LADSPA_Descriptor *_desc, QObject *p)
: QObject(p)
, m_descriptor(_desc)
{
    if(_desc) {
        setObjectName(label());
        for(auto id = 0ul; id < m_descriptor->PortCount; ++id) {
            m_ports.push_back(new LadspaPort(m_descriptor,id, nullptr, this));
        }
    }
}
LadspaPort* LadspaDescriptor::PortAt(QQmlListProperty<LadspaPort> *item, int id)
{
    if(auto desc = qobject_cast<LadspaDescriptor*>(item->object)) {
        if(id >= 0 && id <= int(desc->m_ports.size()))
            return desc->m_ports.at(id);
    }
    return nullptr;
}
int LadspaDescriptor::PortCount(QQmlListProperty<LadspaPort> *item)
{
    if(auto desc = qobject_cast<LadspaDescriptor*>(item->object)) {
        return int(desc->m_ports.size());
    }
    return -1;
}
QQmlListProperty<LadspaPort> LadspaDescriptor::ports()
{
    return QQmlListProperty<LadspaPort>(
        this,
        nullptr,
        nullptr,//&LadspaDescriptor::AppendPort,
        &LadspaDescriptor::PortCount,
        &LadspaDescriptor::PortAt,
        nullptr //&LadspaDescriptor::ClearPorts,
    );
}

