#ifndef STACKQTCONTROLLER_H
#define STACKQTCONTROLLER_H

#include "../Stack/StackObserver.h"

namespace NeuroProof {


class StackSession;
class StackQTUi;
class StackPlaneController;

class StackQTController : public StackObserver {
  public:
    StackQTController(StackSession* stack_session_);
    
    void update() {}

  private:
    StackSession* stack_session;
    StackQTUi* main_ui;
    StackPlaneController* plane_controller;

    void load_views();
};

}

#endif
