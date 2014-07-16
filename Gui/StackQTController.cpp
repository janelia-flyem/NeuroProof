#include "StackQTController.h"
#include "StackQTUi.h"
#include "../Stack/StackPlaneController.h"
#include "../Stack/StackBodyController.h"
#include <QTimer>
#include "../Stack/StackSession.h"
#include "../EdgeEditor/EdgeEditor.h"
#include "MessageBox.h"
#include <QFileDialog>
#include <QApplication>
#include <algorithm>
#include <json/json.h>
#include <json/value.h>
#include <sstream>

using std::stringstream;
using namespace NeuroProof;
using std::cout; using std::endl; using std::string;
using std::vector; using std::sort;

StackQTController::StackQTController(StackSession* stack_session_, QApplication* qapp_) : 
    stack_session(stack_session_), qapp(qapp_), body_controller(0), 
    plane_controller(0), priority_scheduler(0), training_mode(false)
{
    main_ui = new StackQTUi(stack_session);
    
    // delays loading the views so that the main window is displayed first
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

    QObject::connect(main_ui->ui.actionViewOnly, 
            SIGNAL(triggered()), this, SLOT(start_viewonly()));

    QObject::connect(main_ui->ui.actionTraining, 
            SIGNAL(triggered()), this, SLOT(start_training()));

    QObject::connect(main_ui->ui.viewToolPanel, 
            SIGNAL(triggered()), this, SLOT(view_tool_panel()));

    QObject::connect(main_ui->ui.planeSlider,
            SIGNAL(valueChanged(int)), main_ui, SLOT(slider_change(int)) );
    
    QObject::connect(main_ui->ui.opacitySlider,
            SIGNAL(valueChanged(int)), main_ui, SLOT(opacity_change(int)) );

    QObject::connect(main_ui->ui.toggleGT, 
            SIGNAL(clicked()), this, SLOT(toggle_gt()));
    
    QObject::connect(main_ui->ui.currentButton, 
            SIGNAL(clicked()), this, SLOT(grab_current_edge()));
    
    QObject::connect(main_ui->ui.nextButton, 
            SIGNAL(clicked()), this, SLOT(grab_next_edge()));
    
    QObject::connect(main_ui->ui.undoButton, 
            SIGNAL(clicked()), this, SLOT(undo_edge()));

    QObject::connect(main_ui->ui.mergeButton, 
            SIGNAL(clicked()), this, SLOT(merge_edge()));
}

void StackQTController::start_viewonly()
{
    training_mode = false;
    // changing view modes resets the stack and deleted the current body viewer
    if (body_controller) {
        delete body_controller;
        body_controller = 0;
        main_ui->ui.enable3DCheck->setChecked(false);
    }

    stack_session->set_reset_stack();
    plane_controller->enable_selections();
    main_ui->ui.modeWidget->setCurrentIndex(1);
}

void StackQTController::update()
{
    if (training_mode) {
        bool merge_bodies;
        bool merge_change = stack_session->get_merge_bodies(merge_bodies);

        bool next_bodies;
        bool next_change = stack_session->get_next_bodies(next_bodies);

        if (merge_change) {
            merge_edge();
        } else if (next_change) {
            grab_next_edge();
        } 
    } 
}

