#ifndef CONTROLLERLEARNINGEVENTFILTER_H
#define CONTROLLERLEARNINGEVENTFILTER_H

#include <QObject>
#include <QEvent>

#include "controlobject.h"
#include "widget/controlwidgetconnection.h"

struct ControlInfo {
    ControlInfo()
            : clickControl(nullptr),
              emitOption(ControlParameterWidgetConnection::EMIT_ON_PRESS_AND_RELEASE),
              leftClickControl(nullptr),
              leftEmitOption(ControlParameterWidgetConnection::EMIT_ON_PRESS_AND_RELEASE),
              rightClickControl(nullptr),
              rightEmitOption(ControlParameterWidgetConnection::EMIT_ON_PRESS_AND_RELEASE) {
    }

    ControlObject* clickControl;
    ControlParameterWidgetConnection::EmitOption emitOption;
    ControlObject* leftClickControl;
    ControlParameterWidgetConnection::EmitOption leftEmitOption;
    ControlObject* rightClickControl;
    ControlParameterWidgetConnection::EmitOption rightEmitOption;
};

class ControllerLearningEventFilter : public QObject {
    Q_OBJECT
  public:
    ControllerLearningEventFilter(QObject* pParent = nullptr);
    virtual ~ControllerLearningEventFilter();
    virtual bool eventFilter(QObject* pObject, QEvent* pEvent);
    virtual void addWidgetClickInfo(QWidget* pWidget, Qt::MouseButton buttonState,
                            ControlObject* pControl,
                            ControlParameterWidgetConnection::EmitOption emitOption);
  public slots:
    virtual void startListening();
    virtual void stopListening();
  signals:
    void controlClicked(ControlObject* pControl);
  protected:
    QHash<QWidget*, ControlInfo> m_widgetControlInfo;
    bool m_bListening;
};


#endif /* CONTROLLERLEARNINGEVENTFILTER_H */
