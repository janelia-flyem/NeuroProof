#include "StackQTController.h"
#include "StackQTUi.h"
#include "../Stack/StackPlaneController.h"
#include "../Stack/StackBodyController.h"
#include <QTimer>
#include "../Stack/StackSession.h"
#include "MessageBox.h"
#include <QFileDialog>
#include <QApplication>
#include <algorithm>

using namespace NeuroProof;
using std::cout; using std::endl; using std::string;
using std::vector; using std::sort;

StackQTController::StackQTController(StackSession* stack_session_, QApplication* qapp_) : 
    stack_session(stack_session_), qapp(qapp_), body_controller(0), plane_controller(0)
{
    main_ui = new StackQTUi(stack_session);

    /*if (stack_session) {
        load_views();
    }*/

    QTimer::singleShot(1000, this, SLOT(load_views()));
    main_ui->showMaximized();
    main_ui->show();

    QObject::connect(main_ui->ui.enable3DCheck, 
            SIGNAL(released()), this, SLOT(toggle3D()));

    QObject::connect(main_ui->ui.viewBodyPanel, 
            SIGNAL(triggered()), this, SLOT(view_body_panel()));

    QObject::connect(main_ui->ui.actionShortcuts, 
            SIGNAL(triggered()), this, SLOT(show_shortcuts()));
    
    QObject::connect(main_ui->ui.actionAbout, 
            SIGNAL(triggered()), this, SLOT(show_about()));
    
    QObject::connect(main_ui->ui.actionHelp, 
            SIGNAL(triggered()), this, SLOT(show_help()));

    QObject::connect(main_ui->ui.actionNewProject, 
            SIGNAL(triggered()), this, SLOT(new_session()));

    QObject::connect(main_ui->ui.actionOpenProject, 
            SIGNAL(triggered()), this, SLOT(open_session()));

    QObject::connect(main_ui->ui.actionSaveAsProject, 
            SIGNAL(triggered()), this, SLOT(save_as_session()));

    QObject::connect(main_ui->ui.actionSaveProject, 
            SIGNAL(triggered()), this, SLOT(save_session()));
   
    QObject::connect(main_ui->ui.actionAddGT, 
            SIGNAL(triggered()), this, SLOT(add_gt()));

    QObject::connect(main_ui->ui.actionQuit, 
            SIGNAL(triggered()), this, SLOT(quit_program()));
 
    QObject::connect(main_ui->ui.viewToolPanel, 
            SIGNAL(triggered()), this, SLOT(view_tool_panel()));


    QObject::connect(main_ui->ui.planeSlider,
            SIGNAL(valueChanged(int)), main_ui, SLOT(slider_change(int)) );
    
    QObject::connect(main_ui->ui.opacitySlider,
            SIGNAL(valueChanged(int)), main_ui, SLOT(opacity_change(int)) );

    QObject::connect(main_ui->ui.toggleGT, 
            SIGNAL(clicked()), this, SLOT(toggle_gt()));
}

void StackQTController::toggle_gt()
{
    if (body_controller) {
        delete body_controller;
        body_controller = 0;
    }
    
    stack_session->toggle_gt();
}

void StackQTController::add_gt()
{
    QString file_name = QFileDialog::getOpenFileName(main_ui, tr("Add Groundtruth Labels"), 
            ".", tr("H5 Files(*.h5)"));
    string gt_name = file_name.toStdString();
    if (gt_name == "") {
        return;
    }

    string msg = string("Adding groundtruth labels ") + gt_name + string("...");
    main_ui->ui.statusbar->showMessage(msg.c_str());
    main_ui->ui.statusbar->showMessage(msg.c_str());
    stack_session->load_gt(gt_name); 
    main_ui->ui.toggleGT->setEnabled(true);
    main_ui->ui.statusbar->clearMessage();
}

void StackQTController::new_session()
{
    QStringList file_name_list = QFileDialog::getOpenFileNames(main_ui, tr("Add label volume"),
        ".", tr("PNG(*.png *.PNG);;JPEG(*.jpg *.jpeg)"));

    if (file_name_list.size() == 0) {
        return;
    }
   
    // files must be sortable for this to work correctly 
    vector<string> file_name_vec;
    for (int i = 0; i < file_name_list.size(); ++i) {
        file_name_vec.push_back(file_name_list.at(i).toStdString());
    }
    sort(file_name_vec.begin(), file_name_vec.end()); 

    QString label_name_t = QFileDialog::getOpenFileName(main_ui, tr("Add label volume"), 
            ".", tr("H5 Files(*.h5)"));
    string label_name = label_name_t.toStdString();
    if (label_name == "") {
        return;
    }
    
    try {
        clear_session();
        main_ui->ui.statusbar->showMessage("Initializing Session...");
        main_ui->ui.statusbar->showMessage("Loading Session...");
        stack_session = new StackSession(file_name_vec, label_name);
        main_ui->ui.statusbar->clearMessage();
        main_ui->load_session(stack_session);
        load_views();
    } catch (ErrMsg& err_msg) {
        string msg = string("Session could not be created: ") + err_msg.str;
        MessageBox msgbox(msg.c_str());
    }
}

