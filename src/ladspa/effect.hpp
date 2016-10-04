_Pragma("once")

#include "ladspa/descriptor.hpp"
#include "ladspa/port.hpp"

class LadspaEffect : public QObject {
    Q_OBJECT
    Q_PROPERTY(LadspaDescriptor* prototype READ prototype CONSTANT)
    Q_PROPERTY(float addingGain READ addingGain WRITE setAddingGain )
    Q_PROPERTY(bool active READ active WRITE setActive )
    Q_PROPERTY(unsigned long sampleRate READ sampleRate WRITE setSampleRate CONSTANT)
    Q_PROPERTY(QQmlListProperty<LadspaPort> ports READ ports CONSTANT)
protected:
    LadspaDescriptor *m_descriptor{};
    std::atomic<LADSPA_Handle> m_handle{};
    std::vector<LadspaPort*>   m_ports{};
    float                      m_adding_gain{1.f};
    int                        m_sample_rate{};
    bool                       m_active{false};

public:
    static void registerTypes();
    const LADSPA_Descriptor *desc() const;
    LadspaEffect(LadspaDescriptor *_desc, unsigned long sampleRate, QObject *pParent = nullptr);
   ~LadspaEffect();
    float addingGain() const;
    void setAddingGain(float _gain);
    Q_INVOKABLE void run(unsigned long count);
    Q_INVOKABLE void run_adding(unsigned long count);
    LadspaDescriptor *prototype() const;
    bool create(unsigned long _rate);
    unsigned long sampleRate() const;
    void setSampleRate(unsigned long _rate);
    void destroy();
    bool active() const;
    void setActive(bool x);
    bool activate();
    bool deactivate();
    static LadspaPort *PortAt(QQmlListProperty<LadspaPort> *item, int id);
    static int PortCount(QQmlListProperty<LadspaPort> *item);
    QQmlListProperty<LadspaPort> ports();
};

QML_DECLARE_TYPE(LadspaEffect)
