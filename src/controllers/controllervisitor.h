#ifndef CONTROLLERVISITOR_H
#define CONTROLLERVISITOR_H

class Controller;

class ControllerVisitor {
  public:
    virtual void visit(Controller* controller);
};

#endif /* CONTROLLERVISITOR_H */
