#include "StackPlaneController.h"
#include "StackPlaneView.h"
#include "StackSession.h"
#include <vtkPointData.h>
#include <vtkRenderer.h>
#include <vtkAssemblyPath.h>
#include <vtkImageActor.h>
#include "QVTKWidget.h"

using namespace NeuroProof;
using std::string;

vtkClickCallback *vtkClickCallback::New() 
{
    return new vtkClickCallback();
}

void vtkClickCallback::Execute(vtkObject *caller, unsigned long, void*)
{
    vtkRenderWindowInteractor *iren = 
        reinterpret_cast<vtkRenderWindowInteractor*>(caller);
    char * sym_key = iren->GetKeySym();
    string key_val = "";
    if (sym_key) {
        key_val = string(sym_key);
    }

    vtkRenderer* renderer = image_viewer->GetRenderer();
    vtkImageActor* actor = image_viewer->GetImageActor();
    vtkInteractorStyle *style = vtkInteractorStyle::SafeDownCast(
            iren->GetInteractorStyle());

    // Pick at the mouse location provided by the interactor
    prop_picker->Pick( iren->GetEventPosition()[0],
            iren->GetEventPosition()[1],
            0.0, renderer );

    // There could be other props assigned to this picker, so 
    // make sure we picked the image actor
    vtkAssemblyPath* path = prop_picker->GetPath();
    bool validPick = false;

    if (path) {
        vtkCollectionSimpleIterator sit;
        path->InitTraversal(sit);
        vtkAssemblyNode *node;
        for(int i = 0; i < path->GetNumberOfItems() && !validPick; ++i)
        {
            node = path->GetNextNode( sit );
            if( actor == vtkImageActor::SafeDownCast( node->GetViewProp() ) )
            {
                validPick = true;
            }
        }
    }

    if ( !validPick ) {
        return;
    }
    double pos[3];
    prop_picker->GetPickPosition( pos );

    VolumeLabelPtr labelvol = stack_session->get_stack()->get_labelvol();
    if (iren->GetShiftKey()) {
        stack_session->active_label(int(pos[0]),
                labelvol->shape(1) - int(pos[1]) - 1, int(pos[2]));
    } else {
        stack_session->select_label(int(pos[0]),
                labelvol->shape(1) - int(pos[1]) - 1, int(pos[2]));
    }
}

vtkClickCallback::vtkClickCallback() : stack_session(0) 
{
    PointData = vtkPointData::New();
}


void vtkClickCallback::set_image_viewer(vtkSmartPointer<vtkImageViewer2> image_viewer_)
{
    image_viewer = image_viewer_;
}

void vtkClickCallback::set_picker(vtkSmartPointer<vtkPropPicker> prop_picker_)
{
    prop_picker = prop_picker_;
}

void vtkClickCallback::set_stack_session(StackSession * stack_session_)
{
    stack_session = stack_session_;
}

vtkClickCallback::~vtkClickCallback()
{
    PointData->Delete();
}

void vtkSimpInteractor::OnKeyPress()
{
    vtkRenderWindowInteractor *iren = this->Interactor;
    string key_val = string(iren->GetKeySym());
    
    if (key_val == "d") {
        // increase plane by going down stack
        stack_session->increment_plane();
    } else if (key_val == "s") {
        // decrease plane by going down stack
        stack_session->decrement_plane();
    } else if (key_val == "f") {
        stack_session->toggle_show_all();    
    } else if (key_val == "r") {
        stack_session->reset_active_labels();    
    } else if (key_val == "Up") {
        view->pan(0,10);
    } else if (key_val == "Down") {
        view->pan(0,-10);
    } else if (key_val == "Left") {
        view->pan(-10,0);
    } else if (key_val == "Right") {
        view->pan(10,0);
    }
}

void vtkSimpInteractor::OnKeyRelease()
{
    vtkRenderWindowInteractor *iren = this->Interactor;
    iren->SetKeySym(0); 
}
void vtkSimpInteractor::set_stack_session(StackSession * stack_session_)
{
    stack_session = stack_session_;
}
    
void vtkSimpInteractor::set_view(StackPlaneView* view_)
{
    view = view_;
}

StackPlaneController::StackPlaneController(StackSession* stack_session_) :
        stack_session(stack_session_), view(0)
{
    stack_session->attach_observer(this);
    Stack* stack = stack_session->get_stack();
    if (!stack) {
        throw ErrMsg("Controller started with no stack");
    }
    stack->attach_observer(this);

    view = new StackPlaneView(stack_session, this);
}

void StackPlaneController::update()
{

}

void StackPlaneController::initialize()
{
    view->initialize();

    vtkSmartPointer<vtkImageViewer2> image_viewer = view->get_viewer();
    vtkSmartPointer<vtkPropPicker> prop_picker = vtkSmartPointer<vtkPropPicker>::New();
    prop_picker->PickFromListOn();
    prop_picker->AddPickList(image_viewer->GetImageActor());

    click_callback = vtkSmartPointer<vtkClickCallback>::New();
    click_callback->set_stack_session(stack_session);
    click_callback->set_image_viewer(image_viewer);
    click_callback->set_picker(prop_picker);

    QVTKInteractor* interactor = view->get_interactor(); 
    interactor->AddObserver(vtkCommand::LeftButtonPressEvent, click_callback); 

    interactor_style = vtkSmartPointer<vtkSimpInteractor>::New();
    interactor_style->set_stack_session(stack_session);
    interactor_style->set_view(view);
    interactor->SetInteractorStyle(interactor_style);

    stack_session->set_plane(0);
}

void StackPlaneController::start()
{
    view->start();
}

