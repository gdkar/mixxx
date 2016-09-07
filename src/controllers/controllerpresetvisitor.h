#ifndef CONTROLLERPRESETVISITOR_H
#define CONTROLLERPRESETVISITOR_H

class ControllerPreset;

class ControllerPresetVisitor {
  public:
    virtual void visit(ControllerPreset* preset) { void(sizeof(preset));}
};

class ConstControllerPresetVisitor {
  public:
    virtual void visit(const ControllerPreset* preset) {void(sizeof(preset));}
};

#endif /* CONTROLLERPRESETVISITOR_H */
