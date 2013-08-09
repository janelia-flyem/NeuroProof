#ifndef STACKQTUI_H
#define STACKQTUI_H

#include "ui_neuroproof_stack_viewer.h"

namespace NeuroProof {

class StackQTUi : public QMainWindow {
    Q_OBJECT

  public:
    StackQTUi()
    {
        ui.setupUi(this);
    }

  private:
    Ui::MainWindow ui;
};


}

#endif
