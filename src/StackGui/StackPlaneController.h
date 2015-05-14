/*!
 * Contains functionality that determines what happens when
 * a user interacts with a planar view.  The primary mechanism
 * involves intercepting user events like clicks and key strokes
 * and changing the state of the stack session.  Changes to
 * the stack session triggers changes to the planar view.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef STACKPLANECONTROLLER_H
#define STACKPLANECONTROLLER_H

#include "StackObserver.h"
#include <vtkCommand.h>
#include <vtkSmartPointer.h>
#include <vtkInteractorStyleImage.h>
#include <vtkImageViewer2.h>
#include <vtkPropPicker.h>
#include <QWidget>

class vtkPointData;
class QWidget;
class QVTKWidget;

namespace NeuroProof {

class StackSession;
class StackPlaneView;

/*!
 * Class that handles clicks on the planar view widget.  In particular,
 * it handles body selection on a given plane.
*/
class vtkClickCallback : public vtkCommand {
  public:
    /*!
     * Static method to create a new intance of the class.
     * This is the only way to create a new object.
     * \return instance of object
    */
    static vtkClickCallback *New();
    
    /*!
     * Main function executing when a left user click occurs.
     * \param caller calling object (interactor)
     * \param unused unused param
     * \param unused unused param
    */
    virtual void Execute(vtkObject *caller, unsigned long, void*);
    
    /*!
     * Adds stack session to object.
     * \param stack_session_ stack session object
    */
    void set_stack_session(StackSession* stack_session_);
   
    /*!
     * Adds image viewer to object.
     * \param image_viewer_ vtk image viewer
    */
    void set_image_viewer(vtkSmartPointer<vtkImageViewer2> image_viewer_);
    
    /*!
     * Adds the picker to the object.  The picker will be examining
     * picks only on the image viewer.
     * \param prop_picker_ vtk picker
    */
    void set_picker(vtkSmartPointer<vtkPropPicker> prop_picker_);
   
    /*!
     * Allow active labels to be selected (using shift+left click).
    */
    void enable_selections() { enable_selection = true; }
    
    /*!
     * Disallow active labels to be selected (using shift+left click).
    */
    void disable_selections() { enable_selection = false; }
   
    /*!
     * Destructor deletes point data.
    */
    ~vtkClickCallback();


  private:
    /*!
     * Default constructor is private to enable heap allocation.
    */
    vtkClickCallback();
    
    //! point data created in constructor
    vtkPointData* PointData;
    
    //! stack session containing label volume that will be clicked.
    StackSession *stack_session;

    //! vtk image viewer
    vtkSmartPointer<vtkImageViewer2> image_viewer;

    //! picker object for vtk image viewer
    vtkSmartPointer<vtkPropPicker> prop_picker;

    //! boolean to enable/disable active label selections in picker.
    bool enable_selection;
};

/*!
 * Implements a style for the interactor handling the plane viewer.
 * In particular it handles different keyboard actions.
 * TODO: expose key events in this style to the main window widget.
*/
class vtkSimpInteractor : public vtkInteractorStyleImage
{
  public:
    /*!
     * Static function for creating object on the heap.
     * \return heap allocated object
    */
    static vtkSimpInteractor *New() 
    {
        return new vtkSimpInteractor;
    }
    
    /*!
     * Set the stack session model that corresponds to this
     * particular style.
     * \param stack_session_ stack session model
    */
    void set_stack_session(StackSession* stack_session_);
    
    /*!
     * Set the view that is controlled by the interactor and style.
     * \param view_ plane view object
    */
    void set_view(StackPlaneView* view_);
    
    /*!
     * Allow shift click selection operations to be registered
     * by the interactor style.
    */
    void enable_selections() { enable_selection = true; }
    
    /*!
     * Disallow shift click select operations to be registered
     * by the interactor style.
    */
    void disable_selections() { enable_selection = false; }
    
    /*!
     * Prevent actions from occuring on OnChar event.
    */
    virtual void OnChar() {}
    
    /*!
     * Implements actions when certain keys are pressed.
    */
    virtual void OnKeyPress(); 
    
    /*!
     * Clears the current key stored in interactor when the key is released.
    */
    virtual void OnKeyRelease(); 
    
    /*!
     * Prevent actions from occuring when mouse is moved.
    */
    virtual void OnMouseMove() {}

    virtual void OnMouseWheelBackward();
    virtual void OnMouseWheelForward();

  private:
    /*!
     * Private constructor for class.  Shift+click is enabled by default.
    */
    vtkSimpInteractor() : stack_session(0), enable_selection(true) {}
    
    //! stack session model
    StackSession *stack_session;

    //! plane view object
    StackPlaneView * view;

    //! enabling shift+click selection
    bool enable_selection;
};

/*!
 * This is the main controller for showing planes of images in
 * a stack.  It handles some signals produced by QT widget hence
 * deriving from QObject.  The controller is also an stack observer.
 * Currently, no stack events require handling.
*/
class StackPlaneController : public QObject, public StackObserver {
    Q_OBJECT
  public:
    /*!
     * Constructor for controller that requires a stack session
     * model and optionally takes in a parent widget that indicates
     * where the plane widget will be located.  Creates the view.
     * \param stack_session_ stack session model
     * \param widget_parent widget containing the plane (or 0 for no parent)
    */
    StackPlaneController(StackSession* stack_session_, QWidget* widget_parent = 0);
    
    /*!
     * Destructor the deletes the view.
    */
    ~StackPlaneController();

    /*!
     * Initializes the view and hooks up different event handlers to
     * the proper action.
    */
    virtual void initialize();
    
    /*!
     * No updates are performed when the stack session updates.
    */
    virtual void update() {}
    
    /*!
     * Starts the plane view widget.
    */
    virtual void start();
 
    /*!
     * Allow shift click selection operations to be registered
     * by the interactor style.
    */
    void enable_selections();
    
    /*!
     * Disallow shift click selection operations to be registered
     * by the interactor style.
    */
    void disable_selections();

  public slots:
    /*!
     * Handles the toggle all button (if it exists) such that all
     * the label colors are toggled between on and off.
    */
    void toggle_show_all();
 
    /*!
     * Handles the clear selection button (if it exists) such that
     * all active labels or cleared.
    */
    void clear_selection();
  
  private:
    //! stack session model
    StackSession* stack_session;
    
    //! plane view object 
    StackPlaneView* view;

    //! handles picking from mouse clicks
    vtkSmartPointer<vtkClickCallback> click_callback;

    //! handles keyboard presses
    vtkSmartPointer<vtkSimpInteractor> interactor_style;
};

}

#endif
