/**
* @file controllerenumerator.h
* @author Sean Pappalardo spappalardo@mixxx.org
* @date Sat Apr 30 2011
* @brief Base class handling discovery and enumeration of DJ controllers.
*
* This class handles discovery and enumeration of DJ controllers and
*   must be inherited by a class that implements it on some API.
*/

_Pragma("once")
#include <atomic>
#include <memory>
#include "controllers/controller.h"
#include "controllers/controllermanager.h"

class ControllerEnumerator : public QObject {
    Q_OBJECT
  public:
    ControllerEnumerator();
    // In this function, the inheriting class must delete the Controllers it
    // creates
    virtual ~ControllerEnumerator();
    virtual QList<Controller*> queryDevices() = 0;
    // Sub-classes return true here if their devices must be polled to get data
    // from the controler.
    virtual bool needPolling();
};
struct EnumeratorFactory {
    std::atomic<EnumeratorFactory*> m_next{nullptr};
    EnumeratorFactory()
    {
        m_next.store(ControllerManager::m_factories.exchange(this));
    }
    virtual ~EnumeratorFactory() = default;
    EnumeratorFactory *next() const
    {
        return m_next.load();
    }
    virtual ControllerEnumerator* create() const = 0;
};
template<class T>
struct ThisFactory : public EnumeratorFactory {
    ThisFactory( )
    : EnumeratorFactory()
    { }
    virtual ControllerEnumerator *create() const override
    {
        return new T{};
    }
};
