#include "StackQTController.h"
#include "StackQTUi.h"

using namespace NeuroProof;

StackQTController::StackQTController(StackSession* stack_session_) : 
    stack_session(stack_session_)
{
    main_ui = new StackQTUi;
    main_ui->showMaximized();
    main_ui->show();
}


