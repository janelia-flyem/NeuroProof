/*!
 * Functionality for viewing a plane within a label
 * volume.  The stack plane controller manipulates
 * this plane view.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/ 
#ifndef STACKPLANEVIEW_H
#define STACKPLANEVIEW_H

#include "StackObserver.h"
#include <vtkImageViewer2.h>
#include <vtkSmartPointer.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkImageMapToColors.h>
#include <vtkLookupTable.h>
#include <vtkImageBlend.h>
#include <vtkImageFlip.h>
#include "StackBase.h"

class QVTKWidget;
class QWidget;
class QVTKInteractor;
class QVBoxLayout;

namespace NeuroProof {

class StackSession;
class StackPlaneController;

/*!
 * A type of stack observer that subscribes to the stack
 * session model and listens for updates.  The plane view
 * points to the 32-bit label volume and show the grayscale
 * and label color for a given plane.  The plane displayed
 * can be changed.
*/
class StackPlaneView : public StackObserver {
  public:
    /*!
     * Constructor that attaches the view to the stack session
     * model.  The planar widget is attached a parent if provided.
     * \param stack_session session holding stack
     * \param controller plane controller
     * \param widget_parent main widget that contains the planar view
    */
    StackPlaneView(StackSession* stack_session,
            StackPlaneController* controller, QWidget* widget_parent_ = 0);

    /*!
     * Initialize the 3D dataset so that one plane can be shown
     * at a time.
    */
    virtual void initialize();

    /*!
     * Listens for chagnes to stack session.  Changes are initiated by
     * the plane controller.  See the plane controoler for the types of
     * operations that can be performed on the stack.
    */
    virtual void update();

    /*!
     * Retrieves the interactor for the image data viewer.
     * \return qt-vtk interactor
    */
    QVTKInteractor * get_interactor()
    {
        return renderWindowInteractor;
    } 

    /*!
     * Retrive the image viewer.
     * \return vtk image viewer
    */
    vtkSmartPointer<vtkImageViewer2> get_viewer()
    {
        return viewer;
    }

    /*!
     * Pan the camera by an incremental amount.
     * \param xdiff pan of camera in x direction
     * \param ydiff pan of camera in y direction
    */
    void pan(int xdiff, int ydiff);

    /*!
     * Destructor for class that removes widget from
     * parent widget if it existed.
    */
    ~StackPlaneView();

    /*!
     * Start the view widget.
    */
    virtual void start();

  protected:
    //! stack session model
    StackSession* stack_session;
    
    //! plane controller that creates view
    StackPlaneController* controller;

  private:
    /*!
     * Generate the colors for every node in the
     * RAG.  If the RAG was already colored, nothing
     * will change.  Colors stored in "color" property.
     * \param rag RAG corresponding to the image data
    */
    void load_rag_colors(RagPtr rag);
   
    //! widget containing image viewer 
    QVTKWidget * qt_widget;

    //! parent widget that contains image viewer widget
    QWidget * widget_parent;
 
    //! layout of parent to hold qt_widget 
    QVBoxLayout * layout;

    //! qt-vtk window interactor
    QVTKInteractor * renderWindowInteractor;

    //! plane viewer of 3D dataset
    vtkSmartPointer<vtkImageViewer2> viewer;

    //! blending of grayscale and label volume dataset
    vtkSmartPointer<vtkImageBlend> vtkblend;

    //! filter to flip image due to change in coordinate system
    vtkSmartPointer<vtkImageFlip> vtkblend_flipped;

    //! filter that maps the image volume to colors
    vtkSmartPointer<vtkImageMapToColors> labelvtk_mapped;

    //! color lookup table
    vtkSmartPointer<vtkLookupTable> label_lookup;

    //! vtk image dataset
    vtkSmartPointer<vtkImageData> labelvtk;

    //! holds label volume data
    vtkSmartPointer<vtkUnsignedIntArray> labelarray;

    //! filter that maps grayscale data
    vtkSmartPointer<vtkImageMapToColors> gray_mapped;

    //! trivial grayscale lookup table
    vtkSmartPointer<vtkLookupTable> graylookup;

    //! grayscale vtk image dataset
    vtkSmartPointer<vtkImageData> grayvtk;

    //! holds grayscale volume data
    vtkSmartPointer<vtkUnsignedCharArray> grayarray;

    //! holds initial zoom value -- could move to session or view state
    double initial_zoom;
};

}

#endif
