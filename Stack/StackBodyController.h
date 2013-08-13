#ifndef STACKBODYCONTROLLER_H
#define STACKBODYCONTROLLER_H

#include "StackObserver.h"

class QWidget;

namespace NeuroProof {

class StackSession;
class StackBodyView;

class StackBodyController : public StackObserver {
  public:
    StackBodyController(StackSession* stack_session, QWidget* widget_parent = 0);

    virtual void initialize();
    virtual void update() {}
    virtual void start();

  private:
    StackSession* stack_session;
    StackBodyView* view;

};

}

#endif
