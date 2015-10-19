_Pragma("once")
#include <QLabel>

#include "widget/wlabel.h"
class ControlObject;
class WKey : public WLabel  {
    Q_OBJECT
  public:
    WKey(const char* group, QWidget* pParent=NULL);
    virtual ~WKey();

    virtual void onConnectedControlChanged(double dParameter, double dValue);
    void setup(QDomNode node, const SkinContext& context);

  private slots:
    void setValue(double dValue);
    void preferencesUpdated(double dValue);
    void setCents();

  private:
    double m_dOldValue;
    bool m_displayCents;
    ControlObject* m_preferencesUpdated;
    ControlObject* m_engineKeyDistance;
};
