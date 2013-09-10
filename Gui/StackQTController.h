#ifndef STACKQTCONTROLLER_H
#define STACKQTCONTROLLER_H

#include "../Stack/StackObserver.h"
#include <QWidget>

class QApplication;

namespace NeuroProof {


class StackSession;
class StackQTUi;
class StackPlaneController;
class StackBodyController;

class StackQTController : public QObject, public StackObserver {
    Q_OBJECT
  
  public:
    StackQTController(StackSession* stack_session_, QApplication* qapp_);
    ~StackQTController();

    void update() {}

  private:
    void clear_session();

    StackSession* stack_session;
    QApplication* qapp;
    StackQTUi* main_ui;
    StackPlaneController* plane_controller;
    StackBodyController* body_controller;

  private slots: 
    void open_session();
    void quit_program();
    void save_session();
    void new_session();
    void save_as_session();
    void add_gt();
    void load_views();
    void show_shortcuts();
    void show_about();
    void show_help();
    void toggle_gt();
    void toggle3D();
    void view_body_panel();
    void view_tool_panel();
};

}

#endif
