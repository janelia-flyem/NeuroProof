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

class QVTKWidget;
class QWidget;
class QVTKInteractor;
class QVBoxLayout;

namespace NeuroProof {

class StackSession;
class StackPlaneController;

class StackPlaneView : public StackObserver {
  public:
    StackPlaneView(StackSession* stack_session,
            StackPlaneController* controller, QWidget* widget_parent_ = 0);

    // call update and create controller
    virtual void initialize();

    virtual void update();

    QVTKInteractor * get_interactor()
    {
        return renderWindowInteractor;
    } 

    vtkSmartPointer<vtkImageViewer2> get_viewer()
    {
        return viewer;
    }

    void pan(int xdiff, int ydiff);

    ~StackPlaneView();

    virtual void start();

  protected:
    StackSession* stack_session;
    StackPlaneController* controller;

  private:
    QVTKWidget * qt_widget;
    QWidget * widget_parent;
  
    QVBoxLayout * layout;
    QVTKInteractor * renderWindowInteractor;
    vtkSmartPointer<vtkImageViewer2> viewer;
    vtkSmartPointer<vtkImageBlend> vtkblend;
    vtkSmartPointer<vtkImageFlip> vtkblend_flipped;
    vtkSmartPointer<vtkImageMapToColors> labelvtk_mapped;
    vtkSmartPointer<vtkLookupTable> label_lookup;
    vtkSmartPointer<vtkImageData> labelvtk;
    vtkSmartPointer<vtkUnsignedIntArray> labelarray;
    vtkSmartPointer<vtkImageMapToColors> gray_mapped;
    vtkSmartPointer<vtkLookupTable> graylookup;
    vtkSmartPointer<vtkImageData> grayvtk;
    vtkSmartPointer<vtkUnsignedCharArray> grayarray;
};

}

#endif
