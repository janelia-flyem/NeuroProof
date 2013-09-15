#ifndef STACKQTUI_H
#define STACKQTUI_H

#include "ui_neuroproof_stack_viewer.h"
#include "../Stack/StackSession.h"

namespace NeuroProof {

class StackQTUi : public QMainWindow, public StackObserver {
    Q_OBJECT

  public:
    StackQTUi(StackSession* stack_session_);
    ~StackQTUi();
            
    void clear_session();   
    void load_session(StackSession* stack_session_);
    void update();
    
    Ui::MainWindow ui;

  public slots:
    void slider_change(int val);
    void opacity_change(int val);

  private:
    StackSession * stack_session;
    unsigned int max_plane;
};


}

#endif
