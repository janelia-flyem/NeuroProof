#include "StackBodyView.h"
#include "StackSession.h"
#include "QVTKWidget.h"
#include <QtGui/QWidget>

#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkInteractorStyle.h>
#include <vtkPointData.h>
#include <tr1/unordered_map>

#include <boost/thread/mutex.hpp>

using namespace NeuroProof;
using std::tr1::unordered_set;
using std::tr1::unordered_map;

#include "../Utilities/ScopeTime.h"
#include <QVBoxLayout>

static boost::mutex mutex_create;
static boost::mutex mutex_meta;

StackBodyView::StackBodyView(StackSession* stack_session_, QWidget* widget_parent_) :
        stack_session(stack_session_), qt_widget(0), widget_parent(widget_parent_)
{
    stack_session->attach_observer(this);
} 

StackBodyView::~StackBodyView()
{
    stack_session->detach_observer(this);
    // remove the widget from the main window if a main window exists
    if (qt_widget) {
        if (layout) {
            QLayoutItem * item;
            if ((item = layout->takeAt(0)) != 0) {
                layout->removeItem(item);
            }
            layout->removeWidget(qt_widget);
            delete layout;
        }
        delete qt_widget;
    }
}

void StackBodyView::create_label_volume(std::tr1::unordered_map<unsigned int, int>& labels)
{
    unordered_map<Label_t, unsigned char> color_mapping;
    Stack* stack = stack_session->get_stack();
    RagPtr rag = stack->get_rag();

    // load mappings of a label id to a color
    for (unordered_map<Label_t, int>::iterator iter = labels.begin(); 
            iter != labels.end(); ++iter) {
        unsigned char cmap = (iter->second) % 18 + 1;
        color_mapping[iter->first] = cmap;
    }

    unsigned char * mapped_iter = labels_rebase;
    Label_t* label_iter = current_volume_labels->data();

    unsigned long long vol_size = current_volume_labels->shape(0) * 
        current_volume_labels->shape(1) * current_volume_labels->shape(2);

    // quickly create a volume with 8-bit colors
    for (unsigned long long iter = 0; iter < vol_size; ++iter) {
        Label_t label = *label_iter++;
        unsigned char mapping = 0;

        unordered_map<Label_t, unsigned char>::iterator color_iter = 
            color_mapping.find(label);
        if (color_iter != color_mapping.end()) {
            mapping = color_iter->second;
        }

        *mapped_iter++ = mapping;
    }
}

