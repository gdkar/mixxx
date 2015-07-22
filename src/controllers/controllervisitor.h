_Pragma("once")
class Controller;
class ControllerVisitor {
  public:
    virtual void visit(Controller* ) {};
};
