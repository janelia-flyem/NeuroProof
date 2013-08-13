#ifndef STACKBODYVIEW_H
#define STACKBODYVIEW_H

#include "StackObserver.h"
#include "VolumeLabelData.h"

#include <tr1/unordered_set>

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

class StackBodyView : public StackObserver {
  public:
    StackBodyView(StackSession* stack_session, QWidget* widget_parent_ = 0);

    // call update and create controller
    virtual void initialize();

    virtual void update();

    ~StackBodyView();

    virtual void start();

  protected:
    StackSession* stack_session;

  private:
    void create_label_volume(std::tr1::unordered_set<unsigned int>& labels);

    vtkSmartPointer<vtkRenderer> ren1;
    vtkSmartPointer<vtkVolume> volume; 
    vtkSmartPointer<vtkRenderWindow> rw;
    vtkSmartPointer<vtkVolumeProperty> volumeProperty;

    vtkSmartPointer<vtkUnsignedCharArray> labelarray;
    vtkSmartPointer<vtkImageData> labelvtk;
    vtkSmartPointer<vtkImageMapToColors> labelvtk_mapped;

    vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> volumeMapper;
    vtkSmartPointer<vtkColorTransferFunction> volumeColor;
    vtkSmartPointer<vtkPiecewiseFunction> volumeScalarOpacity;
    vtkSmartPointer<vtkImageResample> label_resample;
    
    QVTKWidget * qt_widget;
    QVTKInteractor * renderWindowInteractor;
    unsigned char * labels_rebase;
    VolumeLabelPtr current_volume_labels;



    vtkSmartPointer<vtkPlaneSource> plane_source;
    vtkSmartPointer<vtkPolyDataMapper> plane_mapper;
    vtkSmartPointer<vtkActor> actor;
    QWidget * widget_parent;
    QVBoxLayout * layout;
};

}

#endif
