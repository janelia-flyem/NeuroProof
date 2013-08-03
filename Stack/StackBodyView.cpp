#include "StackBodyView.h"
#include "StackSession.h"
#include "QVTKWidget.h"

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

static boost::mutex mutex_create;
static boost::mutex mutex_meta;

StackBodyView::StackBodyView(StackSession* stack_session_) :
        stack_session(stack_session_), qt_widget(0)
{
    stack_session->attach_observer(this);
} 

StackBodyView::~StackBodyView()
{
    stack_session->detach_observer(this);
    if (qt_widget) {
        delete qt_widget;
    }
}

void StackBodyView::create_label_volume(std::tr1::unordered_set<unsigned int>& labels)
{
    unordered_map<Label_t, unsigned char> color_mapping;
    Stack* stack = stack_session->get_stack();
    RagPtr rag = stack->get_rag();

    for (unordered_set<Label_t>::iterator iter = labels.begin(); 
            iter != labels.end(); ++iter) {
        RagNode_t* node = rag->find_rag_node(*iter);
        int color_id = node->get_property<int>("color");
        unsigned char cmap = color_id % 18 + 1;
        color_mapping[*iter] = cmap;
    }

    unsigned char * mapped_iter = labels_rebase;
    Label_t* label_iter = current_volume_labels->begin();

    unsigned long long vol_size = current_volume_labels->shape(0) * 
        current_volume_labels->shape(1) * current_volume_labels->shape(2);

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

    labels_rebase = new unsigned char [labelvol->shape(0) * labelvol->shape(1) * labelvol->shape(2)];
   
    unordered_set<Label_t> labels;
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

    label_resample = vtkSmartPointer<vtkImageResample>::New();
    label_resample->SetInput(labelvtk);
    label_resample->SetAxisMagnificationFactor(0, 0.50);
    label_resample->SetAxisMagnificationFactor(1, 0.50);
    label_resample->SetAxisMagnificationFactor(2, 0.50);

    volumeMapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
    volumeMapper->SetInputConnection(label_resample->GetOutputPort());
    volumeMapper->SetMinimumImageSampleDistance(4.0); 

    volumeColor = vtkSmartPointer<vtkColorTransferFunction>::New();

    volumeScalarOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();

    // set lookup table
    stack_session->compute_label_colors();
    RagPtr rag = stack->get_rag();
    
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
    actor->GetProperty()->SetOpacity(1.0);
    actor->GetProperty()->SetLineWidth(2);

    ren1 = vtkSmartPointer<vtkRenderer>::New();
    ren1->AddViewProp(volume);
    ren1->AddActor(actor);
    ren1->SetBackground(1,1,1);
    ren1->ResetCamera();
    
    ren1->GetActiveCamera()->Elevation(45);
    ren1->GetActiveCamera()->OrthogonalizeViewUp();
    ren1->GetActiveCamera()->Elevation(65);
    ren1->GetActiveCamera()->OrthogonalizeViewUp();

    rw = vtkSmartPointer<vtkRenderWindow>::New();
    rw->AddRenderer(ren1);
}

void StackBodyView::start()
{
    qt_widget = new QVTKWidget;
    qt_widget->SetRenderWindow(rw);
    renderWindowInteractor = qt_widget->GetInteractor();
    rw->Render();
    qt_widget->show();
}

void StackBodyView::update()
{
    ScopeTime timer;
    double rgba[4];
    unordered_set<Label_t> active_labels;

    unsigned int plane_id;
    if (stack_session->get_plane(plane_id)) {
        double blah[3];
        ren1->GetActiveCamera()->GetPosition(blah);
        std::cout << "pos: " << blah[0] << " " << blah[1] << " " << blah[2] << std::endl;
        
        ren1->GetActiveCamera()->GetFocalPoint(blah);
        std::cout << "foc: " << blah[0] << " " << blah[1] << " " << blah[2] << std::endl;


        double pt[3];
        plane_source->GetPoint1(pt);
        plane_source->SetPoint1(pt[0], pt[1], plane_id);
        plane_source->GetPoint2(pt);
        plane_source->SetPoint2(pt[0], pt[1], plane_id);
        plane_source->SetOrigin(0,0,plane_id);
        
        renderWindowInteractor->Render();
    }

    if (stack_session->get_active_labels(active_labels)) {
        create_label_volume(active_labels);
        labelvtk->Modified();
        renderWindowInteractor->Render();
    }
}
