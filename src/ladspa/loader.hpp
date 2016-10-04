_Pragma("once")

#include "ladspa/port.hpp"
#include "ladspa/descriptor.hpp"
#include "ladspa/effect.hpp"

class LadspaLoader : public QObject {
    Q_OBJECT
    static LadspaDescriptor *PortAt(QQmlListProperty<LadspaDescriptor> *item, int id);
    static int DescriptortCount(QQmlListProperty<LadspaDescriptor> *item);
    QQmlListProperty<LadspaDescriptor> ports();
public:
    

};