// call update and create controller -- rag, gray, and labels must exist
void StackBodyView::initialize()
{
    Stack* stack = stack_session->get_stack();
    labelvtk = vtkSmartPointer<vtkImageData>::New();

    VolumeLabelPtr labelvol = stack->get_labelvol();  
    if (!labelvol) {
        throw ErrMsg("Cannot Initialize: no label volume loaded");
    }

    current_volume_labels = VolumeLabelPtr(new VolumeLabelData(*labelvol));
    current_volume_labels->rebase_labels();

    // create initial volume to be rendered
    labels_rebase = new unsigned char [labelvol->shape(0) * labelvol->shape(1) * labelvol->shape(2)];
   
    unordered_map<Label_t, int> labels;
    create_label_volume(labels);

    labelarray = vtkSmartPointer<vtkUnsignedCharArray>::New();
    labelarray->SetArray(labels_rebase, labelvol->shape(0) * labelvol->shape(1) *
            labelvol->shape(2), 0);

    // set label options
    labelvtk->GetPointData()->SetScalars(labelarray);
    labelvtk->SetDimensions(labelvol->shape(0), labelvol->shape(1),
            labelvol->shape(2));
    labelvtk->SetScalarType(VTK_UNSIGNED_INT);
    labelvtk->SetSpacing(1, 1, 1);
    labelvtk->SetOrigin(0.0, 0.0, 0.0);

    // downsample data to render faster
    label_resample = vtkSmartPointer<vtkImageResample>::New();
    label_resample->SetInput(labelvtk);
    label_resample->SetAxisMagnificationFactor(0, 0.50);
    label_resample->SetAxisMagnificationFactor(1, 0.50);
    label_resample->SetAxisMagnificationFactor(2, 0.50);

    volumeMapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
    volumeMapper->SetInputConnection(label_resample->GetOutputPort());
    
    // to improve rendering performance
    volumeMapper->SetMinimumImageSampleDistance(2.0); 

    volumeColor = vtkSmartPointer<vtkColorTransferFunction>::New();

    volumeScalarOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();

    // set lookup table
    RagPtr rag = stack->get_rag();
    stack_session->compute_label_colors(rag);
    
    volumeColor->AddRGBPoint(0, 0, 0, 0);
    for (int i = 0; i <= 17; ++i) {
        unsigned char r, g, b;
        stack_session->get_rgb(i, r, g, b);
        volumeColor->AddRGBPoint(i+1, r/255.0, g/255.0, b/255.0);
    }

    volumeScalarOpacity->AddPoint(0, 0);
    volumeScalarOpacity->AddPoint(1, 1.0);

    volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetColor(volumeColor);
    volumeProperty->SetScalarOpacity(volumeScalarOpacity);
    volumeProperty->ShadeOn();
    
    volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);

    // show a plane object on the body to show the current plane
    plane_source = vtkSmartPointer<vtkPlaneSource>::New();
    plane_source->SetOrigin(0,0,0);
    plane_source->SetNormal(0,0,1);
    plane_source->SetPoint1(520, 0, 0);
    plane_source->SetPoint2(0, 520, 0);
    plane_source->SetXResolution(1); 
    plane_source->SetYResolution(1); 

    plane_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    plane_mapper->SetInputConnection(plane_source->GetOutputPort());

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(plane_mapper);
    actor->GetProperty()->SetColor(0.75, 0.75, 0.75);
    actor->GetProperty()->SetOpacity(0.0);
    actor->GetProperty()->SetLineWidth(2);

    ren1 = vtkSmartPointer<vtkRenderer>::New();
    ren1->AddViewProp(volume);
    ren1->AddActor(actor);
    ren1->SetBackground(1,1,1);
    ren1->ResetCamera();
   
    // angle the body view slightly to make it more visually pleasing 
    ren1->GetActiveCamera()->Elevation(45);
    ren1->GetActiveCamera()->OrthogonalizeViewUp();
    ren1->GetActiveCamera()->Elevation(65);
    ren1->GetActiveCamera()->OrthogonalizeViewUp();
    ren1->GetActiveCamera()->Zoom(0.85);

    rw = vtkSmartPointer<vtkRenderWindow>::New();
    rw->AddRenderer(ren1);
}

void StackBodyView::start()
{
    qt_widget = new QVTKWidget;
    if (widget_parent) {
        layout = new QVBoxLayout;
        layout->addWidget(qt_widget);
        widget_parent->setLayout(layout);
    }
    
    qt_widget->SetRenderWindow(rw);
    renderWindowInteractor = qt_widget->GetInteractor();
    rw->Render();
    qt_widget->show();
    _update(true);
}

void StackBodyView::_update(bool initialize)
{
    double rgba[4];
    unordered_map<Label_t, int> active_labels;

    unsigned int plane_id;

    // plane object position updated
    if (stack_session->get_plane(plane_id) || initialize) {
        double pt[3];
        plane_source->GetPoint1(pt);
        plane_source->SetPoint1(pt[0], pt[1], plane_id);
        plane_source->GetPoint2(pt);
        plane_source->SetPoint2(pt[0], pt[1], plane_id);
        plane_source->SetOrigin(0,0,plane_id);
        
        renderWindowInteractor->Render();
    }

    // only show bodies in the active label list
    if (stack_session->get_active_labels(active_labels) || initialize) {
        create_label_volume(active_labels);
        labelvtk->Modified();
        renderWindowInteractor->Render();
    }
}

void StackBodyView::update()
{
    _update(false);
}