void StackQTController::start_training()
{
    if (body_controller) {
        delete body_controller;
        body_controller = 0;
        main_ui->ui.enable3DCheck->setChecked(false);
    }

    if (stack_session->is_gt_mode()) {
        // for some reason the message has to be called twice to display
        main_ui->ui.statusbar->showMessage("Restoring labels...");
        main_ui->ui.statusbar->showMessage("Restoring labels...");
        toggle_gt();
        main_ui->ui.statusbar->clearMessage(); 
    } else {
        main_ui->ui.statusbar->showMessage("Resetting stack...");
        main_ui->ui.statusbar->showMessage("Resetting stack...");
        stack_session->set_reset_stack();
        main_ui->ui.statusbar->clearMessage(); 
    }

    // shows the training control widgets
    main_ui->ui.modeWidget->setCurrentIndex(0);
    plane_controller->disable_selections();

    
    //     ask for the predictions
    QString prob_name_t = QFileDialog::getOpenFileName(main_ui, tr("Add probability volumes"), 
            ".", tr("H5 Files(*.h5)"));
    string prob_name = prob_name_t.toStdString();
    if (prob_name == "") {
        return;
    }
    //load probability
    stack_session->get_stack()->read_prob_list(prob_name,string("volume/predictions"));
    // call BioStack rag() again
    stack_session->get_stack()->build_rag();    
    stack_session->get_stack()->set_classifier();    
    stack_session->get_stack()->set_edge_locations();
    // reset rag pointers in StackSession
    stack_session->set_reset_stack();
    
    // check if scheduler is already loaded
    if (!priority_scheduler) {
        // edge location already computed in session
        Json::Value json_vals_priority;
        Json::Value empty_list;
        json_vals_priority["synapse_bodies"] = empty_list;
        json_vals_priority["orphan_bodies"] = empty_list;
        RagPtr rag = stack_session->get_stack()->get_rag();
        
        // TODO: allow user to change priority mode (training, body, etc)
        // TODO: compute real probabilities for the edge, looking at everything
        priority_scheduler = new EdgeEditor(*rag, 0.0,0.99,0.0,json_vals_priority);
	priority_scheduler->set_splearn_mode(stack_session->get_stack());
    }
    
    training_mode = true;
    update_progress();
    grab_current_edge();
}

void StackQTController::grab_next_edge()
{

    if ((priority_scheduler->isFinished())) {
        MessageBox msgbox("No more edges");
	printf("fininshed, need to save\n");
	QString dir_name = QFileDialog::getExistingDirectory(main_ui, tr("Choose classifier directory"), ".");
	string session_name = dir_name.toStdString();
	if (session_name == "") {
	    return;
	}
	stack_session->get_stack()->save_classifier(session_name+"/iterative_classifier.xml");
    } else {
        Node_t node1, node2;
        bool remove = stack_session->is_remove_edge(node1, node2);
	int edge_label = remove? -1: 1;
	priority_scheduler->set_edge_label(edge_label);
//         boost::tuple<Node_t, Node_t> body_pair(node1, node2);
//         priority_scheduler->removeEdge(body_pair, remove);
//         // this actually does the merging of labels if necessary
//         stack_session->set_commit_edge(node2, node1, true);
        grab_current_edge(); 
    }
    
    update_progress();
}

void StackQTController::undo_edge()
{
    Node_t node1, node2;
    if (stack_session->is_remove_edge(node1, node2)) {
        stack_session->unmerge_edge(); 
    } else if (stack_session->undo_queue_empty()) {
        MessageBox msgbox("Cannot undo anymore");
    } else {
        priority_scheduler->undo();
        Location location;
        boost::tuple<Node_t, Node_t> pair = priority_scheduler->getTopEdge(location);
        Label_t node1 = boost::get<0>(pair);
        Label_t node2 = boost::get<1>(pair);
        // always assume that node2 is the one that could be getting restored
        stack_session->set_undo_edge(node1, node2);
        stack_session->set_body_pair(node1, node2, location);
    }

    update_progress();
}

void StackQTController::update_progress()
{
    int num_processed = stack_session->get_num_examined_edges();
    int num_total = num_processed + priority_scheduler->getNumRemainingQueue();
    stringstream str;
    str << num_processed << "/" << num_total << " edges examined";

    main_ui->ui.progressLabel->setText(str.str().c_str());
}


void StackQTController::merge_edge()
{
    if ((priority_scheduler->isFinished())) {
        MessageBox msgbox("No more edges");
    } else {
        stack_session->merge_edge();
    }
}

