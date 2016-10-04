_Pragma("once")
#include <QtQml>
#include <QObject>
#include <QMetaObject>
#include <QMetaType>

#include <initializer_list>
#include "ladspa/port.hpp"

class LadspaDescriptor : public QObject {
    Q_OBJECT
    Q_PROPERTY(unsigned long uid READ uid CONSTANT)
    Q_PROPERTY(bool realtime READ realtime CONSTANT)
    Q_PROPERTY(bool inplaceBroken READ inplaceBroken CONSTANT)
    Q_PROPERTY(bool hardRTCapable READ hardRTCapable CONSTANT)
    Q_PROPERTY(QString label READ label CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString maker READ maker CONSTANT)
    Q_PROPERTY(QString copyright READ copyright CONSTANT)
    Q_PROPERTY(QQmlListProperty<LadspaPort> ports READ ports CONSTANT)

protected:
    std::vector<LadspaPort*> m_ports;
    LADSPA_Descriptor       *m_descriptor{};
public:
    LadspaDescriptor(LADSPA_Descriptor *_desc, QObject *p = nullptr);
   ~LadspaDescriptor();
    const LADSPA_Descriptor *raw() const { return m_descriptor;}

    unsigned long uid() const { return m_descriptor ? m_descriptor->UniqueID : 0;}
    bool realtime() const { return m_descriptor ? LADSPA_IS_REALTIME(m_descriptor->Properties) : false;}
    bool inplaceBroken() const { return m_descriptor ? LADSPA_IS_INPLACE_BROKEN(m_descriptor->Properties) : false;}
    bool hardRTCapable() const { return m_descriptor ? LADSPA_IS_HARD_RT_CAPABLE(m_descriptor->Properties) : false;}
    QString label() const { return m_descriptor ? QString{ m_descriptor->Label} : "";}
    QString name() const { return m_descriptor ? QString{ m_descriptor->Name} : "";}
    QString maker() const { return m_descriptor ? QString{ m_descriptor->Maker} : "";}
    QString copyright() const { return m_descriptor ? QString{ m_descriptor->Copyright} : "";}
    static LadspaPort *PortAt(QQmlListProperty<LadspaPort> *item, int id);
    static int PortCount(QQmlListProperty<LadspaPort> *item);
    QQmlListProperty<LadspaPort> ports();
};

QML_DECLARE_TYPE(LadspaDescriptor)
