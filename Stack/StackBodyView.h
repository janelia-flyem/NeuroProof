/*!
 * Functionality for viewing the a body in 3D from
 * a dense volume labeling stored in a stack.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef STACKBODYVIEW_H
#define STACKBODYVIEW_H

#include "StackObserver.h"
#include "VolumeLabelData.h"

#include <tr1/unordered_set>

// vtk headers are included since class member variables are not pointers
#include <vtkSmartPointer.h>
#include <vtkUnsignedCharArray.h>
#include <vtkImageData.h>
#include <vtkImageMapToColors.h>
#include <vtkVolumeProperty.h>
#include <vtkVolume.h>
#include <vtkRenderWindow.h>
#include <vtkPolyDataMapper.h>

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h> 
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkImageResample.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h> 
#include <vtkProperty.h>
#include <vtkPlaneSource.h>

class QVTKWidget;
class QVTKInteractor;
class QWidget;
class QVBoxLayout;

namespace NeuroProof {

class StackSession;

/*!
 * A type of stack observer that subscribes to the stack session
 * model and listens for updates.  The body view processes the stack
 * to generate a simpler label volume that can be quickly rendered
 * to view 3D bodies.  Only active labels in the stack session
 * will visible in the 3D viewer.  TODO: Boost the speed of the
 * tool to load more quickly./
*/
class StackBodyView : public StackObserver {
  public:
    /*!
     * Constructor takes stack session model and an optional main
     * window widget that will hold the 3D viewer.
     * \param stack_session session holding stack
     * \param widget_parent main widget that contains the body view
    */
    StackBodyView(StackSession* stack_session, QWidget* widget_parent_ = 0);

    /*!
     * Initialize basic datastructures to enable loading of
     * 3D bodies when bodies are added to active labels.  If this
     * function is called when the stack session already has active
     * labels, those bodies will be shown initially.
    */
    virtual void initialize();

    /*!
     * Listens for changes to stack session.  Changes in the active
     * labels will change the bodies shown.
    */
    virtual void update();

    /*!
     * Destructor that removes view from the parent widget.
    */
    ~StackBodyView();

    /*!
     * Start the view widget.
    */
    virtual void start();

  protected:
    //! stack session model
    StackSession* stack_session;

  private:
    /*!
     * Create an 8-bit label volume from a 32-bit label volume.  It will
     * associated color ids only for the the bodies in the active labels list.
     * \param labels mapping for active labels to colors that should be shown
    */
    void create_label_volume(std::tr1::unordered_map<unsigned int, int>& labels);
    
    /*!
     * Internal update variable that will look for the initial
     * state in stack session even if there are no updates
     * if the initialize variable is true.
     * \param intialize boolean to indicate whether initialization will occur
    */
    virtual void _update(bool initialize);

    //! renderer for 3D scene
    vtkSmartPointer<vtkRenderer> ren1;

    //! volume containing 3D body labels
    vtkSmartPointer<vtkVolume> volume;

    //! window containing rendered object
    vtkSmartPointer<vtkRenderWindow> rw;

    //! holds color and opacity properties for volume 
    vtkSmartPointer<vtkVolumeProperty> volumeProperty;

    //! array containing body color ids
    vtkSmartPointer<vtkUnsignedCharArray> labelarray;

    //! 3D dataset holding label array 
    vtkSmartPointer<vtkImageData> labelvtk;

    //! volume raycast mapper 
    vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> volumeMapper;

    //! color for body image data
    vtkSmartPointer<vtkColorTransferFunction> volumeColor;

    //! opacity for body image data
    vtkSmartPointer<vtkPiecewiseFunction> volumeScalarOpacity;

    //! resampling of image data to make it faster to load
    vtkSmartPointer<vtkImageResample> label_resample;
    
    //! widget containing window
    QVTKWidget * qt_widget;

    //! interactor used in qt widget
    QVTKInteractor * renderWindowInteractor;

    //! value passed to vtk image data array -- deleted by vtk
    unsigned char * labels_rebase;

    //! copy of label volume that is rebased 
    VolumeLabelPtr current_volume_labels;

    //! variable defining 2D plane that shows current stack plane
    vtkSmartPointer<vtkPlaneSource> plane_source;

    //! mapper of plane
    vtkSmartPointer<vtkPolyDataMapper> plane_mapper;
    
    //! actor that holds plane
    vtkSmartPointer<vtkActor> actor;

    //! widget that holds this 3D body widget
    QWidget * widget_parent;

    //! layout created to place the 3D body widget on the parent
    QVBoxLayout * layout;
};

}

#endif
