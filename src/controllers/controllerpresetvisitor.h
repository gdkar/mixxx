_Pragma("once")
class ControllerPreset;
class ControllerPresetVisitor {
  public:
    virtual void visit(ControllerPreset* ){};
};
class ConstControllerPresetVisitor {
  public:
    virtual void visit(const ControllerPreset* ){};
};
