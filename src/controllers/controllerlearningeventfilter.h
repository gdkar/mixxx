#ifndef CONTROLLERLEARNINGEVENTFILTER_H
#define CONTROLLERLEARNINGEVENTFILTER_H

#include <QObject>
#include <QEvent>

#include "control/controlobject.h"
#include "widget/controlwidgetconnection.h"

struct ControlInfo {
    using EmitOption = ControlParameterWidgetConnection::EmitOption;
    ControlInfo()
            : clickControl(nullptr),
              emitOption(ControlParameterWidgetConnection::EMIT_ON_PRESS_AND_RELEASE),
              leftClickControl(nullptr),
              leftEmitOption(ControlParameterWidgetConnection::EMIT_ON_PRESS_AND_RELEASE),
              rightClickControl(nullptr),
              rightEmitOption(ControlParameterWidgetConnection::EMIT_ON_PRESS_AND_RELEASE) {
    }

    ControlObject* clickControl{};
    EmitOption emitOption;
    ControlObject* leftClickControl{};
    EmitOption leftEmitOption;
    ControlObject* rightClickControl{};
    EmitOption rightEmitOption;
};

class ControllerLearningEventFilter : public QObject {
    Q_OBJECT
  public:
    ControllerLearningEventFilter(QObject* pParent = nullptr);
    virtual ~ControllerLearningEventFilter();
    virtual bool eventFilter(QObject* pObject, QEvent* pEvent);
    void addWidgetClickInfo(QWidget* pWidget, Qt::MouseButton buttonState,
                            ControlObject* pControl,
                            ControlParameterWidgetConnection::EmitOption emitOption);
  public slots:
    void startListening();
    void stopListening();
  signals:
    void controlClicked(ControlObject* pControl);
  private:
    QHash<QWidget*, ControlInfo> m_widgetControlInfo;
    bool m_bListening;
};


#endif /* CONTROLLERLEARNINGEVENTFILTER_H */
