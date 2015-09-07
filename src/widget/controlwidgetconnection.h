#ifndef CONTROLWIDGETCONNECTION_H
#define CONTROLWIDGETCONNECTION_H

#include <QObject>
#include <QMetaType>
#include <QMetaEnum>
#include <QString>
#include <QScopedPointer>
#include <QByteArray>
#include <QtGlobal>
#include "configobject.h"
class ControlObjectSlave;
class WBaseWidget;
class ValueTransformer;

class ControlWidgetConnection : public QObject {
    Q_OBJECT
  public:
    // Takes ownership of pControl and pTransformer.
    ControlWidgetConnection(WBaseWidget* pBaseWidget,
                            ControlObjectSlave* pControl,
                            ValueTransformer* pTransformer);
    virtual ~ControlWidgetConnection();
    double getControlParameter() const;
    double getControlParameterForValue(double value) const;
    const ConfigKey& getKey() const; 
    virtual QString toDebugString() const = 0;
  protected slots:
    virtual void slotControlValueChanged(double v) = 0;
  protected:
    void setControlParameter(double v);
    WBaseWidget* m_pWidget;
    QScopedPointer<ControlObjectSlave> m_pControl;
  private:
    QScopedPointer<ValueTransformer> m_pValueTransformer;
};
class ControlParameterWidgetConnection : public ControlWidgetConnection {
    Q_OBJECT
  public:
    enum EmitOption {
        EMIT_NEVER                = 0x00,
        EMIT_ON_PRESS             = 0x01,
        EMIT_ON_RELEASE           = 0x02,
        EMIT_ON_PRESS_AND_RELEASE = 0x03,
        EMIT_DEFAULT              = 0x04
    };
    static QString emitOptionToString(EmitOption option);
    enum DirectionOption {
        DIR_NON                  = 0x00,
        DIR_FROM_WIDGET          = 0x01,
        DIR_TO_WIDGET            = 0x02,
        DIR_FROM_AND_TO_WIDGET   = 0x03,
        DIR_DEFAULT              = 0x04
    };
    Q_ENUMS(EmitOption);
    Q_ENUMS(DirectionOption);
    static QString directionOptionToString(DirectionOption option);
    ControlParameterWidgetConnection(WBaseWidget* pBaseWidget,
                                     ControlObjectSlave* pControl,
                                     ValueTransformer* pTransformer,
                                     DirectionOption directionOption,
                                     EmitOption emitOption);
    virtual ~ControlParameterWidgetConnection();
    void Init();
    QString toDebugString() const;
    virtual int getDirectionOption() const;
    virtual int getEmitOption() const { return m_emitOption; };
    virtual void setDirectionOption(enum DirectionOption v);
    virtual void setEmitOption(enum EmitOption v);
    void resetControl();
    void setControlParameter(double v);
    void setControlParameterDown(double v);
    void setControlParameterUp(double v);
  private slots:
    virtual void slotControlValueChanged(double v);
  private:
    DirectionOption m_directionOption;
    EmitOption m_emitOption;
};
class ControlWidgetPropertyConnection : public ControlWidgetConnection {
    Q_OBJECT
  public:
    ControlWidgetPropertyConnection(WBaseWidget* pBaseWidget,
                                    ControlObjectSlave* pControl,
                                    ValueTransformer* pTransformer,
                                    const QString& property);
    virtual ~ControlWidgetPropertyConnection();
    QString toDebugString() const;
  private slots:
    virtual void slotControlValueChanged(double v);
  private:
    QByteArray m_propertyName;
};
#endif /* CONTROLWIDGETCONNECTION_H */
