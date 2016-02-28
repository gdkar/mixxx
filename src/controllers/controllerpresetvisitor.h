_Pragma("once")
class ControllerPreset;

class ControllerPresetVisitor {
  public:
    virtual void visit(ControllerPreset* preset) = 0;
};

class ConstControllerPresetVisitor {
  public:
    virtual void visit(const ControllerPreset* preset) = 0;
};