void StackQTController::grab_current_edge()
{
    if (!(priority_scheduler->isFinished())) {
        Location location;
        boost::tuple<Node_t, Node_t> pair = priority_scheduler->getTopEdge(location);
        Label_t node1 = boost::get<0>(pair);
        Label_t node2 = boost::get<1>(pair);
	printf("received firstedge: (%d, %d)\n", node1, node2);
        stack_session->set_body_pair(node1, node2, location);
    } else {
        MessageBox msgbox("No more edges");
    }
}

void StackQTController::toggle_gt()
{
    // toggling gt will reset the stack and will destroy the current 3D viewer
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

    // this will overwrite any GT currently associated with the stack
    string msg = string("Adding groundtruth labels ") + gt_name + string("...");
    main_ui->ui.statusbar->showMessage(msg.c_str());
    main_ui->ui.statusbar->showMessage(msg.c_str());
    stack_session->load_gt(gt_name); 
    main_ui->ui.toggleGT->setEnabled(true);
    main_ui->ui.statusbar->clearMessage();
}

void StackQTController::new_session()
{
  //  printf("start new session\n");
    QStringList file_name_list = QFileDialog::getOpenFileNames(main_ui, tr("Add label volume"),
        ".", tr("PNG(*.png *.PNG);;JPEG(*.jpg *.jpeg)"));
    
    printf("file names %d\n",file_name_list.size());

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
    
    // TODO: move to view -- have event triggered by session
    main_ui->ui.menuMode->setEnabled(true);
    string msg = string("Saving session as ") + session_name + string("...");
    main_ui->ui.statusbar->showMessage(msg.c_str());
    main_ui->ui.statusbar->showMessage(msg.c_str());
    try {
        stack_session->export_session(session_name); 
	priority_scheduler->save_labeled_edges(session_name);
      
    } catch (ErrMsg& err_msg) {
        string msg = string("Session could not be created: ") + err_msg.str;
        MessageBox msgbox(msg.c_str());
    }
    main_ui->ui.menuMode->setEnabled(true);
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
	priority_scheduler->save_labeled_edges(session_name);
        main_ui->ui.statusbar->clearMessage();
    } else {
        save_as_session();
    }
}

void StackQTController::quit_program()
{
    // exits the application
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
    msg += "Scroll: increment/decrement plane\n";
    msg += "Shift+Scroll: zoom in/out\n";
    msg += "t: merge bodies\n";
    msg += "u: next bodies\n";

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
    if (priority_scheduler) {
        delete priority_scheduler;
        priority_scheduler = 0;
    }
    if (stack_session) {
        main_ui->clear_session();
        stack_session->detach_observer(this);
        delete stack_session;
        stack_session = 0;
    }
}

StackQTController::~StackQTController()
{
    clear_session();
    delete main_ui;
}

// TODO: move functionality to Qt view
void StackQTController::view_body_panel()
{
    // widget3 is the body panel dock widget
    main_ui->ui.dockWidget3->show();
}

// TODO: move functionality to Qt view
void StackQTController::view_tool_panel()
{
    // widget2 is the tool panel dock widget
    main_ui->ui.dockWidget2->show();
}

void StackQTController::load_views()
{
    if (!stack_session) {
        return;
    }
    stack_session->attach_observer(this);

    // for some reason, I have to call this twice for it to display properly
    main_ui->ui.statusbar->showMessage("Loading Session...");
    main_ui->ui.statusbar->showMessage("Loading Session...");

    plane_controller = new StackPlaneController(stack_session, main_ui->ui.planeView);
    plane_controller->initialize();
    plane_controller->start();

    main_ui->ui.toggleLabels->disconnect();
    main_ui->ui.clearSelection->disconnect();

    // connect plane controller functionality to the show all labels and clear
    // active label signals
    QObject::connect(main_ui->ui.toggleLabels, 
            SIGNAL(clicked()), plane_controller, SLOT(toggle_show_all()));

    QObject::connect(main_ui->ui.clearSelection, 
            SIGNAL(clicked()), plane_controller, SLOT(clear_selection()));
    
    main_ui->ui.statusbar->clearMessage();
}
