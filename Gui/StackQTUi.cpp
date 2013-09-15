#include "StackQTUi.h"

using namespace NeuroProof;

StackQTUi::StackQTUi(StackSession* stack_session_) : stack_session(stack_session_)
{
    ui.setupUi(this);
    if (stack_session) {
        load_session(stack_session);
    } else {
        clear_session();
    }
}

StackQTUi::~StackQTUi()
{
    clear_session();
}

void StackQTUi::clear_session()
{
    if (stack_session) {
        stack_session->detach_observer(this);
        stack_session = 0;
    }
    ui.menuMode->setEnabled(false);
    ui.actionAddGT->setEnabled(false);
    ui.actionSaveProject->setEnabled(false);
    ui.actionSaveAsProject->setEnabled(false);
    ui.dockWidget2->setEnabled(false);
}

void StackQTUi::load_session(StackSession* stack_session_)
{
    if (stack_session) {
        stack_session->detach_observer(this);
    }
    stack_session = stack_session_;
    stack_session->attach_observer(this);


    if (stack_session->has_session_name()) {
        ui.menuMode->setEnabled(true);
    }
    ui.actionAddGT->setEnabled(true);
    ui.actionSaveProject->setEnabled(true);
    ui.actionSaveAsProject->setEnabled(true);
    ui.dockWidget2->setEnabled(true);
    ui.modeWidget->setCurrentIndex(1);

    unsigned int plane_id = 0;    
    stack_session->get_plane(plane_id);

    Stack *stack = stack_session->get_stack();

    if (stack_session->has_gt_stack()) {
        ui.toggleGT->setEnabled(true);
    } else {
        ui.toggleGT->setEnabled(false);
    }
    ui.toggleGT->setText("Show GT Labels");

    max_plane = stack->get_zsize()-1;
    ui.planeSlider->setRange(0, max_plane);
    ui.planeSlider->setValue(max_plane - plane_id);

    unsigned int opacity_val = 0;
    stack_session->get_opacity(opacity_val);
    ui.opacitySlider->setValue(opacity_val);
}

void StackQTUi::update()
{
    unsigned int plane_id = 0;    
    if (stack_session->get_plane(plane_id)) {
        ui.planeSlider->setValue(max_plane - plane_id);
    }

    unsigned int opacity_val = 0;
    if (stack_session->get_opacity(opacity_val)) {
        ui.opacitySlider->setValue(opacity_val);
    }

    VolumeLabelPtr labelvol;
    RagPtr rag; 
    if (stack_session->get_reset_stack(labelvol, rag)) {
        if (stack_session->is_gt_mode()) {
            ui.toggleGT->setText("Show Orig Labels");
        } else {
            ui.toggleGT->setText("Show GT Labels");
        }
        ui.enable3DCheck->setChecked(false);
    }
}

void StackQTUi::slider_change(int val)
{
    stack_session->set_plane(max_plane - val);
}

void StackQTUi::opacity_change(int val)
{
    stack_session->set_opacity(val);
}


