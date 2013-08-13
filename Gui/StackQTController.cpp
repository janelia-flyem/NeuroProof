#include "StackQTController.h"
#include "StackQTUi.h"
#include "../Stack/StackPlaneController.h"
#include "../Stack/StackBodyController.h"

using namespace NeuroProof;
using std::cout; using std::endl;

StackQTController::StackQTController(StackSession* stack_session_) : 
    stack_session(stack_session_)
{
    main_ui = new StackQTUi;

    if (stack_session) {
        load_views();
    }
    
    main_ui->showMaximized();
    main_ui->show();
}

void StackQTController::load_views()
{
    // ?! load views when main widget loads
    plane_controller = new StackPlaneController(stack_session, main_ui->ui.planeView);
    plane_controller->initialize();
    plane_controller->start();
    
    body_controller = new StackBodyController(stack_session, main_ui->ui.bodyView);
    body_controller->initialize();
    body_controller->start();
}
