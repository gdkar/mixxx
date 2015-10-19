_Pragma("once")
#include <QObject>
#include <QMetaType>
#include <QMetaEnum>
#include <QString>
#include <QScopedPointer>
#include <QByteArray>
#include <QtGlobal>
#include "configobject.h"
class ControlObject;
class WBaseWidget;
class ControlWidgetConnection : public QObject {
    Q_OBJECT
    Q_PROPERTY(double controlParameter READ getControlParameter WRITE setControlParameter NOTIFY controlParameterChanged);
    Q_PROPERTY(bool invert READ invert WRITE setInvert NOTIFY invertChanged);
  public:
    // Takes ownership of pControl and pTransformer.
    ControlWidgetConnection(WBaseWidget* pBaseWidget,ControlObject* pControl);
    virtual ~ControlWidgetConnection();
    double getControlParameter() const;
    double getControlParameterForValue(double value) const;
    ConfigKey getKey() const; 
    virtual QString toDebugString() const = 0;
    bool invert()const;
    void setInvert(bool);
  signals:
    void controlParameterChanged();
    void invertChanged(bool);
  protected slots:
    virtual void slotControlValueChanged(double v) = 0;
  protected:
    void setControlParameter(double v);
    WBaseWidget* m_pWidget;
    QScopedPointer<ControlObject> m_pControl;
  private:
    bool m_bInvert = false;
};
class ControlParameterWidgetConnection : public ControlWidgetConnection {
    Q_OBJECT
  public:
    enum class EmitOption {
        Never                = 0x00,
        OnPress              = 0x01,
        OnRelease            = 0x02,
        Default              = 0x04
    };
    Q_ENUM         (EmitOption);
    Q_DECLARE_FLAGS(EmitOptions,EmitOption);
    Q_PROPERTY(EmitOptions emitOption READ getEmitOption WRITE setEmitOption NOTIFY emitOptionChanged );
    Q_PROPERTY(DirectionOptions directionOption READ getDirectionOption WRITE setDirectionOption NOTIFY directionOptionChanged);
    static QString emitOptionToString(EmitOptions option);
    enum class DirectionOption {
        None                     = 0x00,
        FromWidget               = 0x01,
        ToWidget                 = 0x02,
        Default                  = 0x04
    };
    Q_ENUM         (DirectionOption);
    Q_DECLARE_FLAGS(DirectionOptions,DirectionOption);
    static QString directionOptionToString(DirectionOptions option);
    ControlParameterWidgetConnection(WBaseWidget* pBaseWidget,ControlObject* pControl, 
                                     DirectionOptions directionOption,EmitOptions emitOption);
    virtual ~ControlParameterWidgetConnection();
    void Init();
    QString toDebugString() const;
    virtual DirectionOptions  getDirectionOption() const;
    virtual EmitOptions       getEmitOption() const ;
    virtual void setDirectionOption(DirectionOptions v);
    virtual void setEmitOption(EmitOptions v);
    void resetControl();
    void setControlParameter(double v);
    void setControlParameterDown(double v);
    void setControlParameterUp(double v);
  signals:
    void emitOptionChanged(EmitOptions);
    void directionOptionChanged(DirectionOptions);
  private slots:
    virtual void slotControlValueChanged(double );
  private:
    DirectionOptions m_directionOption = DirectionOption::Default;
    EmitOptions      m_emitOption      = EmitOption::Default;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(ControlParameterWidgetConnection::EmitOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(ControlParameterWidgetConnection::DirectionOptions);
class ControlWidgetPropertyConnection : public ControlWidgetConnection {
    Q_OBJECT
  public:
    ControlWidgetPropertyConnection(WBaseWidget* pBaseWidget,ControlObject* pControl, const QString& property);
    virtual ~ControlWidgetPropertyConnection();
    QString toDebugString() const;
  private slots:
    virtual void slotControlValueChanged(double v);
  private:
    QByteArray m_propertyName;
};
