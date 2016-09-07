#include "controllers/controllervisitor.h"

void ControllerVisitor::visit(Controller *controller)
{
    void(sizeof(controller));
}
