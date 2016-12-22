#ifndef CONTROLLERPRESETVISITOR_H
#define CONTROLLERPRESETVISITOR_H

class MidiControllerPreset;
class HidControllerPreset;

class ControllerPresetVisitor {
  public:
    virtual ~ControllerPresetVisitor() = 0;
    virtual void visit(ControllerPreset* preset)
    {
        Q_UNUSED(preset);
    }
};
inline ControllerPresetVisitor::~ControllerPresetVisitor(){}
class ConstControllerPresetVisitor {
  public:
    virtual ~ConstControllerPresetVisitor() = 0;
    virtual void visit(const ControllerPreset* preset)
    {
        Q_UNUSED(preset);
    }
};
inline ConstControllerPresetVisitor::~ConstControllerPresetVisitor(){}

#endif /* CONTROLLERPRESETVISITOR_H */