void StackQTController::save_as_session()
{
    QString dir_name = QFileDialog::getExistingDirectory(main_ui, tr("Save Session"), ".");
    string session_name = dir_name.toStdString();
    if (session_name == "") {
        return;
    }
    
    string msg = string("Saving session as ") + session_name + string("...");
    main_ui->ui.statusbar->showMessage(msg.c_str());
    main_ui->ui.statusbar->showMessage(msg.c_str());
    try {
        stack_session->export_session(session_name); 
    } catch (ErrMsg& err_msg) {
        string msg = string("Session could not be created: ") + err_msg.str;
        MessageBox msgbox(msg.c_str());
    }
    main_ui->ui.statusbar->clearMessage();
}

void StackQTController::save_session()
{
    if (stack_session->has_session_name()) {
        string session_name = stack_session->get_session_name();
        string msg = string("Saving session ") + session_name + string("...");
        main_ui->ui.statusbar->showMessage(msg.c_str());
        main_ui->ui.statusbar->showMessage(msg.c_str());
        stack_session->save();
        main_ui->ui.statusbar->clearMessage();
    } else {
        save_as_session();
    }
}


void StackQTController::quit_program()
{
    qapp->exit(0);
}

void StackQTController::open_session()
{
    QString dir_name = QFileDialog::getExistingDirectory(main_ui, tr("Open Session"), ".");
    string session_name = dir_name.toStdString();
    if (session_name == "") {
        return;
    }
    
    try {
        clear_session();
        stack_session = new StackSession(session_name);
        main_ui->load_session(stack_session);
        load_views();
    } catch (ErrMsg& err_msg) {
        string msg = string("Session could not be created: ") + err_msg.str;
        MessageBox msgbox(msg.c_str());
    }
}


void StackQTController::show_shortcuts()
{
    string msg;
    msg = "Current Key Bindings (not configurable)\n\n";
    msg += "d: increment plane\n";
    msg += "s: decrement plane\n";
    msg += "f: toggle label colors\n";
    msg += "r: empty active body list\n";
    msg += "Up: pan up\n";
    msg += "Down: pan down\n";
    msg += "Left: pan left\n";
    msg += "Right: pan right\n";
    msg += "Left click: select body\n";
    msg += "Shift left click: adds body to active list\n";

    MessageBox msgbox(msg.c_str());
}

void StackQTController::show_help()
{
    string msg;

    msg = "Please consult the documentation found in the NeuroProof README";
    msg += " and the README in the examples sub-repository for information";
    msg += " on to use neuroproof_stack_viewer\n";

    MessageBox msgbox(msg.c_str());
}

void StackQTController::show_about()
{
    string msg;
    
    msg = "The NeuroProof software is an image segmentation tool currently";
    msg += " being used in the FlyEM project at Janelia Farm Research Campus";
    msg += " to help reconstruct neuronal structure in the fly brain.\n";

    msg += "Stephen Plaza (plaza.stephen@gmail.com)";
    
    MessageBox msgbox(msg.c_str());
}

void StackQTController::toggle3D()
{
    bool state = main_ui->ui.enable3DCheck->isChecked();
    if (state && stack_session) {
        if (body_controller) {
            delete body_controller;
        }
        body_controller = new StackBodyController(stack_session, main_ui->ui.bodyView);
        body_controller->initialize();
        body_controller->start();
    } else if (stack_session && body_controller) {
        delete body_controller;
        body_controller = 0;
    }
}

void StackQTController::clear_session()
{
    if (body_controller) {
        delete body_controller;
        body_controller = 0;
    }
    if (plane_controller) {
        delete plane_controller;
        plane_controller = 0;
    }
    if (stack_session) {
        main_ui->clear_session();
        delete stack_session;
        stack_session = 0;
    }
}

StackQTController::~StackQTController()
{
    clear_session();
    delete main_ui;
}



// ?! maybe move to the main view
void StackQTController::view_body_panel()
{
    main_ui->ui.dockWidget3->show();
}

// ?! maybe move to the main view
void StackQTController::view_tool_panel()
{
    main_ui->ui.dockWidget2->show();
}

void StackQTController::load_views()
{
    if (!stack_session) {
        return;
    }

    // for some reason, I have to call this twice for it to display properly
    main_ui->ui.statusbar->showMessage("Loading Session...");
    main_ui->ui.statusbar->showMessage("Loading Session...");

    plane_controller = new StackPlaneController(stack_session, main_ui->ui.planeView);
    plane_controller->initialize();
    plane_controller->start();

    main_ui->ui.toggleLabels->disconnect();
    main_ui->ui.clearSelection->disconnect();

    QObject::connect(main_ui->ui.toggleLabels, 
            SIGNAL(clicked()), plane_controller, SLOT(toggle_show_all()));

    QObject::connect(main_ui->ui.clearSelection, 
            SIGNAL(clicked()), plane_controller, SLOT(clear_selection()));
    
    main_ui->ui.statusbar->clearMessage();
}
