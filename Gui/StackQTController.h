#ifndef STACKQTCONTROLLER_H
#define STACKQTCONTROLLER_H

#include "../Stack/StackObserver.h"
#include <QWidget>

namespace NeuroProof {


class StackSession;
class StackQTUi;
class StackPlaneController;
class StackBodyController;

class StackQTController : public QObject, public StackObserver {
    Q_OBJECT
  
  public:
    StackQTController(StackSession* stack_session_);

    void update() {}

  private:
    StackSession* stack_session;
    StackQTUi* main_ui;
    StackPlaneController* plane_controller;
    StackBodyController* body_controller;

  private slots: 
    void load_views();
};

}

#endif
