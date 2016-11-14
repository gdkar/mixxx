#ifndef CONTROLLERVISITOR_H
#define CONTROLLERVISITOR_H

class Controller;

class ControllerVisitor {
  public:
    virtual void visit(Controller *controller) {
        Q_UNUSED(controller);
//        qWarning() << "Attempted to load an unsupported " << controller->metaObject()->className();
    }
};

#endif /* CONTROLLERVISITOR_H */
