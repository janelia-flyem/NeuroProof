#include "StackQTController.h"
#include "StackQTUi.h"
#include "../Stack/StackPlaneController.h"

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
    // ?! maybe just create vtk in designer set properly and just reload window for different view
    // ?! how to reload vtk widget into scene -- create new widget or reset window??
    plane_controller = new StackPlaneController(stack_session,
            main_ui->ui.planeView);

    plane_controller->initialize();
    plane_controller->start();
}
