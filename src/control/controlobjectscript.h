#ifndef CONTROLOBJECTSCRIPT_H
#define CONTROLOBJECTSCRIPT_H

#include "controllers/controllerengine.h"

#include "control/controlproxy.h"

// this is used for communicate with controller scripts
class ControlObjectScript : public ControlProxy {
    Q_OBJECT
    Q_PROPERTY(QJSValue callback READ callback WRITE setCallback NOTIFY callbackChanged        )
    Q_PROPERTY(QJSValue context  READ context  WRITE setContext NOTIFY contextChanged         )
  public:
    Q_INVOKABLE explicit ControlObjectScript(const ConfigKey& key, QObject* pParent = nullptr);
    Q_INVOKABLE ControlObjectScript(QString group, QString item, QObject *pParent = nullptr);
    Q_INVOKABLE explicit ControlObjectScript(QObject* pParent = nullptr);

    Q_INVOKABLE double modify(QJSValue func);
    Q_INVOKABLE double modify(QJSValue func, QJSValue _context);
    Q_INVOKABLE void connectControl(QJSValue cb, QJSValue ctx = QJSValue{});
    Q_INVOKABLE void disconnectControl();

    QJSValue callback() const;
    void setCallback(QJSValue);

    QJSValue context()  const;
    void setContext (QJSValue);

  signals:
    void callbackChanged(QJSValue);
    void contextChanged(QJSValue);
  protected slots:
   // Receives the value from the master control by a unique queued connection
    void onValueChanged(double v);
  private:
    QJSValue m_callback{};
    QJSValue m_context{};
};
#endif // CONTROLOBJECTSCRIPT_H
