_Pragma("once")
#include <QObject>
#include "configobject.h"
class EngineMaster;
class ShoutcastManager : public QObject {
    Q_OBJECT
  public:
    ShoutcastManager(ConfigObject<ConfigValue>* pConfig, EngineMaster* pEngine);
    virtual ~ShoutcastManager();
    // Returns true if the Shoutcast connection is enabled. Note this only
    // indicates whether the connection is enabled, not whether it is connected.
    virtual bool isEnabled();
  public slots:
    // Set whether or not the Shoutcast connection is enabled.
    virtual void setEnabled(bool enabled);
  signals:
    virtual void shoutcastEnabled(bool);
  private:
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
};
