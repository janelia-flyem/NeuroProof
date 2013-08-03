#include "StackBodyController.h"
#include "StackSession.h"
#include "StackBodyView.h"

using namespace NeuroProof;

StackBodyController::StackBodyController(StackSession* stack_session_) :
    stack_session(stack_session_), view(0)
{
    stack_session->attach_observer(this);
    Stack* stack = stack_session->get_stack();
    if (!stack) {
        throw ErrMsg("Controller started with no stack");
    }
    stack->attach_observer(this);

    view = new StackBodyView(stack_session);
}

void StackBodyController::initialize()
{
    stack_session->initialize();
    view->initialize();
}

void StackBodyController::start()
{
    view->start();
}

