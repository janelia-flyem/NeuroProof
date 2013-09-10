#include "StackBodyController.h"
#include "StackSession.h"
#include "StackBodyView.h"
#include <QtGui/QWidget>

using namespace NeuroProof;

StackBodyController::StackBodyController(StackSession* stack_session_,
        QWidget* widget_parent) : stack_session(stack_session_), view(0)
{
    stack_session->attach_observer(this);
    Stack* stack = stack_session->get_stack();
    if (!stack) {
        throw ErrMsg("Controller started with no stack");
    }
    stack->attach_observer(this);

    view = new StackBodyView(stack_session, widget_parent);
}

void StackBodyController::initialize()
{
    view->initialize();
}

void StackBodyController::start()
{
    view->start();
}

StackBodyController::~StackBodyController()
{
    stack_session->detach_observer(this);
    Stack* stack = stack_session->get_stack();
    stack->detach_observer(this);
    delete view;
}

