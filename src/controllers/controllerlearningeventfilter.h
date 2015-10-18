_Pragma("once")
#include <QObject>
#include <QEvent>

#include "widget/controlwidgetconnection.h"
class ControlObject;
struct ControlInfo {
    ControlInfo() = default;
    ControlObject* clickControl     = nullptr;
    ControlParameterWidgetConnection::EmitOptions emitOption
      = ControlParameterWidgetConnection::EmitOption::OnPress
       |ControlParameterWidgetConnection::EmitOption::OnRelease;
    ControlObject* leftClickControl = nullptr;
    ControlParameterWidgetConnection::EmitOptions leftEmitOption
      = ControlParameterWidgetConnection::EmitOption::OnPress
       |ControlParameterWidgetConnection::EmitOption::OnRelease;
    ControlObject* rightClickControl= nullptr;
    ControlParameterWidgetConnection::EmitOptions rightEmitOption
      = ControlParameterWidgetConnection::EmitOption::OnPress
       |ControlParameterWidgetConnection::EmitOption::OnRelease;
};
class ControllerLearningEventFilter : public QObject {
    Q_OBJECT
  public:
    ControllerLearningEventFilter(QObject* pParent = nullptr);
    virtual ~ControllerLearningEventFilter();
    virtual bool eventFilter(QObject* pObject, QEvent* pEvent);
    void addWidgetClickInfo(QWidget* pWidget, Qt::MouseButton buttonState,
                            ControlObject* pControl,
                            ControlParameterWidgetConnection::EmitOptions emitOption);
  public slots:
    void startListening();
    void stopListening();
  signals:
    void controlClicked(ControlObject* pControl);
  private:
    QHash<QWidget*, ControlInfo> m_widgetControlInfo;
    bool m_bListening = false;
};
