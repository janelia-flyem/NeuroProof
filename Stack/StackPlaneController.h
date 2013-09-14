#ifndef STACKPLANECONTROLLER_H
#define STACKPLANECONTROLLER_H

#include "StackObserver.h"
#include <vtkCommand.h>
#include <vtkSmartPointer.h>
#include <vtkInteractorStyleImage.h>
//#include <vtkRenderWindowInteractor.h>
#include <vtkImageViewer2.h>
#include <vtkPropPicker.h>
#include <QWidget>

class vtkPointData;
class QWidget;
class QVTKWidget;

namespace NeuroProof {

class StackSession;
class StackPlaneView;

class vtkClickCallback : public vtkCommand {
  public:
    static vtkClickCallback *New();
    virtual void Execute(vtkObject *caller, unsigned long, void*);
    void set_stack_session(StackSession* stack_session_);
    void set_image_viewer(vtkSmartPointer<vtkImageViewer2> image_viewer_);
    void set_picker(vtkSmartPointer<vtkPropPicker> prop_picker_);
    void enable_selections() { enable_selection = true; }
    void disable_selections() { enable_selection = false; }
    ~vtkClickCallback();


  private:
    vtkClickCallback();
    
    vtkPointData* PointData;
    StackSession *stack_session;
    vtkSmartPointer<vtkImageViewer2> image_viewer;
    vtkSmartPointer<vtkPropPicker> prop_picker;
    bool enable_selection;
};

class vtkSimpInteractor : public vtkInteractorStyleImage
{
  public:
    static vtkSimpInteractor *New() 
    {
        return new vtkSimpInteractor;
    }
    void set_stack_session(StackSession* stack_session_);
    void set_view(StackPlaneView* view_);
    void enable_selections() { enable_selection = true; }
    void disable_selections() { enable_selection = false; }
    virtual void OnChar() {}
    virtual void OnKeyPress(); 
    virtual void OnKeyRelease(); 
    virtual void OnMouseMove() 
    {
        //if (this->Interactor->GetShiftKey()) {
        //    vtkInteractorStyleImage::OnMouseMove();
        //}
    }
  private:
    vtkSimpInteractor() : stack_session(0), enable_selection(true) {}
    
    StackSession *stack_session;
    StackPlaneView * view;
    bool enable_selection;
};

class StackPlaneController : public QObject, public StackObserver {
    Q_OBJECT
  public:
    StackPlaneController(StackSession* stack_session, QWidget* widget_parent = 0);
    ~StackPlaneController();

    virtual void initialize();
    virtual void update();
    virtual void start();
    void enable_selections();
    void disable_selections();

  public slots:
      void toggle_show_all();
      void clear_selection();
  
  private:
    StackSession* stack_session;
    StackPlaneView* view;

    vtkSmartPointer<vtkClickCallback> click_callback;
    vtkSmartPointer<vtkSimpInteractor> interactor_style;
};

}

#endif
